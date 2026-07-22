#pragma once

#include <cstdint>
#include <list>
#include <map>

namespace celebrant {

using Price = std::int64_t;
using Quantity = std::int64_t;
using SessionId = std::uint64_t;
using OrderId = std::uint64_t;

enum class Side : std::uint8_t {
    Buy,
    Sell,
};

enum class OrderType : std::uint8_t {
    Limit,
    Market,
};

struct Order {
    OrderId id;
    Side side;
    Price price;
    Quantity remaining;
    SessionId session;
};
struct Level {
    std::list<Order> orders;
    Quantity aggregate;
};

struct Handle {
    std::map<Price, Level>::iterator level_it;
    std::list<Order>::iterator it;
};

enum class RejectReason : std::uint8_t {
    UnknownSymbol,
    UnknownOrder,
    InvalidQuantity,
    InvalidPrice,
};

} // namespace celebrant
