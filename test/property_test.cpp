#include <map>
#include <random>

#include <gtest/gtest.h>

#include "celebrant/order_book.hpp"
#include "celebrant/types.hpp"

TEST(Property, QuantityConservation) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> side_d(0, 1);
    std::uniform_int_distribution<int> type_d(0, 1);
    std::uniform_int_distribution<celebrant::Price> price_d(95, 105);
    std::uniform_int_distribution<celebrant::Quantity> qty_d(1, 20);

    celebrant::OrderBook book;

    std::map<celebrant::OrderKey, celebrant::Quantity> submitted;
    std::map<celebrant::OrderKey, celebrant::Quantity> filled;

    for (celebrant::OrderId i = 1; i <= 1000; ++i) {

        celebrant::NewOrder o = {
            .id = i,
            .side = (side_d(rng) != 0) ? celebrant::Side::Buy : celebrant::Side::Sell,
            .price = price_d(rng),
            .quantity = qty_d(rng),
            .session = 1,
            .type = (type_d(rng) != 0) ? celebrant::OrderType::Limit : celebrant::OrderType::Market,
        };

        auto out = book.process(o);
        submitted[{.session = o.session, .id = o.id}] = o.quantity;

        ASSERT_TRUE(out.has_value());

        for (const celebrant::Trade& t : out.value()) {
            EXPECT_GT(t.quantity, 0);
            EXPECT_GE(t.resting_remaining, 0);
            filled[t.aggressor] += t.quantity;
            filled[t.resting] += t.quantity;

            EXPECT_LE(filled[t.aggressor], submitted[t.aggressor]);
            EXPECT_LE(filled[t.resting], submitted[t.resting]);
        }
    }
}
