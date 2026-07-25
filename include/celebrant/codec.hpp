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

} // namespace celebrant
