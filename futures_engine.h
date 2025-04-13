#ifndef _FUTURES_ENGINE_
#define _FUTURES_ENGINE_

#include <tuple>
#include "order.h"
#include "orderbook.h"
#include "queue.h"

constexpr int LOOPS = 1e6;

struct MatchResult
{
    enum MatchResultType
    {
        FAILURE,
        PARTIAL,
        SUCCESS
    };

public:
    MatchResultType m_result_type;
    int m_price;

    MatchResult(const MatchResultType result_type,
                int price = -1);

    MatchResult();

    // private:
    //     bool m_result_set;
};

class FuturesEngine
{
public:
    void start(Queue &queue) noexcept;

    void handle_new_order(std::unique_ptr<BasePayload> message_payload) noexcept;

    void handle_cancel_order(std::unique_ptr<BasePayload> message_payload) noexcept;

    void handle_modify_order(std::unique_ptr<BasePayload> message_payload) noexcept;

    void handle_close_order(std::unique_ptr<BasePayload> message_payload) noexcept;

    void place_fok_market_order(std::shared_ptr<OrderPayload> &&payloadp) noexcept;

    void place_gtc_market_order(std::shared_ptr<OrderPayload> payloadp) noexcept;

    void place_limit_order(std::shared_ptr<OrderPayload> &&payloadp) noexcept;

    void cancel_order(const std::unique_ptr<BasePayload> payload);

    void modify_order(const ModifyOrderPayload &payload) noexcept;

    void close_order(const std::unique_ptr<BasePayload> &payload) noexcept;

    const MatchResult match_fok(Order &order, OrderBook &orderbook) noexcept;

    const MatchResult match_gtc(Order &order, OrderBook &orderbook) noexcept;

    void handle_filled_orders(std::vector<std::pair<std::shared_ptr<Order> &, int>> &orders, OrderBook &orderbook, const float price) noexcept;

    void handle_touched_orders(std::vector<std::pair<std::shared_ptr<Order> &, int>> &orders, OrderBook &orderbook, const float price) noexcept;

    void place_tp_sl(Order &order, OrderBook &orderbook) noexcept;

private:
    const MatchResult gen_match_result(const float og_standing_quantity, Order &order, const float price) noexcept;

    void modify_entry(const ModifyOrderPayload &payload, Position &position, OrderBook &orderbook);

    void modify_stop_loss(const ModifyOrderPayload &payload, Position &position, OrderBook &orderbook) noexcept;

    void modify_take_profit(const ModifyOrderPayload &payload, Position &position, OrderBook &orderbook);

    std::map<const std::string, OrderBook> orderbooks;
};
#endif