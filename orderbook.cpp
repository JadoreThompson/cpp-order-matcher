#include <thread>
#include "iostream"
#include "orderbook.h"
#include "order.h"

OrderBookEmpty::OrderBookEmpty(const char *msg) : m_msg(msg) {};

const char *OrderBookEmpty::what()
{
    return this->m_msg;
}

OrderBook::OrderBook(const std::string instrument, const float price)
    : m_instrument(instrument), m_price(price), m_last_price(price) {};

// Returns the book that the order should be matched against.
// std::map<float, std::list<Order *>> &OrderBook::get_book(const Order &order)
// std::map<float, std::list<std::shared_ptr<Order> &>> &OrderBook::get_book(const Order &order)
// std::map<float, std::list<std::reference_wrapper<std::shared_ptr<Order>>>> &OrderBook::get_book(const Order &order)
std::map<float, std::list<std::shared_ptr<Order>>> &OrderBook::get_book(const Order &order)
{
    if (order.m_tag == Tag::ENTRY)
    {
        return (order.m_payload->m_side == Side::ASK) ? this->m_bids : this->m_asks;
    }

    return (order.m_payload->m_side == Side::ASK) ? this->m_bids : this->m_asks;
}

Position &OrderBook::get_position(const int id)
{
    return this->m_tracker.at(id);
}

// Specifically used for ENTRY orders.
Position &OrderBook::declare(std::shared_ptr<OrderPayload> payload)
{
    return this->m_tracker.emplace(payload->m_id, std::make_shared<Order>(Tag::ENTRY, payload)).first->second;
}

// Used for STOP_LOSS and TAKE_PROFIT orders.
void OrderBook::track(std::shared_ptr<Order> order)
{
    Position &position = this->m_tracker.at(order->m_payload->m_id);

    if (order->m_tag == Tag::ENTRY)
    {
        throw std::string("Position already exists");
    }

    if (order->m_tag == Tag::TAKE_PROFIT)
    {
        position.m_take_profit_order = order;
    }
    else
    {
        position.m_stop_loss_order = order;
    }
}

/*
 * Calling this will invalidate the pointers
 * within the bids, asks. Ensure you remove
 * the order from the level before calling this.
 */
void OrderBook::rtrack(std::shared_ptr<Order> &order)
{
    Position &position = this->m_tracker.at(order->m_payload->m_id);

    if (order->m_tag == Tag::STOP_LOSS)
    {
        position.m_stop_loss_order = nullptr;
    }
    else if (order->m_tag == Tag::TAKE_PROFIT)
    {
        position.m_take_profit_order = nullptr;
    }
    else
    {
        if (position.m_take_profit_order)
        {
            remove_from_level(position.m_take_profit_order);
        }

        if (position.m_stop_loss_order)
        {
            remove_from_level(position.m_stop_loss_order);
        }

        if (order->m_payload->m_status == Status::PENDING)
        {
            remove_from_level(order);
        }

        this->m_tracker.erase(order->m_payload->m_id);
    }
}

float OrderBook::get_price() { return this->m_price; }

void OrderBook::set_price(float price)
{
    this->m_price = price;
    _update_trailing_stop_loss_orders(price);
    this->m_last_price = price;
}

float OrderBook::get_best_price(Side &&side)
{
    float best_price;

    if (side == Side::ASK)
    {
        best_price = this->m_asks.begin()->first;
        return (best_price) ? best_price : this->m_price;
    }

    return (this->m_bids.empty()) ? this->m_price : this->m_bids.begin()->first;
}

void OrderBook::push_order(std::shared_ptr<Order> &order)
{
    // if (this->counter % 10 == 0)
    // {
    //     // std::cout << "Current size: " << this->m_bids.size();
    //     std::cout
    //         << "\n Bids: " << this->m_bids.size()
    //         << " Asks " << this->m_asks.size()
    //         << std::endl;

    //     std::this_thread::sleep_for(std::chrono::seconds(3));
    // }

    // this->counter++;

    auto &book = get_book(*order);
    float price;

    if (order->m_tag == Tag::ENTRY)
    {
        price = order->m_payload->m_entry_price;
    }
    else
    {
        if (order->m_tag == Tag::TAKE_PROFIT)
        {
            price = order->m_payload->m_take_profit_price;
        }
        else
        {
            if (order->m_payload->m_side == Side::BID)
            {
                // price = this->m_price - order.m_payload->m_stop_loss_order->m_distance;
                price = this->m_price - order->m_payload->m_stop_loss_order.m_distance;
            }
            else
            {
                // price = this->m_price + order.m_payload->m_stop_loss_order->m_distance;
                price = this->m_price - order->m_payload->m_stop_loss_order.m_distance;
            }

            // this->m_trailing_stop_loss_orders.push_back(order);
            this->m_trailing_stop_loss_orders.push_back(order);
        }
    }

    // book[price].push_back(order);
    book[price].push_back(order);
}

// void OrderBook::remove_from_level(Order &order)
void OrderBook::remove_from_level(std::shared_ptr<Order> &order)
{
    auto &book = get_book(*order);
    float price;

    if (order->m_tag == Tag::ENTRY)
    {
        price = order->m_payload->m_entry_price;
    }
    else
    {

        if (order->m_tag == Tag::TAKE_PROFIT)
        {
            price = order->m_payload->m_take_profit_price;
        }
        else
        {
            // if (order.m_payload->m_side == Side::BID)
            // {
            //     price = this->m_last_price - order.m_payload->m_stop_loss_order->m_distance;
            // }
            // else
            // {
            //     price = this->m_last_price + order.m_payload->m_stop_loss_order->m_distance;
            // }

            this->m_trailing_stop_loss_orders.remove(order);
        }
    }

    book[price].remove(order);
}

std::pair<int, int> OrderBook::size()
{
    return std::pair<int, int>{this->m_bids.size(), this->m_asks.size()};
}

void OrderBook::_update_trailing_stop_loss_orders(float price)
{
    for (std::shared_ptr<Order> &order : this->m_trailing_stop_loss_orders)
    {
        remove_from_level(order);
        push_order(order);
    }
}
