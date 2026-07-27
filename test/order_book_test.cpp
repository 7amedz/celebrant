#include <algorithm>

#include <gtest/gtest.h>

#include "celebrant/engine.hpp"
#include "celebrant/types.hpp"

// buy meets a sell of equal qty: one trade, both fully gone
TEST(Match, FullFillEqualOrder) {
    celebrant::Engine engine;
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

    engine.process(order1);            // add resting order
    auto out = engine.process(order2); // agg
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].price, order1.price);
    EXPECT_EQ(out.value()[0].quantity, std::min(order1.quantity, order2.quantity));
    EXPECT_EQ(out.value()[0].resting_remaining, 0);
    EXPECT_EQ(out.value()[0].aggressor.session, order2.session);
    EXPECT_EQ(out.value()[0].aggressor.id, order2.id);
    EXPECT_EQ(out.value()[0].resting.session, order1.session);
    EXPECT_EQ(out.value()[0].resting.id, order1.id);
    auto rest_cancel = engine.cancel({.session = order1.session, .id = order1.id});
    ASSERT_FALSE(rest_cancel.has_value());
    EXPECT_EQ(rest_cancel.error(), celebrant::RejectReason::UnknownOrder);
    auto agg_cancel = engine.cancel({.session = order2.session, .id = order2.id});
    ASSERT_FALSE(agg_cancel.has_value());
    EXPECT_EQ(agg_cancel.error(), celebrant::RejectReason::UnknownOrder);
}

// cancel the middle order, the other two keep their queue order
TEST(Cancel, CancelMidQueue) {
    celebrant::Engine engine;
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
    engine.process(order1);
    engine.process(order2);
    engine.process(order3);

    auto cancel_out = engine.cancel({.session = order2.session, .id = order2.id}); // cancel order2
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
    auto buy_outcome = engine.process(buy);
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

// market buy sweeps several resting orders. One trade each
TEST(Match, AggressorSweep) {
    celebrant::Engine engine;
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
    engine.process(sell1);
    engine.process(sell2);
    engine.process(sell3);
    engine.process(sell4);

    celebrant::NewOrder buy = {
        .id = 5,
        .side = celebrant::Side::Buy,
        .price = 0,     // market: price whatever
        .quantity = 22, // enough for all
        .session = 5,
        .type = celebrant::OrderType::Market,
    };
    auto out = engine.process(buy);
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

// buy bigger than the resting order. Remaining rests
TEST(Match, AggressorRests) {
    celebrant::Engine engine;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 15, // bigger than resting
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    auto out = engine.process(buy);
    ASSERT_TRUE(out.has_value());
    ASSERT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].resting.id, sell.id);
    EXPECT_EQ(out.value()[0].quantity, sell.quantity); // resting fully consumed
    EXPECT_EQ(out.value()[0].resting_remaining, 0);

    // aggressor rests with qty:5 remaining so cancel will return 5
    auto residual_cancel = engine.cancel({.session = buy.session, .id = buy.id});
    ASSERT_TRUE(residual_cancel.has_value());
    EXPECT_EQ(residual_cancel.value(), 5);
}

// buy smaller than resting: buy done, resting stays with the remaining
TEST(Match, AggressorGetsFilledRestingStays) {
    celebrant::Engine engine;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 4, // less than resting
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    auto out = engine.process(buy);
    ASSERT_TRUE(out.has_value());
    ASSERT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].resting.id, sell.id);
    EXPECT_EQ(out.value()[0].quantity, buy.quantity); // qty=aggressor qty
    EXPECT_EQ(out.value()[0].resting_remaining, 6);

    // resting still has 6 remaining. cancel should return 6
    auto rest_cancel = engine.cancel({.session = sell.session, .id = sell.id});
    ASSERT_TRUE(rest_cancel.has_value());
    EXPECT_EQ(rest_cancel.value(), 6);

    // the buy fully filled, never rested
    auto agg_cancel = engine.cancel({.session = buy.session, .id = buy.id});
    ASSERT_FALSE(agg_cancel.has_value());
    EXPECT_EQ(agg_cancel.error(), celebrant::RejectReason::UnknownOrder);
}

// buy priced below the best ask, no cross, it just rests
TEST(Match, AggressorBuyPriceLessThanBestAsk) {
    celebrant::Engine engine;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 105,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100, // below best ask
        .quantity = 10,
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    auto out = engine.process(buy);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out.value().size(), 0); // no trade

    // the buy rested so cancelling it should return original quantity
    auto rest_cancel = engine.cancel({.session = buy.session, .id = buy.id});
    ASSERT_TRUE(rest_cancel.has_value());
    EXPECT_EQ(rest_cancel.value(), buy.quantity);
}

// market buy with no sell resting: no trade. dropped
TEST(Match, MarketBuyEmptyBook) {
    celebrant::Engine engine;
    celebrant::NewOrder buy = {
        .id = 1,
        .side = celebrant::Side::Buy,
        .price = 0,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Market,
    };
    auto out = engine.process(buy);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out.value().size(), 0); // no trade

    auto cancel_out = engine.cancel({.session = buy.session, .id = buy.id});
    ASSERT_FALSE(cancel_out.has_value());
    EXPECT_EQ(cancel_out.error(), celebrant::RejectReason::UnknownOrder);
}

