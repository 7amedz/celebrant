#include "celebrant/connection.hpp"

#include <cstddef>
#include <iostream>
#include <system_error>
#include <utility>
#include <variant>

#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#include "celebrant/connection_registry.hpp"
#include "celebrant/sbe_codec.hpp"

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

            while (auto len = sbe_codec::frame_length(accumulator_.data(), accumulator_.size())) {
                if (accumulator_.size() < *len) {
                    break; // acc doesn't have full message
                }
                auto outcome = sbe_codec::decode(accumulator_.data(), *len, session_);
                if (outcome.has_value()) {
                    std::visit([this](const auto& msg) { queue_.push(msg); }, outcome.value());
                } else {
                    const ParseError& err = outcome.error();
                    send(sbe_codec::encode(Reject{.id = err.id, .reason = err.reason}));
                }
                accumulator_.erase(0, *len);
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
