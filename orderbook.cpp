#include "orderbook.h"
#include "order.h"
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

OrderBook::OrderBook(const std::string instrument_) : instrument(instrument_) {};

// Returns the book that the order should be matched against.
std::map<float, std::list<Order *>> &OrderBook::get_book(const Order &order) const
{
    if (order.tag == ENTRY)
    {
        return const_cast<std::map<float, std::list<Order *>> &>((order.payload.side == ASK) ? this->bids : this->asks);
    }

    return const_cast<std::map<float, std::list<Order *>> &>((order.payload.side == ASK) ? this->bids : this->asks);
}

Position &OrderBook::get_position(const int id)
{
    try
    {
        return this->tracker.at(id);
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Position with id " + std::to_string(id) + " doesn't exist");
    }
}

// Specifically used for ENTRY orders.
Position &OrderBook::declare(OrderPayload &payload)
{
    if (this->tracker.find(payload.id) != this->tracker.end())
    {
        throw std::string("Position already exists");
    }

    this->tracker.emplace(payload.id, Position(new Order(payload, ENTRY)));
    return this->tracker.at(payload.id);
}

// Used for STOP_LOSS and TAKE_PROFIT orders.
Position &OrderBook::track(Order &order)
{
    try
    {
        Position &position = this->tracker.at(order.payload.id);
        std::cout << "Tracking order with id " << std::to_string(order.payload.id) << std::endl;

        if (order.tag == ENTRY)
        {
            throw std::string("Position already exists");
        }

        if (order.tag == TAKE_PROFIT)
        {
            position.take_profit_order = &order;
        }
        else
        {
            position.stop_loss_order = &order;
        }

        return position;
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Cannot add " + std::to_string(order.tag) + " to tracker without an existing position");
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
        Position &position = this->tracker.at(order.payload.id);

        if (order.tag == STOP_LOSS)
        {
            position.stop_loss_order = nullptr;
        }
        else if (order.tag == TAKE_PROFIT)
        {
            position.take_profit_order = nullptr;
        }
        else
        {
            if (position.take_profit_order)
            {
                remove_from_level(*position.take_profit_order);
                delete position.take_profit_order;
            }

            if (position.stop_loss_order)
            {
                remove_from_level(*position.stop_loss_order);
                delete position.stop_loss_order;
            }

            if (order.payload.get_status() == (PENDING || PARTIALLY_FILLED))
            {
                remove_from_level(order);
                delete position.entry_order;
            }
            
            this->tracker.erase(order.payload.id);
        }
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Position for order id " + std::to_string(order.payload.id) + " doesn't exist");
    }
}

void OrderBook::remove_from_level(Order &order)
{
    auto &book = get_book(order);

    if (order.tag == ENTRY)
    {
        book[order.payload.entry_price].remove(&order);
    }
    else
    {
        book[*((order.tag == TAKE_PROFIT)
                   ? order.payload.take_profit_price
                   : order.payload.stop_loss_price)]
            .remove(&order);
    }
}

void OrderBook::push_order(Order &order)
{
    auto &book = get_book(order);

    if (order.tag == ENTRY)
    {
        book[order.payload.entry_price].push_back(&order);
    }
    else
    {
        const float *price =
            (order.tag == TAKE_PROFIT)
                ? order.payload.take_profit_price
                : order.payload.stop_loss_price;
        book[*price].push_back(&order);
    }
}

std::pair<int, int> OrderBook::size()
{
    return std::pair<int, int>{this->bids.size(), this->asks.size()};
}