// market buy fills what it can. Rest is dropped
TEST(Match, MarketPartialThenCancelled) {
    celebrant::Engine engine;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 5,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 0,
        .quantity = 12, // more than the resting 5
        .session = 2,
        .type = celebrant::OrderType::Market,
    };
    auto out = engine.process(buy);
    ASSERT_TRUE(out.has_value());
    ASSERT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].resting.id, sell.id);
    EXPECT_EQ(out.value()[0].quantity, sell.quantity); // fills the 5
    EXPECT_EQ(out.value()[0].resting_remaining, 0);

    auto cancel_out = engine.cancel({.session = buy.session, .id = buy.id});
    ASSERT_FALSE(cancel_out.has_value());
    EXPECT_EQ(cancel_out.error(), celebrant::RejectReason::UnknownOrder);
}

// two sells same price, the one that rested first fills first
TEST(Priority, TimePriorityTest) {
    celebrant::Engine engine;
    celebrant::NewOrder sellA = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    celebrant::NewOrder sellB = {
        .id = 2,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sellA);
    engine.process(sellB);

    celebrant::NewOrder buy = {
        .id = 3,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 10,
        .session = 3,
        .type = celebrant::OrderType::Limit,
    };
    auto out = engine.process(buy);
    ASSERT_TRUE(out.has_value());
    ASSERT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].resting.id, sellA.id);

    auto b_cancel = engine.cancel({.session = sellB.session, .id = sellB.id});
    ASSERT_TRUE(b_cancel.has_value());
    EXPECT_EQ(b_cancel.value(), 10);
}

// better priced order fills first even though it came after
TEST(Priority, PricePriority) {
    celebrant::Engine engine;
    celebrant::NewOrder sellHigh = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 101,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    celebrant::NewOrder sellLow = {
        .id = 2,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sellHigh);
    engine.process(sellLow);

    celebrant::NewOrder buy = {
        .id = 3,
        .side = celebrant::Side::Buy,
        .price = 101,
        .quantity = 10,
        .session = 3,
        .type = celebrant::OrderType::Limit,
    };
    auto out = engine.process(buy);
    ASSERT_TRUE(out.has_value());
    ASSERT_EQ(out.value().size(), 1);
    EXPECT_EQ(out.value()[0].resting.id, sellLow.id);
    EXPECT_EQ(out.value()[0].price, sellLow.price);
}

// cancel a resting order, a second cancel misses it
TEST(Cancel, CancelRestingOrder) {
    celebrant::Engine engine;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sell);

    auto cancel_out = engine.cancel({.session = sell.session, .id = sell.id});
    ASSERT_TRUE(cancel_out.has_value());
    EXPECT_EQ(cancel_out.value(), 10);

    auto second_cancel = engine.cancel({.session = sell.session, .id = sell.id});
    ASSERT_FALSE(second_cancel.has_value());
    EXPECT_EQ(second_cancel.error(), celebrant::RejectReason::UnknownOrder);
}

// cancel the only order at a price -> that whole level is gone
TEST(Cancel, CancelLastOrderErasesLevel) {
    celebrant::Engine engine;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sell);

    auto cancel_out = engine.cancel({.session = sell.session, .id = sell.id});
    ASSERT_TRUE(cancel_out.has_value());

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 10,
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    auto out = engine.process(buy);
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(out.value().size(), 0);
}

// cancel a partly filled order, the remaining is removed
TEST(Cancel, CancelPartiallyFilled) {
    celebrant::Engine engine;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 4,
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(buy);

    auto cancel_out = engine.cancel({.session = sell.session, .id = sell.id});
    ASSERT_TRUE(cancel_out.has_value());
    EXPECT_EQ(cancel_out.value(), 6);
}

// cancel an id that was never placed: unknown order
TEST(Cancel, CancelUnknownId) {
    celebrant::Engine engine;
    auto cancel_out = engine.cancel({.session = 1, .id = 99});
    ASSERT_FALSE(cancel_out.has_value());
    EXPECT_EQ(cancel_out.error(), celebrant::RejectReason::UnknownOrder);
}

// cancel an order that already fully filled: unknown order
TEST(Cancel, CancelFullyFilled) {
    celebrant::Engine engine;
    celebrant::NewOrder sell = {
        .id = 1,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sell);

    celebrant::NewOrder buy = {
        .id = 2,
        .side = celebrant::Side::Buy,
        .price = 100,
        .quantity = 10,
        .session = 2,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(buy);

    auto cancel_out = engine.cancel({.session = sell.session, .id = sell.id});
    ASSERT_FALSE(cancel_out.has_value());
    EXPECT_EQ(cancel_out.error(), celebrant::RejectReason::UnknownOrder);
}

// wrong session cannot cancel someone else's order
TEST(Cancel, CancelWrongSession) {
    celebrant::Engine engine;
    celebrant::NewOrder sell = {
        .id = 5,
        .side = celebrant::Side::Sell,
        .price = 100,
        .quantity = 10,
        .session = 1,
        .type = celebrant::OrderType::Limit,
    };
    engine.process(sell);

    auto wrong = engine.cancel({.session = 2, .id = 5});
    ASSERT_FALSE(wrong.has_value());
    EXPECT_EQ(wrong.error(), celebrant::RejectReason::UnknownOrder);

    auto right = engine.cancel({.session = 1, .id = 5});
    ASSERT_TRUE(right.has_value());
    EXPECT_EQ(right.value(), 10);
}
