#pragma once

#include <map>
#include <unordered_map>

#include "celebrant/types.hpp"

namespace celebrant {

class OrderBook; // for use in Location

struct Location {
    OrderBook* book; // using raaw pointer for reassignment
    Handle handle;
};

using EngineIndex = std::unordered_map<OrderKey, Location>;

class OrderBook {
  public:
    OrderBook() = default;

    [[nodiscard]] CancelOutcome cancel(OrderKey key, EngineIndex& index);

    [[nodiscard]] Outcome process(NewOrder new_order, EngineIndex& index);

  private:
    Quantity remove_resting(Handle h, EngineIndex& index);
    static bool price_acceptable(const NewOrder& new_order, Price best);
    static BookSide::iterator best_level(BookSide& resting_side, Side aggressor_side);
    BookSide bids_;
    BookSide asks_;
};
} // namespace celebrant
//
