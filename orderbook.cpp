#include <iostream>
#include <stdexcept>
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
std::map<float, std::list<Order *>> &OrderBook::get_book(const Order &order) const
{
    if (order.m_tag == Order::Tag::ENTRY)
    {
        return const_cast<std::map<float, std::list<Order *>> &>((order.m_payload->m_side == NewOrderPayload::Side::ASK) ? this->m_bids : this->m_asks);
    }

    return const_cast<std::map<float, std::list<Order *>> &>((order.m_payload->m_side == NewOrderPayload::Side::ASK) ? this->m_bids : this->m_asks);
}

Position &OrderBook::get_position(const int id)
{
    try
    {
        return this->m_tracker.at(id);
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Position with id " + std::to_string(id) + " doesn't exist");
    }
}

// Specifically used for ENTRY orders.
Position &OrderBook::declare(std::shared_ptr<NewOrderPayload> payload)
{
    if (this->m_tracker.find(payload->m_id) != this->m_tracker.end())
    {
        throw std::string("Position already exists");
    }

    this->m_tracker.emplace(payload->m_id, new Order(payload, Order::Tag::ENTRY));
    return this->m_tracker.at(payload->m_id);
}

// Used for STOP_LOSS and TAKE_PROFIT orders.
Position &OrderBook::track(Order &order)
{
    try
    {
        Position &position = this->m_tracker.at(order.m_payload->m_id);

        if (order.m_tag == Order::Tag::ENTRY)
        {
            throw std::string("Position already exists");
        }

        if (order.m_tag == Order::Tag::TAKE_PROFIT)
        {
            position.m_take_profit_order = &order;
        }
        else
        {
            position.m_stop_loss_order = &order;
        }

        return position;
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Cannot add " + std::to_string(order.m_tag) + " to tracker without an existing position");
    }
}

/*
 * Calling this will invalidate the pointers
 * within the bids, asks. Ensure you remove
 * the order from the level before calling this.
 */
void OrderBook::rtrack(Order &order)
{
    try
    {
        Position &position = this->m_tracker.at(order.m_payload->m_id);

        if (order.m_tag == Order::Tag::STOP_LOSS)
        {
            position.m_stop_loss_order = nullptr;
        }
        else if (order.m_tag == Order::Tag::TAKE_PROFIT)
        {
            position.m_take_profit_order = nullptr;
        }
        else
        {
            if (position.m_take_profit_order)
            {
                remove_from_level(*position.m_take_profit_order);
                delete position.m_take_profit_order;
            }

            if (position.m_stop_loss_order)
            {
                remove_from_level(*position.m_stop_loss_order);
                delete position.m_stop_loss_order;
            }

            if (order.m_payload->get_status() == NewOrderPayload::Status::PENDING)
            {
                remove_from_level(order);
                delete position.m_entry_order;
            }

            this->m_tracker.erase(order.m_payload->m_id);
        }
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Position for order id " + std::to_string(order.m_payload->m_id) + " doesn't exist");
    }
}

float OrderBook::get_price() { return this->m_price; }

void OrderBook::set_price(float price)
{
    this->m_price = price;
    _update_trailing_stop_loss_orders(price);
    this->m_last_price = price;
}

float OrderBook::best_price(NewOrderPayload::Side &&side)
{
    if (side == NewOrderPayload::Side::ASK)
    {
        float best_price = this->m_asks.begin()->first;
        if (best_price)
        {
            return this->m_price;
        }

        return best_price;
    }

    float best_price = this->m_bids.rbegin()->first;
    if (best_price)
    {
        return this->m_price;
    }

    return best_price;
}

void OrderBook::push_order(Order &order)
{
    auto &book = get_book(order);
    float price;

    if (order.m_tag == Order::Tag::ENTRY)
    {
        // book[order.m_payload->m_entry_price].push_back(&order);
        price = order.m_payload->m_entry_price;
    }
    else
    {
        if (order.m_tag == Order::Tag::TAKE_PROFIT)
        {
            price = order.m_payload->m_take_profit_price;
        }
        else
        {
            if (order.m_payload->m_side == NewOrderPayload::Side::BID)
            {
                price = this->m_price - order.m_payload->m_stop_loss_order->m_distance;
            }
            else
            {
                price = this->m_price + order.m_payload->m_stop_loss_order->m_distance;
            }

            this->m_trailing_stop_loss_orders.push_back(&order);
        }
    }

    book[price].push_back(&order);
}

void OrderBook::remove_from_level(Order &order)
{
    auto &book = get_book(order);
    float price;

    if (order.m_tag == Order::Tag::ENTRY)
    {
        // book[order.m_payload->m_entry_price].remove(&order);
        price = order.m_payload->m_entry_price;
    }
    else
    {

        if (order.m_tag == Order::Tag::TAKE_PROFIT)
        {
            price = order.m_payload->m_take_profit_price;
        }
        else
        {
            if (order.m_payload->m_side == NewOrderPayload::Side::BID)
            {
                price = this->m_last_price - order.m_payload->m_stop_loss_order->m_distance;
            }
            else
            {
                price = this->m_last_price + order.m_payload->m_stop_loss_order->m_distance;
            }

            this->m_trailing_stop_loss_orders.remove(&order);
        }

        // book[*((order.m_tag == Order::Tag::TAKE_PROFIT)
        //            ? order.m_payload->take_profit_price
        //            : order.m_payload->stop_loss_price)]
        //     .remove(&order);
    }

    book[price].remove(&order);
}

std::pair<int, int> OrderBook::size()
{
    return std::pair<int, int>{this->m_bids.size(), this->m_asks.size()};
}

void OrderBook::_update_trailing_stop_loss_orders(float price)
{
    for (Order *&order : this->m_trailing_stop_loss_orders)
    {
        remove_from_level(*order);
        push_order(*order);
    }
}
