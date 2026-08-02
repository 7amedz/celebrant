#include <csignal>
#include <thread>

#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>

#include "celebrant/connection_registry.hpp"
#include "celebrant/engine_runner.hpp"
#include "celebrant/gateway.hpp"
#include "celebrant/inbound_queue.hpp"

namespace asio = boost::asio;

int main(int argc, char* argv[]) {

    asio::io_context io;

    celebrant::ConnectionRegistry registry;
    celebrant::InboundQueue queue;
    celebrant::EngineRunner runner(queue, registry, io);

    std::jthread engine([&runner] { runner.run(); });
    celebrant::Gateway gateway(io, queue, 9001, registry);

    asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&](const std::error_code&, int) {
        queue.push(celebrant::Shutdown{}); // shutdown request to engine
        io.stop();                         // stop io thread
    });

    io.run();
    return 0;
}
