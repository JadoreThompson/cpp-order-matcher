#ifndef _ORDERBOOK_
#define _ORDERBOOK_

#include <exception>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include "engine/order.h"


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

    std::map<float, std::list<std::shared_ptr<Order>>> &get_book(const Order &order) const noexcept;

    Position &declare(const std::shared_ptr<OrderPayload> payload) noexcept;

    void track(std::shared_ptr<Order> order) const;

    void rtrack(std::shared_ptr<Order> &order) noexcept;

    Position &get_position(const int id) const noexcept;

    const float get_price() const noexcept;

    void set_price(const float price);

    const float get_best_price(Side side) const noexcept;

    void push_order(const std::shared_ptr<Order> &order) noexcept;

    void remove_from_level(const std::shared_ptr<Order> &order) noexcept;

    const std::pair<int, int> size() const noexcept;

    void print_size(const std::map<float, const std::list<std::shared_ptr<Order>>> &bids) const noexcept;


private:
    void update_trailing_stop_loss_orders(const float price) noexcept;

    std::map<float, std::list<std::shared_ptr<Order>>> m_bids;
    std::map<float, std::list<std::shared_ptr<Order>>> m_asks;
    std::map<int, Position> m_tracker;
    std::unordered_set<std::shared_ptr<Order>> m_trailing_stop_loss_orders;
    float m_last_price;
    float m_price;
};

#endif