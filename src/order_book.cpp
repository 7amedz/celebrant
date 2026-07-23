#include "celebrant/order_book.hpp"

#include <algorithm>
#include <iterator>
#include <list>
#include <vector>

#include "celebrant/types.hpp"

namespace celebrant {

Quantity OrderBook::remove_resting(Handle h) {
    Quantity removed = h.it->remaining;
    OrderKey key = {.session = h.it->session, .id = h.it->id};
    Side side = h.it->side;
    Level& level = h.level_it->second; // first is price
    level.aggregate -= removed;
    level.orders.erase(h.it);
    BookSide& bookside = (side == Side::Buy) ? bids_ : asks_;
    if (level.orders.empty()) {
        bookside.erase(h.level_it);
    }
    index_.erase(key);

    return removed;
}
CancelOutcome OrderBook::cancel(OrderKey key) {
    auto found = index_.find(key); // use find to get it
    if (found == index_.end()) {   // check if not in book
        RejectReason reason = RejectReason::UnknownOrder;
        return reason;
    }
    Quantity removed_qty = remove_resting(found->second);
    return removed_qty;
}

bool OrderBook::price_acceptable(const NewOrder& new_order, Price best) {
    switch (new_order.type) {
    case OrderType::Market:
        return true;
    case OrderType::Limit:
        return new_order.side == Side::Buy ? best <= new_order.price : best >= new_order.price;
    }
    return false;
}

BookSide::iterator OrderBook::best_level(BookSide& resting_side, Side aggressor_side) {
    return (aggressor_side == Side::Buy) ? resting_side.begin() : std::prev(resting_side.end());
}

Outcome OrderBook::process(NewOrder new_order) {
    Quantity aggressor_remaining = new_order.quantity;
    Side order_side = new_order.side;
    BookSide& resting_bookside = (order_side == Side::Sell) ? bids_ : asks_;
    BookSide& aggressor_bookside = (order_side == Side::Sell) ? asks_ : bids_;
    std::vector<Trade> trades;
    while (aggressor_remaining && !resting_bookside.empty()) {
        auto best_price_level = best_level(resting_bookside, order_side);

        if (!price_acceptable(new_order, best_price_level->first)) {
            break;
        }
        std::list<Order>& resting_orders = best_price_level->second.orders;
        Order& best_resting_order = resting_orders.front();
        Quantity trade_qty = std::min(aggressor_remaining, best_resting_order.remaining);
        trades.push_back(Trade{
            .aggressor = {.session = new_order.session, .id = new_order.id},
            .resting = {.session = best_resting_order.session, .id = best_resting_order.id},
            .price = best_resting_order.price,
            .quantity = trade_qty,
            .resting_remaining = best_resting_order.remaining - trade_qty,
        });
        aggressor_remaining -= trade_qty;
        best_resting_order.remaining -= trade_qty;
        best_price_level->second.aggregate -= trade_qty;
        if (best_resting_order.remaining == 0) {
            auto r_it = resting_orders.begin();
            remove_resting({.level_it = best_price_level, .it = r_it});
        }
    }
    if (aggressor_remaining > 0 &&
        new_order.type == OrderType::Limit) { // in case of market type it gets dropped and derived
        // TODO: extract the if body into a function add_resting
        Order order{
            .id = new_order.id,
            .side = new_order.side,
            .price = new_order.price,
            .remaining = aggressor_remaining,
            .session = new_order.session,
        };
        auto price_level_it =
            aggressor_bookside.try_emplace(order.price)
                .first; // assign iterator to existing price level or create new one;
        std::list<Order>& orders_list = price_level_it->second.orders;
        auto it = orders_list.insert(orders_list.end(), order);
        price_level_it->second.aggregate += order.remaining;
        index_[{.session = order.session, .id = order.id}] = {.level_it = price_level_it, .it = it};
    }
    return trades;
}

} // namespace celebrant
