#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "celebrant/codec.hpp"
#include "celebrant/types.hpp"

namespace celebrant::sbe_codec {

[[nodiscard]] std::optional<std::size_t> frame_length(char* buffer,
                                                      std::size_t available); // header+block

[[nodiscard]] DecodeOutcome decode(char* buffer, std::size_t len, SessionId session);

[[nodiscard]] std::string encode(const Ack& ack);
[[nodiscard]] std::string encode(const Fill& fill);
[[nodiscard]] std::string encode(const CancelConfirm& cancel_confirm);
[[nodiscard]] std::string encode(const Reject& reject);

} // namespace celebrant::sbe_codec
