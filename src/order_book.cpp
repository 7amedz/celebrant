#include "celebrant/order_book.hpp"

#include "celebrant/types.hpp"

namespace celebrant {

Quantity OrderBook::remove_resting(Handle h) {
    Quantity removed = h.it->remaining;
    OrderKey key = {.session = h.it->session, .id = h.it->id};
    Side side = h.it->side;
    Level& level = h.level_it->second; // first is price
    level.aggregate -= removed;
    level.orders.erase(h.it);
    std::map<Price, Level>& book = (side == Side::Buy) ? bids_ : asks_;
    if (level.orders.empty()) {
        book.erase(h.level_it);
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

} // namespace celebrant
