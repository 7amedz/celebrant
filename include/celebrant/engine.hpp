#pragma once

#include <map>

#include "celebrant/order_book.hpp"
#include "celebrant/types.hpp"

namespace celebrant {

class Engine {
  public:
    Outcome process(NewOrder new_order);
    CancelOutcome cancel(OrderKey key);
    void cancel_all(SessionId session);

  private:
    std::map<Symbol, OrderBook> books_;
    EngineIndex index_;
};
} // namespace celebrant
