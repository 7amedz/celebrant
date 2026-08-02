

#include "celebrant/gateway.hpp"

#include <memory>

#include "celebrant/connection.hpp"

namespace celebrant {

Gateway::Gateway(asio::io_context& io, InboundQueue& queue, unsigned short port)
    : acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), queue_(queue) {
    do_accept();
}
void Gateway::do_accept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
        if (!ec) {
            auto conn_sptr = std::make_shared<Connection>(
                std::move(socket), queue_, next_session_++); // new connection with sptr
            conn_sptr->start_read(); // start read uses sptr and keeps it alive after this scope
        }
        this->do_accept();
    });
}

} // namespace celebrant
