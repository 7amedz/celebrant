#include "celebrant/codec.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
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
                           .type = type};
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

} // namespace celebrant
