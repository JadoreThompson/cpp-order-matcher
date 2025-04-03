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
    const char *m_msg;

public:
    OrderBookEmpty(const char *msg);
    const char *what();
};

class OrderBook
{
private:
    std::map<int, Position> m_tracker;
    std::map<float, std::list<Order *>> m_bids;
    std::map<float, std::list<Order *>> m_asks;
    std::list<Order *> m_trailing_stop_loss_orders;
    float m_last_price;
    float m_price;

    void _update_trailing_stop_loss_orders(float price);

public:
    const std::string m_instrument;

    OrderBook(const std::string instrument, const float price);

    std::map<float, std::list<Order *>> &get_book(const Order &order) const;

    Position &declare(std::shared_ptr<NewOrderPayload> payload);

    Position &track(Order &order);

    void rtrack(Order &order);

    Position &get_position(const int id);

    float get_price();

    void set_price(float price);

    float best_price(NewOrderPayload::Side &&side);

    void push_order(Order &order);

    void remove_from_level(Order &order);

    std::pair<int, int> size();
};

#endif