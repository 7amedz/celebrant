#pragma once

#include <algorithm>
#include <array>
#include <compare> //for std::strong_ordering in spaceship operator
#include <cstdint>
#include <functional> // for std::hash
#include <list>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "celebrant/expected.hpp"

namespace celebrant {

using Price = std::int64_t;
using Quantity = std::int64_t;
using SessionId = std::uint64_t;
using OrderId = std::uint64_t;

struct Symbol {
    std::array<char, 8> data{};

    Symbol() = default;
    explicit Symbol(std::string_view s) { // for decode (which checks size<9 before running this)
        std::ranges::copy(s, data.begin());
        // remaining bytes remain zero
    }

    auto operator<=>(const Symbol&) const = default;
    bool operator==(const Symbol&) const = default;
};

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
    Symbol symbol;

    bool operator==(const NewOrder&) const = default;
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

struct CancelAll {
    SessionId session;
};

struct Shutdown {};

using Request =
    std::variant<NewOrder, OrderKey, CancelAll, Shutdown>; // InboundQueue element type

} // namespace celebrant

template <> struct std::hash<celebrant::OrderKey> {
    std::size_t operator()(const celebrant::OrderKey& k) const noexcept {
        std::size_t h = std::hash<celebrant::SessionId>{}(k.session);
        h ^= std::hash<celebrant::OrderId>{}(k.id) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
