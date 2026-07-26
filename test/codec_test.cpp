#include <optional>
#include <string>
#include <variant>

#include <gtest/gtest.h>

#include "celebrant/codec.hpp"
#include "celebrant/types.hpp"

using namespace celebrant;

// ---- decode: well-formed (happy path) ----

// NEW limit: symbol kept, price converts 100.50 -> 10050 ticks
TEST(CodecDecode, NewLimit) {
    auto result = decode("NEW,1,AAPL,BUY,LIMIT,25,100.50", 7);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<NewOrder>(result.value()));
    NewOrder expected{.id = 1,
                      .side = Side::Buy,
                      .price = 10050,
                      .quantity = 25,
                      .session = 7,
                      .type = OrderType::Limit,
                      .symbol = "AAPL"};
    EXPECT_EQ(std::get<NewOrder>(result.value()), expected);
}

// bare integer price 100 -> 10000 ticks (the x100 conversion, no decimal)
TEST(CodecDecode, NewLimitIntegerPrice) {
    auto result = decode("NEW,3,AAPL,BUY,LIMIT,25,100", 7);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(std::get<NewOrder>(result.value()).price, 10000);
}

// NEW market: no price field, price stays 0
TEST(CodecDecode, NewMarket) {
    auto result = decode("NEW,2,AAPL,SELL,MARKET,10", 7);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<NewOrder>(result.value()));
    const auto& order = std::get<NewOrder>(result.value());
    EXPECT_EQ(order.type, OrderType::Market);
    EXPECT_EQ(order.price, 0);
    EXPECT_EQ(order.quantity, 10);
    EXPECT_EQ(order.side, Side::Sell);
}

// CANCEL: session-qualified key
TEST(CodecDecode, Cancel) {
    auto result = decode("CANCEL,5", 7);
    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(std::holds_alternative<OrderKey>(result.value()));
    const auto& key = std::get<OrderKey>(result.value());
    EXPECT_EQ(key.session, 7u);
    EXPECT_EQ(key.id, 5u);
}

// ---- decode: malformed, one per ParseRejectReason ----

static void expect_reject(std::string_view line, ParseRejectReason reason,
                          std::optional<OrderId> id) {
    auto result = decode(line, 7);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().reason, reason);
    EXPECT_EQ(result.error().id, id);
}

TEST(CodecReject, UnknownMessageType) {
    expect_reject("FOO,1", ParseRejectReason::UnknownMessageType, std::nullopt);
}

TEST(CodecReject, WrongFieldCountLimit) { // limit needs 7 fields, has 6
    expect_reject("NEW,1,AAPL,BUY,LIMIT,25", ParseRejectReason::WrongFieldCount, 1);
}

TEST(CodecReject, WrongFieldCountMarket) { // market needs 6 fields, has 7
    expect_reject("NEW,1,AAPL,BUY,MARKET,25,100.50", ParseRejectReason::WrongFieldCount, 1);
}

TEST(CodecReject, WrongFieldCountCancel) { // cancel needs exactly 2
    expect_reject("CANCEL,5,extra", ParseRejectReason::WrongFieldCount, 5);
}

TEST(CodecReject, NonNumericId) { // id unparseable -> no id to echo
    expect_reject("NEW,abc,AAPL,BUY,LIMIT,25,100.50", ParseRejectReason::NonNumericField,
                  std::nullopt);
}

TEST(CodecReject, NonNumericQuantity) {
    expect_reject("NEW,1,AAPL,BUY,LIMIT,xx,100.50", ParseRejectReason::NonNumericField, 1);
}

TEST(CodecReject, NumberOutOfRange) { // id far past uint64
    expect_reject("NEW,99999999999999999999,AAPL,BUY,LIMIT,25,100.50",
                  ParseRejectReason::NumberOutOfRange, std::nullopt);
}

TEST(CodecReject, UnknownSide) {
    expect_reject("NEW,1,AAPL,XYZ,LIMIT,25,100.50", ParseRejectReason::UnknownSide, 1);
}

TEST(CodecReject, UnknownOrderType) {
    expect_reject("NEW,1,AAPL,BUY,FOO,25,100.50", ParseRejectReason::UnknownOrderType, 1);
}

TEST(CodecReject, ExcessPricePrecision) { // 3 decimals -> sub-tick
    expect_reject("NEW,1,AAPL,BUY,LIMIT,25,100.501", ParseRejectReason::ExcessPricePrecision, 1);
}

// ---- encode ----

TEST(CodecEncode, Ack) { EXPECT_EQ(encode(Ack{.id = 5}), "ACK,5\n"); }

TEST(CodecEncode, CancelConfirm) { EXPECT_EQ(encode(CancelConfirm{.id = 5}), "CXL,5\n"); }

TEST(CodecEncode, Fill) {
    Fill fill{.id = 5,
              .symbol = "AAPL",
              .side = Side::Buy,
              .qty_filled = 100,
              .price = 10050,
              .qty_remaining = 50};
    EXPECT_EQ(encode(fill), "FILL,5,AAPL,BUY,100,100.50,50\n");
}

// price fraction zero-pads: 10005 -> "100.05", not "100.5"
TEST(CodecEncode, FillPricePadding) {
    Fill fill{.id = 5,
              .symbol = "AAPL",
              .side = Side::Buy,
              .qty_filled = 100,
              .price = 10005,
              .qty_remaining = 0};
    EXPECT_EQ(encode(fill), "FILL,5,AAPL,BUY,100,100.05,0\n");
}

TEST(CodecEncode, TradePrint) {
    TradePrint trade{.symbol = "AAPL", .quantity = 100, .price = 10050, .seq = 42};
    EXPECT_EQ(encode(trade), "TRADE,AAPL,100,100.50,42\n");
}

TEST(CodecEncode, BookUpdate) {
    BookUpdate book{
        .symbol = "AAPL", .side = Side::Buy, .price = 10050, .aggregate_qty = 250, .seq = 43};
    EXPECT_EQ(encode(book), "BOOK,AAPL,BUY,100.50,250,43\n");
}

TEST(CodecEncode, RejectWithId) {
    Reject reject{.id = 5, .reason = RejectReason::UnknownOrder};
    EXPECT_EQ(encode(reject), "REJECT,5,UnknownOrder\n");
}

// line too broken to yield an id -> empty id field
TEST(CodecEncode, RejectNoId) {
    Reject reject{.id = std::nullopt, .reason = ParseRejectReason::UnknownMessageType};
    EXPECT_EQ(encode(reject), "REJECT,,UnknownMessageType\n");
}
