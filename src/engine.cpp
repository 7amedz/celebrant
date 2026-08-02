#include "celebrant/engine.hpp"

#include <vector>

#include "celebrant/types.hpp"

namespace celebrant {

Outcome Engine::process(NewOrder new_order) {
    auto& book = this->books_[new_order.symbol];
    return book.process(new_order, index_);
}

CancelOutcome Engine::cancel(OrderKey key) {
    auto it = index_.find(key);
    if (it == index_.end()) {
        return RejectReason::UnknownOrder;
    }
    auto location = index_[key];
    auto* book = location.book;
    return book->cancel(key, index_);
}

void Engine::cancel_all(SessionId session) {
    std::vector<OrderKey> keys;
    for (const auto& [key, loc] : index_) {
        if (key.session == session) {
            keys.push_back(key);
        }
    }
    for (const OrderKey& key : keys) {
        cancel(key);
    }
}

} // namespace celebrant
