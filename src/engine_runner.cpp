#include "celebrant/engine_runner.hpp"

#include "celebrant/types.hpp"

namespace celebrant {

/*  The Lambda we pass into std::visit needs to account for all types (NewOrder/OrderKey)
 *  this needs to be inside the run function since engine_ is private

struct Visitor {
    EngineRunner* self;   // what [this] captures
    void operator()(const NewOrder& o) { self->engine_.process(o); }   //NewOrder version
    void operator()(const OrderKey& k) { self->engine_.cancel(k);  }   // OrderKey
    // operator() so we could call visitor(o)...
};

    */

EngineRunner::EngineRunner(InboundQueue& inbound) : inbound_(inbound) {};

template <class... Ts> struct Overloaded : Ts... {
    using Ts::operator()...;
}; // template for overloaded lambda
void EngineRunner::run() {

    for (;;) {
        Request request = inbound_.pop();

        // if (std::holds_alternative<NewOrder>(request)) {
        //     engine_.process(std::get<NewOrder>(request));
        // } else {
        //     engine_.cancel(std::get<OrderKey>(request));
        // }

        std::visit(Overloaded{
                       [this](const NewOrder& o) { engine_.process(o); },
                       [this](const OrderKey& k) { engine_.cancel(k); },
                   },
                   request);

        // std::visit (function,variant) returns what function returns
    }
}

} // namespace celebrant
