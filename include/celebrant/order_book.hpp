#pragma once

#include <map>
#include <unordered_map>

#include "celebrant/types.hpp"

namespace celebrant {

class OrderBook {
  public:
    OrderBook() = default;

    [[nodiscard]] CancelOutcome cancel(OrderKey key);

    [[nodiscard]] Outcome process(NewOrder new_order);

  private:
    Quantity remove_resting(Handle h);
    static bool price_acceptable(const NewOrder& new_order, Price best);
    static BookSide::iterator best_level(BookSide& resting_side, Side aggressor_side);
    BookSide bids_;
    BookSide asks_;
    std::unordered_map<OrderKey, Handle> index_;
};
} // namespace celebrant
//
