#pragma once

#include <map>

#include "celebrant/types.hpp"

namespace celebrant {

class OrderBook {
  public:
    OrderBook() = default;

    [[nodiscard]] CancelOutcome cancel(OrderKey key);

    [[nodiscard]] Outcome process(NewOrder new_order);

  private:
    std::map<Price, Level> bids_;
    std::map<Price, Level> asks_;
    std::map<OrderKey, Handle> index_;
};
} // namespace celebrant
