#ifndef _ORDERBOOK_
#define _ORDERBOOK_

#include <map>
#include <list>
#include <string>
#include "order.h"

enum RemoveType
{
    ALL,
    INDIVIDUAL
};

class OrderBook
{
private:
    std::map<float, std::list<Order>> bids;
    std::map<float, std::list<Order>> asks;
    std::map<int, Position> tracker;
    void push_tp_sl(Order &order);

public:
    const std::string instrument;
    OrderBook(const std::string instrument);
    std::map<float, std::list<Order>> &get_book(const Order &order) const;
    Position &track(Order &order);
    void rtrack(Order &order);
    Position &get_position(const int id);
    void remove_from_level(Order &order);
    void push_order(Order &order);
    std::pair<int, int> size();
};
#endif