

#include "celebrant/gateway.hpp"

#include <memory>

#include "celebrant/connection.hpp"
#include "celebrant/connection_registry.hpp"

namespace celebrant {

Gateway::Gateway(asio::io_context& io, InboundQueue& queue, unsigned short port,
                 ConnectionRegistry& registry)
    : acceptor_(io, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)), queue_(queue),
      registry_(registry) {
    do_accept();
}
void Gateway::do_accept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
        if (!ec) {
            auto conn_sptr = std::make_shared<Connection>(
                std::move(socket), queue_, next_session_++, registry_); // new connection with sptr
            conn_sptr->start_read(); // start read uses sptr and keeps it alive after this scope
            this->registry_.add(next_session_ - 1, conn_sptr);
        }
        this->do_accept();
    });
}

} // namespace celebrant
