#include "celebrant/engine_runner.hpp"

#include <variant>

#include <boost/asio/post.hpp>

#include "celebrant/codec.hpp"
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

EngineRunner::EngineRunner(InboundQueue& inbound, const ConnectionRegistry& registry,
                           boost::asio::io_context& io)
    : inbound_(inbound), registry_(registry), io_(io) {};

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

        bool result =
            std::visit(Overloaded{
                           [this](const NewOrder& o) {
                               Outcome outcome = engine_.process(o);
                               if (!outcome.has_value()) {
                                   send_to(o.session, encode(Reject{.id = o.id, .reason = outcome.error()}));
                                   return true;
                               }
                               send_to(o.session, encode(Ack{.id = o.id}));
                               Side resting_side = (o.side == Side::Buy) ? Side::Sell : Side::Buy;
                               Quantity remaining = o.quantity;
                               for (const Trade& t : outcome.value()) {
                                   remaining -= t.quantity;
                                   send_to(t.aggressor.session,
                                           encode(Fill{.id = t.aggressor.id,
                                                       .symbol = o.symbol,
                                                       .side = o.side,
                                                       .qty_filled = t.quantity,
                                                       .price = t.price,
                                                       .qty_remaining = remaining}));
                                   send_to(t.resting.session,
                                           encode(Fill{.id = t.resting.id,
                                                       .symbol = o.symbol,
                                                       .side = resting_side,
                                                       .qty_filled = t.quantity,
                                                       .price = t.price,
                                                       .qty_remaining = t.resting_remaining}));
                               }
                               return true;
                           },
                           [this](const OrderKey& k) {
                               CancelOutcome outcome = engine_.cancel(k);
                               if (!outcome.has_value()) {
                                   send_to(k.session, encode(Reject{.id = k.id, .reason = outcome.error()}));
                               } else {
                                   send_to(k.session, encode(CancelConfirm{.id = k.id}));
                               }
                               return true;
                           },
                           [](const Shutdown& s) { return false; }, // no need to capture this
                       },
                       request);

        // std::visit (function,variant) returns what function returns
        if (!result) {
            break;
        }
    }
}

void EngineRunner::send_to(SessionId sid, std::string bytes) {
    boost::asio::post(
        io_,
        [this, sid,
         bytes = std::move(bytes)]() mutable { // mutable lambda(we are changing captured field)
            if (auto conn = registry_.get(sid)) {
                conn->send(std::move(bytes));
            }
        });
}

} // namespace celebrant
