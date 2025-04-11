#ifndef _FUTURES_ENGINE_
#define _FUTURES_ENGINE_

#include <tuple>
#include <functional>
#include "order.h"
#include "orderbook.h"
#include "queue.h"

const int LOOPS = 10000;

struct MatchResult
{
    enum MatchResultType
    {
        FAILURE,
        PARTIAL,
        SUCCESS
    };

private:
    bool m_result_set;

public:
    MatchResultType m_result_type;
    int m_price;

    MatchResult(const MatchResultType result_type,
                int price = -1);

    MatchResult();

    void set_result_type(const MatchResultType result_type);

    const MatchResultType &get_result_type() const;
};

class FuturesEngine
{
private:
    std::map<const std::string, OrderBook> orderbooks;

public:
    void start(Queue &queue);

    void place_gtc_market_order(std::shared_ptr<OrderPayload> &payload);

    void place_fok_market_order(std::shared_ptr<OrderPayload> &payload);

    void place_limit_order(std::shared_ptr<OrderPayload> &payload);

    void cancel_order(std::shared_ptr<BasePayload> &payload);

    void modify_order(std::shared_ptr<ModifyOrderPayload> &&payload);

    void close_order(std::shared_ptr<BasePayload> &payload);

    const MatchResult match_gtc(Order &order, OrderBook &orderbook);

    const MatchResult match_fok(Order *&order, OrderBook &orderbook);

    const MatchResult gen_match_result(const float og_standing_quantity, Order &order, const float price);

    void handle_filled_orders(std::vector<std::pair<Order *, int>> &orders, OrderBook &orderbook, const float price);

    void handle_touched_orders(std::vector<std::pair<Order *, int>> &orders, OrderBook &orderbook, const float price);

    void place_tp_sl(Order &order, OrderBook &orderbook);
};
#endif