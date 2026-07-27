#include "celebrant/engine.hpp"

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

} // namespace celebrant
