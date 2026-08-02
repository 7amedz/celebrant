#pragma once

#include <boost/asio.hpp>

#include "celebrant/connection_registry.hpp"
#include "celebrant/inbound_queue.hpp"
#include "celebrant/types.hpp"

namespace celebrant {
namespace asio = boost::asio;

class Gateway {

  public:
    Gateway(asio::io_context& io, InboundQueue& queue, unsigned short port,
            ConnectionRegistry& registry);

  private:
    void do_accept();
    boost::asio::ip::tcp::acceptor acceptor_; // acceptor(listener) socket requires io and
                                              // endpoint(version,port) on construction
    // no default cons

    InboundQueue& queue_;
    SessionId next_session_ = 1;
    ConnectionRegistry& registry_;
};

} // namespace celebrant
