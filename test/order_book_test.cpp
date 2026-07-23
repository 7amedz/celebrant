#include <algorithm>

#include <gtest/gtest.h>

#include "celebrant/order_book.hpp"
#include "celebrant/types.hpp"

TEST(Match, FullFillEqualOrder) {
    celebrant::OrderBook book;
    celebrant::NewOrder order1 = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 25,
        .session = 15,
        .type = celebrant::OrderType::Limit,
    };

    celebrant::NewOrder order2 = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 25,
        .session = 16,
        .type = celebrant::OrderType::Limit,
    };

    book.process(order1);            // add resting order
    auto out = book.process(order2); // agg
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].price, order1.price);
    EXPECT_EQ(out.value()[0].quantity, std::min(order1.quantity, order2.quantity));
    EXPECT_EQ(out.value()[0].resting_remaining, 0);
    EXPECT_EQ(out.value()[0].aggressor.session, order2.session);
    EXPECT_EQ(out.value()[0].aggressor.id, order2.id);
    EXPECT_EQ(out.value()[0].resting.session, order1.session);
    EXPECT_EQ(out.value()[0].resting.id, order1.id);
    auto rest_cancel = book.cancel({.session = order1.session, .id = order1.id});
    ASSERT_FALSE(rest_cancel.has_value());
    EXPECT_EQ(rest_cancel.error(), celebrant::RejectReason::UnknownOrder);
    auto agg_cancel = book.cancel({.session = order2.session, .id = order2.id});
    ASSERT_FALSE(agg_cancel.has_value());
    EXPECT_EQ(agg_cancel.error(), celebrant::RejectReason::UnknownOrder);
}

TEST(Cancel, CancelMidQueue) {
    celebrant::OrderBook book;
    celebrant::NewOrder order1 = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    celebrant::NewOrder order2 = {
        .id = 2,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    celebrant::NewOrder order3 = {
        .id = 3,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 3,
        .type = celebrant::OrderType::Limit,
    };
    book.process(order1);
    book.process(order2);
    book.process(order3);

    auto cancel_out = book.cancel({.session = order2.session, .id = order2.id}); // cancel order2
    ASSERT_TRUE(cancel_out.has_value());
    EXPECT_EQ(cancel_out.value(), 10);
    celebrant::NewOrder buy = {
        .id = 4,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 20, // enough to trade with order 1 and order 3
        .session = 4,
        .type = celebrant::OrderType::Limit,
    };
    auto buy_outcome = book.process(buy);
    ASSERT_TRUE(buy_outcome.has_value());
    ASSERT_TRUE(buy_outcome.value().size() == 2);
    EXPECT_EQ(buy_outcome.value()[0].resting.session, order1.session);
    EXPECT_EQ(buy_outcome.value()[0].resting.id, order1.id);
    EXPECT_EQ(buy_outcome.value()[1].resting.session, order3.session);
    EXPECT_EQ(buy_outcome.value()[1].resting.id, order3.id);

    EXPECT_EQ(buy_outcome.value()[0].price, order1.price);
    EXPECT_EQ(buy_outcome.value()[0].quantity, order1.quantity);
    EXPECT_EQ(buy_outcome.value()[0].resting_remaining, 0);
    EXPECT_EQ(buy_outcome.value()[0].aggressor.session, buy.session);
    EXPECT_EQ(buy_outcome.value()[0].aggressor.id, buy.id);

    EXPECT_EQ(buy_outcome.value()[1].price, order3.price);
    EXPECT_EQ(buy_outcome.value()[1].quantity, order3.quantity);
    EXPECT_EQ(buy_outcome.value()[1].resting_remaining, 0);
    EXPECT_EQ(buy_outcome.value()[1].aggressor.session, buy.session);
    EXPECT_EQ(buy_outcome.value()[1].aggressor.id, buy.id);
}
