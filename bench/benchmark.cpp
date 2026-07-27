// Run pinned to a single core to eliminate ping pong results

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "celebrant/engine.hpp"
#include "celebrant/types.hpp"

using std::size_t;
using namespace celebrant;

// time `orders` through a fresh engine, print percentiles under `label`
static void run(const char* label, const std::vector<NewOrder>& orders, int warmup) {
    Engine engine;

    // warmup loop
    for (int i = 0; i < warmup; ++i) {
        auto out = engine.process(orders[i]);
        (void)out;
    }

    std::vector<std::int64_t> lat_ns;
    lat_ns.reserve(orders.size() - warmup);
    std::size_t sink = 0;
    for (size_t i = warmup; i < orders.size(); ++i) {
        auto start = std::chrono::steady_clock::now();
        auto out = engine.process(orders[i]);
        auto end = std::chrono::steady_clock::now();
        sink += out.value().size(); // use the result -> no dead-code elimination
        lat_ns.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
    }

    std::sort(lat_ns.begin(), lat_ns.end());
    auto pct = [&](double p) {
        return lat_ns[static_cast<size_t>(
            static_cast<double>(lat_ns.size()) *
            p)]; // cast size to double to multiply by p then back to size_t
    };
    std::cout << "== " << label << " ==\n";
    std::cout << "orders timed: " << lat_ns.size() << "\n";
    std::cout << "p50 (ns):     " << pct(0.50) << "\n";
    std::cout << "p90 (ns):     " << pct(0.90) << "\n";
    std::cout << "p99 (ns):     " << pct(0.99) << "\n";
    std::cout << "max (ns):     " << lat_ns.back() << "\n";
    std::cout << "(sink=" << sink << ")\n\n";
}

int main() {
    constexpr int total = 200'000;
    constexpr int warmup = 20'000;
    constexpr int num_symbols = 500;

    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> side_d(0, 1);
    std::uniform_int_distribution<int> type_d(0, 99);          // < 90 -> Limit, else Market
    std::uniform_int_distribution<Price> price_d(9000, 11000); // wide spread -> deep book
    std::uniform_int_distribution<Quantity> qty_d(1, 100);
    std::uniform_int_distribution<SessionId> session_d(1, 10);
    std::uniform_int_distribution<int> sym_d(0, num_symbols - 1);

    // pool of distinct symbols: "SYM0000" .. "SYM0499"
    std::vector<Symbol> symbols;
    symbols.reserve(num_symbols);
    for (int k = 0; k < num_symbols; ++k) {
        symbols.push_back(Symbol{std::format("SYM{:04}", k)});
    }

    // identical order fields in both workloads; only the symbol differs
    std::vector<NewOrder> single; // default symbol -> one book
    std::vector<NewOrder> multi;  // random symbol -> num_symbols books
    single.reserve(total);
    multi.reserve(total);
    for (int i = 1; i <= total; ++i) {
        NewOrder o{
            .id = static_cast<OrderId>(i), // unique id -> unique (session, id)
            .side = (side_d(rng) != 0) ? Side::Buy : Side::Sell,
            .price = price_d(rng),
            .quantity = qty_d(rng),
            .session = session_d(rng),
            .type = (type_d(rng) < 90) ? OrderType::Limit : OrderType::Market,
        };
        single.push_back(o);
        o.symbol = symbols[sym_d(rng)];
        multi.push_back(o);
    }

    run("single-symbol (1 book)", single, warmup);
    run("multi-symbol (500 books)", multi, warmup);
    return 0;
}
