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

TEST(Match, AggressorSweep) {
    celebrant::OrderBook book;
    celebrant::NewOrder sell1 = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 3,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    celebrant::NewOrder sell2 = {
        .id = 2,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 4,
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    celebrant::NewOrder sell3 = {
        .id = 3,
        .side = celebrant::Side::Sell,
        .price = 101,
        .quantity = 5,
        .session = 3,
        .type = celebrant::OrderType::Limit,
    };
    celebrant::NewOrder sell4 = {
        .id = 4,
        .side = celebrant::Side::Sell,
        .price = 102,
        .quantity = 10,
        .session = 4,
        .type = celebrant::OrderType::Limit,
    };
    book.process(sell1);
    book.process(sell2);
    book.process(sell3);
    book.process(sell4);

    celebrant::NewOrder buy = {
        .id = 5,
        .side = celebrant::Side::Buy,
        .price = 0,     // market: price whatever
        .quantity = 22, // enough for all
        .session = 5,
        .type = celebrant::OrderType::Market,
    };
    auto out = book.process(buy);
    ASSERT_TRUE(out.has_value());
    ASSERT_EQ(out.value().size(), 4);

    EXPECT_EQ(out.value()[0].resting.id, sell1.id);
    EXPECT_EQ(out.value()[0].price, sell1.price);
    EXPECT_EQ(out.value()[0].quantity, sell1.quantity);

    EXPECT_EQ(out.value()[1].resting.id, sell2.id);
    EXPECT_EQ(out.value()[1].price, sell2.price);
    EXPECT_EQ(out.value()[1].quantity, sell2.quantity);

    EXPECT_EQ(out.value()[2].resting.id, sell3.id);
    EXPECT_EQ(out.value()[2].price, sell3.price);
    EXPECT_EQ(out.value()[2].quantity, sell3.quantity);

    EXPECT_EQ(out.value()[3].resting.id, sell4.id);
    EXPECT_EQ(out.value()[3].price, sell4.price);
    EXPECT_EQ(out.value()[3].quantity, sell4.quantity);
}

TEST(Match, AggressorRests) {
    celebrant::OrderBook book;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    book.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 15, // bigger than resting
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    auto out = book.process(buy);
    ASSERT_TRUE(out.has_value());
    ASSERT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].resting.id, sell.id);
    EXPECT_EQ(out.value()[0].quantity, sell.quantity); // resting fully consumed
    EXPECT_EQ(out.value()[0].resting_remaining, 0);

    // aggressor rests with qty:5 remaining so cancel will return 5
    auto residual_cancel = book.cancel({.session = buy.session, .id = buy.id});
    ASSERT_TRUE(residual_cancel.has_value());
    EXPECT_EQ(residual_cancel.value(), 5);
}

TEST(Match, AggressorGetsFilledRestingStays) {
    celebrant::OrderBook book;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    book.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 4, // less than resting
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    auto out = book.process(buy);
    ASSERT_TRUE(out.has_value());
    ASSERT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].resting.id, sell.id);
    EXPECT_EQ(out.value()[0].quantity, buy.quantity); // qty=aggressor qty
    EXPECT_EQ(out.value()[0].resting_remaining, 6);

    // resting still has 6 remaining. cancel should return 6
    auto rest_cancel = book.cancel({.session = sell.session, .id = sell.id});
    ASSERT_TRUE(rest_cancel.has_value());
    EXPECT_EQ(rest_cancel.value(), 6);

    // the buy fully filled, never rested
    auto agg_cancel = book.cancel({.session = buy.session, .id = buy.id});
    ASSERT_FALSE(agg_cancel.has_value());
    EXPECT_EQ(agg_cancel.error(), celebrant::RejectReason::UnknownOrder);
}

TEST(Match, AggressorBuyPriceLessThanBestAsk) {
    celebrant::OrderBook book;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 105,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    book.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100, // below best ask
        .quantity = 10,
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    auto out = book.process(buy);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out.value().size(), 0); // no trade

    // the buy rested so cancelling it should return original quantity
    auto rest_cancel = book.cancel({.session = buy.session, .id = buy.id});
    ASSERT_TRUE(rest_cancel.has_value());
    EXPECT_EQ(rest_cancel.value(), buy.quantity);
}

TEST(Match, MarketBuyEmptyBook) {
    celebrant::OrderBook book;
    celebrant::NewOrder buy = {
        .id = 1,
        .side = celebrant::Side::Buy,
        .price = 0,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Market,
    };
    auto out = book.process(buy);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out.value().size(), 0); // no trade

    auto cancel_out = book.cancel({.session = buy.session, .id = buy.id});
    ASSERT_FALSE(cancel_out.has_value());
    EXPECT_EQ(cancel_out.error(), celebrant::RejectReason::UnknownOrder);
}

TEST(Match, MarketPartialThenCancelled) {
    celebrant::OrderBook book;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 5,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    book.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 0,
        .quantity = 12, // more than the resting 5
        .session = 2,
        .type = celebrant::OrderType::Market,
    };
    auto out = book.process(buy);
    ASSERT_TRUE(out.has_value());
    ASSERT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].resting.id, sell.id);
    EXPECT_EQ(out.value()[0].quantity, sell.quantity); // fills the 5
    EXPECT_EQ(out.value()[0].resting_remaining, 0);

    auto cancel_out = book.cancel({.session = buy.session, .id = buy.id});
    ASSERT_FALSE(cancel_out.has_value());
    EXPECT_EQ(cancel_out.error(), celebrant::RejectReason::UnknownOrder);
}
