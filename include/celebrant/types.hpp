#pragma once

#include <compare> //for std::strong_ordering in spaceship operator
#include <cstdint>
#include <list>
#include <map>
#include <vector>

#include "celebrant/expected.hpp"

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

// resting order
struct Order {
    OrderId id;
    Side side;
    Price price;
    Quantity remaining;
    SessionId session;
};

// New incoming order
struct NewOrder {
    OrderId id;
    Side side;
    Price price;
    Quantity quantity;
    SessionId session;
    OrderType type;
};

struct Level {
    std::list<Order> orders;
    Quantity aggregate = 0;
};

using BookSide = std::map<Price, Level>;

struct Handle {
    BookSide::iterator level_it;
    std::list<Order>::iterator it;
};

enum class RejectReason : std::uint8_t {
    UnknownSymbol,
    UnknownOrder,
    InvalidQuantity,
    InvalidPrice,
};

struct OrderKey {
    SessionId session;
    OrderId id;

    // spaceship operator (defining operations <,==,>,>=,<=) to make OrderKey a valid map key
    // default: compares field by field
    auto operator<=>(const OrderKey&) const = default;
};

struct Trade {
    OrderKey aggressor;
    OrderKey resting;
    Price price;
    Quantity quantity;
    Quantity resting_remaining;
};

using Outcome = Expected<std::vector<Trade>, RejectReason>;
using CancelOutcome = Expected<Quantity, RejectReason>;

} // namespace celebrant
