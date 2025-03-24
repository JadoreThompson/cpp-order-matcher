#ifndef _FUTURES_ENGINE_
#define _FUTURES_ENGINE_

#include "order.h"
#include "orderbook.h"

class FuturesEngine
{
public:
    void match(Order &order, OrderBook &orderbook);
    void handle_filled_orders(std::list<Order> &orders, OrderBook &orderbook);
    void place_tp_sl(Order &order, OrderBook &orderbook);
};
#endif