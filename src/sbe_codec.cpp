#include "celebrant/sbe_codec.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include "celebrant/expected.hpp"
#include "celebrant/types.hpp"

// generated SBE
#include "celebrant_sbe/ExecutionReport.h"
#include "celebrant_sbe/MessageHeader.h"
#include "celebrant_sbe/NewOrderSingle.h"
#include "celebrant_sbe/OrderCancelRequest.h"

namespace celebrant::sbe_codec {

Expected<Side, ParseRejectReason> parse_side(char raw) {
    switch (raw) {
    case '1':
        return Side::Buy;
    case '2':
        return Side::Sell;
    default:
        return ParseRejectReason::UnknownSide;
    }
}

Expected<OrderType, ParseRejectReason> parse_order_type(char raw) {
    switch (raw) {
    case '1':
        return OrderType::Market;
    case '2':
        return OrderType::Limit;
    default:
        return ParseRejectReason::UnknownOrderType;
    }
}

sbe::Side::Value wire_side(Side side) {
    return side == Side::Buy ? sbe::Side::Buy : sbe::Side::Sell;
}

sbe::OrdRejReason::Value wire_reject_reason(RejectReason r) {
    switch (r) {
    case RejectReason::UnknownSymbol:
        return sbe::OrdRejReason::UnknownSymbol;
    case RejectReason::UnknownOrder:
        return sbe::OrdRejReason::UnknownOrder;
    case RejectReason::InvalidQuantity:
        return sbe::OrdRejReason::InvalidQuantity;
    case RejectReason::InvalidPrice:
        return sbe::OrdRejReason::Other;
    }
    return sbe::OrdRejReason::Other;
}

sbe::OrdRejReason::Value wire_reject_reason(ParseRejectReason reason) {
    return sbe::OrdRejReason::Other;
}

DecodeOutcome decode_new_order(char* buffer, std::size_t len, std::uint16_t block_length,
                               std::uint16_t version, SessionId session) {
    sbe::NewOrderSingle msg;
    msg.wrapForDecode(buffer, sbe::MessageHeader::encodedLength(), block_length, version, len);

    OrderId id = msg.clOrdID();

    auto side = parse_side(msg.sideRaw());
    if (!side.has_value()) {
        return ParseError{.reason = side.error(), .id = id};
    }

    auto type = parse_order_type(msg.ordTypeRaw());
    if (!type.has_value()) {
        return ParseError{.reason = type.error(), .id = id};
    }

    Price price = (type.value() == OrderType::Market) ? 0 : msg.price();

    NewOrder new_order{.id = id,
                       .side = side.value(),
                       .price = price,
                       .quantity = msg.orderQty(),
                       .session = session,
                       .type = type.value(),
                       .symbol = Symbol(msg.getSymbolAsStringView())};
    return std::variant<NewOrder, OrderKey>{new_order};
}

DecodeOutcome decode_cancel(char* buffer, std::size_t len, std::uint16_t block_length,
                            std::uint16_t version, SessionId session) {
    sbe::OrderCancelRequest msg;
    msg.wrapForDecode(buffer, sbe::MessageHeader::encodedLength(), block_length, version, len);

    OrderKey key{.session = session, .id = msg.origClOrdID()};
    return std::variant<NewOrder, OrderKey>{key};
}

std::optional<std::size_t> frame_length(char* buffer, std::size_t available) {
    if (available < sbe::MessageHeader::encodedLength()) {
        return std::nullopt; // header didn't arrive fully
    }
    sbe::MessageHeader header(buffer, available);
    return sbe::MessageHeader::encodedLength() + header.blockLength();
}

DecodeOutcome decode(char* buffer, std::size_t len, SessionId session) {
    sbe::MessageHeader header(buffer, len);
    const std::uint16_t template_id = header.templateId();
    const std::uint16_t block_length = header.blockLength();
    const std::uint16_t version = header.version();

    switch (template_id) {
    case sbe::NewOrderSingle::SBE_TEMPLATE_ID:
        return decode_new_order(buffer, len, block_length, version, session);
    case sbe::OrderCancelRequest::SBE_TEMPLATE_ID:
        return decode_cancel(buffer, len, block_length, version, session);
    default:
        return ParseError{.reason = ParseRejectReason::UnknownMessageType, .id = std::nullopt};
    }
}

std::string encode(const Ack& ack) {
    std::string buf(sbe::ExecutionReport::sbeBlockAndHeaderLength(), '\0');
    sbe::ExecutionReport report;
    report.wrapAndApplyHeader(buf.data(), 0, buf.size());
    report.clOrdID(ack.id);
    report.lastPx(0);
    report.lastQty(0);
    report.ordStatus(sbe::OrdStatus::New);
    report.side(wire_side(ack.side));
    report.putSymbol(std::string_view(ack.symbol.data.data(), ack.symbol.data.size()));
    report.ordRejReason(sbe::OrdRejReason::NULL_VALUE);
    report.execType(sbe::ExecType::New);
    report.leavesQty(ack.leaves_qty);
    return buf;
}

std::string encode(const Fill& fill) {
    std::string buf(sbe::ExecutionReport::sbeBlockAndHeaderLength(), '\0');
    sbe::ExecutionReport report;
    report.wrapAndApplyHeader(buf.data(), 0, buf.size());
    report.clOrdID(fill.id);
    report.lastPx(fill.price);
    report.lastQty(fill.qty_filled);
    report.ordStatus(fill.qty_remaining == 0 ? sbe::OrdStatus::Filled
                                             : sbe::OrdStatus::PartiallyFilled);
    report.side(wire_side(fill.side));
    report.putSymbol(std::string_view(fill.symbol.data.data(), fill.symbol.data.size()));
    report.ordRejReason(sbe::OrdRejReason::NULL_VALUE);
    report.execType(sbe::ExecType::Trade);
    report.leavesQty(fill.qty_remaining);
    return buf;
}

std::string encode(const CancelConfirm& cancel_confirm) {
    std::string buf(sbe::ExecutionReport::sbeBlockAndHeaderLength(), '\0');
    sbe::ExecutionReport report;
    report.wrapAndApplyHeader(buf.data(), 0, buf.size());
    report.clOrdID(cancel_confirm.id);
    report.lastPx(0);
    report.lastQty(0);
    report.ordStatus(sbe::OrdStatus::Canceled);
    report.side(sbe::Side::NULL_VALUE);
    report.putSymbol(std::string_view());
    report.ordRejReason(sbe::OrdRejReason::NULL_VALUE);
    report.execType(sbe::ExecType::Canceled);
    report.leavesQty(0);
    return buf;
}

std::string encode(const Reject& reject) {
    std::string buf(sbe::ExecutionReport::sbeBlockAndHeaderLength(), '\0');
    sbe::ExecutionReport report;
    report.wrapAndApplyHeader(buf.data(), 0, buf.size());
    sbe::OrdRejReason::Value reason =
        std::visit([](auto r) { return wire_reject_reason(r); }, reject.reason);
    report.clOrdID(reject.id.value_or(0));
    report.lastPx(0);
    report.lastQty(0);
    report.ordStatus(sbe::OrdStatus::Rejected);
    report.side(sbe::Side::NULL_VALUE);
    report.putSymbol(std::string_view());
    report.ordRejReason(reason);
    report.execType(sbe::ExecType::Rejected);
    report.leavesQty(0);
    return buf;
}

} // namespace celebrant::sbe_codec
