#ifndef _FUTURES_ENGINE_
#define _FUTURES_ENGINE_

#include "order.h"
#include "orderbook.h"
#include "queue.h"

enum MatchResultType
{
    FAILURE,
    PARTIAL,
    SUCCESS
};

struct MatchResult
{
private:
    MatchResultType result_type;
    bool result_set;

public:
    int price;

    MatchResult(MatchResultType result_type_,
                int price = -1);
    MatchResult();
    void set_result_type(MatchResultType result_type);
    MatchResultType & get_result_type();
};

class FuturesEngine
{
private:
    std::map<const std::string, OrderBook> orderbooks;

public:
    void start(Queue<OrderPayload> &queue);
    void handler(OrderPayload &payload);
    void place_market_order(OrderPayload &payload);
    void place_limit_order(OrderPayload &payload);
    MatchResult match(Order &order, OrderBook &orderbook);
    void handle_filled_orders(std::list<Order> &orders, OrderBook &orderbook, const float price);
    void handle_touched_orders(std::list<Order> &orders, OrderBook &orderbook);
    void place_tp_sl(Order &order, OrderBook &orderbook) const;
};
#endif