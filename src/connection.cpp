#include "celebrant/connection.hpp"

#include <cstddef>
#include <iostream>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#include "celebrant/codec.hpp"
#include "celebrant/connection_registry.hpp"

namespace celebrant {

Connection::Connection(boost::asio::ip::tcp::socket sock, InboundQueue& queue, SessionId session,
                       ConnectionRegistry& registry)
    : sock_(std::move(sock)), queue_(queue), session_(session), registry_(registry) {}

void Connection::start_read() {
    auto self = shared_from_this();
    sock_.async_read_some(
        boost::asio::buffer(buf_), [this, self](std::error_code ec, std::size_t n) {
            if (ec) {
                registry_.remove(session_);
                queue_.push(CancelAll{session_});
                return; // re arm only if no error
            }
            accumulator_.append(buf_.data(), n);

            std::size_t position;
            while ((position = accumulator_.find('\n')) != std::string::npos) { // if delimiter
                                                                                // found
                std::string_view line(accumulator_.data(), position);
                auto outcome = decode(line, session_);
                if (outcome.has_value()) {
                    std::visit([this](const auto& msg) { queue_.push(msg); }, outcome.value());
                } else {
                    const ParseError& err = outcome.error();
                    send(encode(Reject{.id = err.id, .reason = err.reason}));
                }
                accumulator_.erase(0, position + 1); // erase upto and past the newline
            }
            start_read(); // re arm (insta return)
        });
}

void Connection::send(std::string message) {
    outbox_.push_back(std::move(message));
    if (outbox_.size() == 1) {
        do_write();
    }
}

void Connection::do_write() {
    auto self = shared_from_this();
    boost::asio::async_write(sock_, boost::asio::buffer(outbox_.front()),
                             [this, self](std::error_code ec, std::size_t n) {
                                 if (ec) {
                                     registry_.remove(session_);
                                     queue_.push(CancelAll{session_});
                                     return;
                                 }
                                 outbox_.pop_front();
                                 if (!outbox_.empty()) {
                                     do_write();
                                 }
                             }); // handler runs on write
}

} // namespace celebrant
