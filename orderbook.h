#ifndef _ORDERBOOK_
#define _ORDERBOOK_

#include <exception>
#include <list>
#include <map>
#include <memory>
#include <string>
#include "order.h"

class OrderBookEmpty : public std::exception
{
private:
    const char *msg;

public:
    OrderBookEmpty(const char *msg_);
    const char *what();
};

class OrderBook
{
private:
    std::map<int, Position> tracker;

public:
    float price;
    std::map<float, std::list<Order *>> bids;
    std::map<float, std::list<Order *>> asks;
    const std::string instrument;

    OrderBook(const std::string instrument_, const float price_ = NULL);

    std::map<float, std::list<Order *>> &get_book(const Order &order) const;

    Position &declare(std::shared_ptr<NewOrderPayload> payload);

    Position &track(Order &order);

    void rtrack(Order &order);

    Position &get_position(const int id);

    float best_price(NewOrderPayload::Side &&side);

    void push_order(Order &order);

    void remove_from_level(Order &order);

    std::pair<int, int> size();
};

#endif