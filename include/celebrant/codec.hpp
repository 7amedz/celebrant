// coder/decoder hpp

#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

#include "celebrant/expected.hpp"
#include "celebrant/types.hpp"
namespace celebrant {

enum class ParseRejectReason : std::uint8_t {
    UnknownMessageType,
    WrongFieldCount,
    NonNumericField,
    NumberOutOfRange,
    UnknownSide,
    UnknownOrderType,
    ExcessPricePrecision,
};

struct ParseError {
    ParseRejectReason reason;
    std::optional<OrderId> id;
};

using DecodeOutcome = Expected<std::variant<NewOrder, OrderKey>, ParseError>;
[[nodiscard]] DecodeOutcome decode(std::string_view line, SessionId session);

struct Ack {
    OrderId id;
};

struct Reject {
    std::optional<OrderId> id;
    std::variant<RejectReason, ParseRejectReason> reason;
};

struct Fill {
    OrderId id;
    Symbol symbol;
    Side side;
    Quantity qty_filled;
    Price price;
    Quantity qty_remaining;
};

struct CancelConfirm {
    OrderId id;
};

using FeedSeq = std::uint64_t;

struct TradePrint {
    Symbol symbol;
    Quantity quantity;
    Price price;
    FeedSeq seq;
};

struct BookUpdate {
    Symbol symbol;
    Side side;
    Price price;
    Quantity aggregate_qty;
    FeedSeq seq;
};

[[nodiscard]] std::string encode(const Ack& ack);
[[nodiscard]] std::string encode(const Reject& reject);
[[nodiscard]] std::string encode(const CancelConfirm& cancel_confirm);
[[nodiscard]] std::string encode(const Fill& fill);
[[nodiscard]] std::string encode(const TradePrint& trade);
[[nodiscard]] std::string encode(const BookUpdate& book);

} // namespace celebrant
