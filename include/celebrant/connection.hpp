#pragma once

#include <array>
#include <deque>
#include <memory>
#include <string>

#include <boost/asio/ip/tcp.hpp>

#include "celebrant/inbound_queue.hpp"
#include "celebrant/types.hpp"

namespace celebrant {

class ConnectionRegistry;

class Connection : public std::enable_shared_from_this<Connection> {
  public:
    Connection(boost::asio::ip::tcp::socket sock, InboundQueue& queue, SessionId session,
               ConnectionRegistry& registry);
    void start_read();
    void send(std::string message);

  private:
    void do_write();
    boost::asio::ip::tcp::socket sock_;
    InboundQueue& queue_;
    SessionId session_;
    ConnectionRegistry& registry_;
    std::array<char, 1024> buf_;
    std::string accumulator_; // leftover bytes
    std::deque<std::string> outbox_;
};

} // namespace celebrant
