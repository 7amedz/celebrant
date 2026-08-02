
#pragma once

#include <boost/asio/io_context.hpp>

#include "celebrant/connection_registry.hpp"
#include "celebrant/engine.hpp"
#include "celebrant/inbound_queue.hpp"
#include "celebrant/types.hpp"

namespace celebrant {

class EngineRunner {
  public:
    explicit EngineRunner(InboundQueue& inbound, const ConnectionRegistry& registry,
                          boost::asio::io_context& io);
    void run();

  private:
    void send_to(SessionId sid, std::string bytes);
    Engine engine_;
    InboundQueue& inbound_;
    const ConnectionRegistry& registry_;
    boost::asio::io_context& io_;
};
} // namespace celebrant
