#ifndef _ORDERBOOK_
#define _ORDERBOOK_

#include <exception>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "order.h"

class OrderBookEmpty : public std::exception
{
public:
    OrderBookEmpty(const char *msg);
    const char *what();

private:
    const char *m_msg;
};

class OrderBook
{
public:
    const std::string m_instrument;

    OrderBook(const std::string instrument, const float price);

    std::map<float, std::list<std::shared_ptr<Order>>> &get_book(const Order &order);

    Position &declare(std::shared_ptr<OrderPayload> payload);

    void track(std::shared_ptr<Order> order);

    void rtrack(std::shared_ptr<Order> &order);

    Position &get_position(const int id);

    const float get_price() noexcept;

    void set_price(float price);

    const float get_best_price(Side side) noexcept;

    void push_order(std::shared_ptr<Order> &order);

    void remove_from_level(std::shared_ptr<Order> &order);

    std::pair<int, int> size() noexcept;

    void print_size(const std::map<float, std::list<std::shared_ptr<Order>>> &bids);

    std::map<float, std::list<std::shared_ptr<Order>>> m_bids;
    std::map<float, std::list<std::shared_ptr<Order>>> m_asks;
    std::map<int, Position> m_tracker;

private:
    void update_trailing_stop_loss_orders(float price) noexcept;

    std::unordered_set<std::shared_ptr<Order>> m_trailing_stop_loss_orders;
    float m_last_price;
    float m_price;
};

#endif