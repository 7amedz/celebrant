#include "celebrant/codec.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "celebrant/expected.hpp"
#include "celebrant/types.hpp"

namespace celebrant {

template <typename T> Expected<T, ParseRejectReason> parse_number(std::string_view field) {
    T value;
    auto result = std::from_chars(field.data(), field.data() + field.size(), value);
    if (result.ec == std::errc::invalid_argument) {
        return ParseRejectReason::NonNumericField;
    }
    if (result.ec == std::errc::result_out_of_range) {
        return ParseRejectReason::NumberOutOfRange;
    }
    if (result.ptr != field.data() + field.size()) {
        return ParseRejectReason::NonNumericField; // not parsed entirely
    }
    return value;
}

Expected<OrderType, ParseRejectReason> parse_order_type(std::string_view field) {
    if (field == "MARKET") {
        return OrderType::Market;
    }
    if (field == "LIMIT") {

        return OrderType::Limit;
    }
    return ParseRejectReason::UnknownOrderType;
}

Expected<Side, ParseRejectReason> parse_side(std::string_view field) {
    if (field == "BUY") {
        return Side::Buy;
    }
    if (field == "SELL") {

        return Side::Sell;
    }
    return ParseRejectReason::UnknownSide;
}

Expected<Price, ParseRejectReason> parse_price(std::string_view field) {
    Price price = 0;

    size_t delimiter = field.find('.'); // look for dot
    //
    if (delimiter == std::string_view::npos) { // if no dot

        auto parsed_price = parse_number<Price>(field);
        if (parsed_price.has_value()) {
            if (parsed_price.value() > (INT64_MAX / 100)) {

                return ParseRejectReason::NumberOutOfRange;
            }
            price = 100 * parsed_price.value();
            return price;
        }
        return parsed_price.error();
    }

    std::string_view pre_dot = field.substr(0, delimiter);
    Price left = 0;
    auto parsed_pre_dot = parse_number<Price>(pre_dot);
    if (parsed_pre_dot.has_value()) {

        if (parsed_pre_dot.value() > ((INT64_MAX / 100) - 100)) {

            return ParseRejectReason::NumberOutOfRange;
        }
        left = 100 * parsed_pre_dot.value();
    } else {

        return parsed_pre_dot.error();
    }

    field.remove_prefix(delimiter + 1); // moves internal ptr after dot
    Price right = 0;
    auto parsed_post_dot = parse_number<Price>(field);
    if (parsed_post_dot.has_value()) {
        size_t post_dot_digits = field.size();
        if (post_dot_digits > 2) {
            return ParseRejectReason::ExcessPricePrecision;
        }
        if (post_dot_digits == 1) {
            right = 10 * parsed_post_dot.value();
        } else {

            right = parsed_post_dot.value();
        }
    } else {
        return parsed_post_dot.error();
    }
    price = left + right;
    return price;
}

DecodeOutcome decode(std::string_view line, SessionId session) {

    std::vector<std::string_view> fields;
    while (true) {
        size_t delimiter = line.find(',');
        std::string_view field = line.substr(0, delimiter);
        // up to the comma (or till end if no comma exists)
        fields.push_back(field);
        if (delimiter == std::string_view::npos) {
            break;
        }
        line.remove_prefix(delimiter + 1); // moves internal ptr
    }
    if (fields[0] == "NEW") {
        size_t field_count = fields.size();
        if (field_count < 2) {
            return ParseError{.reason = ParseRejectReason::WrongFieldCount, .id = std::nullopt};
        }
        OrderId id;
        auto id_parse = parse_number<OrderId>(fields[1]);
        if (id_parse.has_value()) {
            id = id_parse.value();
        } else {
            return ParseError{.reason = id_parse.error(), .id = std::nullopt};
        }
        if (field_count < 5)
            return ParseError{.reason = ParseRejectReason::WrongFieldCount, .id = id};

        Symbol symbol = std::string(fields[2]);

        OrderType type;
        auto type_parse = parse_order_type(fields[4]);
        if (type_parse.has_value()) {
            type = type_parse.value();
        } else {

            return ParseError{.reason = type_parse.error(), .id = id};
        }
        if (type == OrderType::Limit) {
            if (field_count != 7) {
                return ParseError{.reason = ParseRejectReason::WrongFieldCount, .id = id};
            }
        }

        if (type == OrderType::Market) {
            if (field_count != 6) {
                return ParseError{.reason = ParseRejectReason::WrongFieldCount, .id = id};
            }
        }
        Side side;
        auto parsed_side = parse_side(fields[3]);
        if (parsed_side.has_value()) {
            side = parsed_side.value();

        } else {

            return ParseError{.reason = parsed_side.error(), .id = id};
        }
        Quantity qty;
        auto parsed_qty = parse_number<Quantity>(fields[5]);
        if (parsed_qty.has_value()) {
            qty = parsed_qty.value();

        } else {

            return ParseError{.reason = parsed_qty.error(), .id = id};
        }
        Price price = 0;
        if (type == OrderType::Limit) {
            auto parsed_price = parse_number<Price>(fields[6]);
            if (parsed_price.has_value()) {
                price = parsed_price.value();

            } else {

                return ParseError{.reason = parsed_price.error(), .id = id};
            }
        }
        NewOrder new_order{.id = id,
                           .side = side,
                           .price = price,
                           .quantity = qty,
                           .session = session,
                           .type = type,
                           .symbol = symbol};
        return std::variant<NewOrder, OrderKey>{new_order};

    } else if (fields[0] == "CANCEL") {
        size_t field_count = fields.size();
        if (field_count < 2) {
            return ParseError{.reason = ParseRejectReason::WrongFieldCount, .id = std::nullopt};
        }
        OrderId id;
        auto id_parse = parse_number<OrderId>(fields[1]);
        if (id_parse.has_value()) {
            id = id_parse.value();
        } else {
            return ParseError{.reason = id_parse.error(), .id = std::nullopt};
        }
        if (field_count != 2) {
            return ParseError{.reason = ParseRejectReason::WrongFieldCount, .id = id};
        }
        OrderKey key{.session = session, .id = id};
        return std::variant<NewOrder, OrderKey>{key};

    } else {
        return ParseError{.reason = ParseRejectReason::UnknownMessageType, .id = std::nullopt};
    }
}

std::string encode(const Ack& ack) {
    return std::format("ACK,{}\n", ack.id);
}

std::string_view to_token(RejectReason r) {
    switch (r) {
    case RejectReason::UnknownOrder:
        return "UnknownOrder";
    case RejectReason::UnknownSymbol:
        return "UnknownSymbol";
    case RejectReason::InvalidQuantity:
        return "InvalidQuantity";
    case RejectReason::InvalidPrice:
        return "InvalidPrice";
    }
    return "";
}

std::string_view to_token(ParseRejectReason r) {
    switch (r) {
    case ParseRejectReason::UnknownMessageType:
        return "UnknownMessageType";
    case ParseRejectReason::WrongFieldCount:
        return "WrongFieldCount";
    case ParseRejectReason::NonNumericField:
        return "NonNumericField";
    case ParseRejectReason::NumberOutOfRange:
        return "NumberOutOfRange";
    case ParseRejectReason::UnknownSide:
        return "UnknownSide";
    case ParseRejectReason::UnknownOrderType:
        return "UnknownOrderType";
    case ParseRejectReason::ExcessPricePrecision:
        return "ExcessPricePrecision";
    }
    return "";
}

std::string encode(const Reject& reject) {
    std::string id = (reject.id.has_value()) ? std::to_string(reject.id.value()) : "";
    // std::visit(callable,variant) pulls out variant tag and gives the value and calls your
    // callable with it. Callable must handle every variant alternative
    std::string_view reason = std::visit([](auto r) { return to_token(r); }, reject.reason);
    return std::format("REJECT,{},{}\n", id, reason);
}

std::string encode(const CancelConfirm& cancel_confirm) {
    return std::format("CXL,{}\n", cancel_confirm.id);
}

std::string_view side_token(Side side) {
    return side == Side::Buy ? "BUY" : "SELL";
}

std::string price_to_string(Price ticks) {
    Price dollars = ticks / 100;
    Price cents = ticks % 100; // get last two digits $$
    return std::format("{}.{:02}", dollars, cents);
}

std::string encode(const Fill& fill) {
    return std::format("FILL,{},{},{},{},{},{}\n", fill.id, fill.symbol, side_token(fill.side),
                       fill.qty_filled, price_to_string(fill.price), fill.qty_remaining);
}

std::string encode(const TradePrint& trade) {
    return std::format("TRADE,{},{},{},{}\n", trade.symbol, trade.quantity,
                       price_to_string(trade.price), trade.seq);
}

std::string encode(const BookUpdate& book) {
    return std::format("BOOK,{},{},{},{},{}\n", book.symbol, side_token(book.side),
                       price_to_string(book.price), book.aggregate_qty, book.seq);
}

} // namespace celebrant
