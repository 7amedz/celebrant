#pragma once

#include <cstddef>
#include <string>

#include "celebrant/codec.hpp"
#include "celebrant/types.hpp"

namespace celebrant::sbe_codec {

[[nodiscard]] DecodeOutcome decode(char* buffer, std::size_t len, SessionId session);

[[nodiscard]] std::string encode(const Ack& ack);
[[nodiscard]] std::string encode(const Fill& fill);
[[nodiscard]] std::string encode(const CancelConfirm& cancel_confirm);
[[nodiscard]] std::string encode(const Reject& reject);

} // namespace celebrant::sbe_codec
