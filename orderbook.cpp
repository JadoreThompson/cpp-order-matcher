#include "orderbook.h"
#include "order.h"
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

OrderBookEmpty::OrderBookEmpty(const char *msg_) : msg(msg_) {};

const char *OrderBookEmpty::what()
{
    return this->msg;
}

OrderBook::OrderBook(const std::string instrument_, const float price_) : instrument(instrument_), price(price_) {};

// Returns the book that the order should be matched against.
std::map<float, std::list<Order *>> &OrderBook::get_book(const Order &order) const
{
    if (order.tag == Order::Tag::ENTRY)
    {
        return const_cast<std::map<float, std::list<Order *>> &>((order.payload->side == NewOrderPayload::Side::ASK) ? this->bids : this->asks);
    }

    return const_cast<std::map<float, std::list<Order *>> &>((order.payload->side == NewOrderPayload::Side::ASK) ? this->bids : this->asks);
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
Position &OrderBook::declare(std::shared_ptr<NewOrderPayload> payload)
{
    if (this->tracker.find(payload->id) != this->tracker.end())
    {
        throw std::string("Position already exists");
    }

    this->tracker.emplace(payload->id, new Order(payload, Order::Tag::ENTRY));
    return this->tracker.at(payload->id);
}

// Used for STOP_LOSS and TAKE_PROFIT orders.
Position &OrderBook::track(Order &order)
{
    try
    {
        Position &position = this->tracker.at(order.payload->id);

        if (order.tag == Order::Tag::ENTRY)
        {
            throw std::string("Position already exists");
        }

        if (order.tag == Order::Tag::TAKE_PROFIT)
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
        Position &position = this->tracker.at(order.payload->id);

        if (order.tag == Order::Tag::STOP_LOSS)
        {
            position.stop_loss_order = nullptr;
        }
        else if (order.tag == Order::Tag::TAKE_PROFIT)
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

            if (order.payload->get_status() == NewOrderPayload::Status::PENDING)
            {
                remove_from_level(order);
                delete position.entry_order;
            }

            this->tracker.erase(order.payload->id);
        }
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Position for order id " + std::to_string(order.payload->id) + " doesn't exist");
    }
}

float OrderBook::best_price(NewOrderPayload::Side &&side)
{
    if (side == NewOrderPayload::Side::ASK)
    {
        float best_price = this->asks.begin()->first;
        if (best_price == NULL)
        {
            return this->price;
        }

        return best_price;
    }

    if (side == NewOrderPayload::Side::BID)
    {
        float best_price = this->bids.rbegin()->first;
        if (best_price == NULL)
        {
            return this->price;
        }

        return best_price;
    }
}

void OrderBook::push_order(Order &order)
{
    auto &book = get_book(order);

    if (order.tag == Order::Tag::ENTRY)
    {
        book[order.payload->entry_price].push_back(&order);
    }
    else
    {
        const float *price =
            (order.tag == Order::Tag::TAKE_PROFIT)
                ? order.payload->take_profit_price
                : order.payload->stop_loss_price;
        book[*price].push_back(&order);
    }
}

void OrderBook::remove_from_level(Order &order)
{
    auto &book = get_book(order);

    if (order.tag == Order::Tag::ENTRY)
    {
        book[order.payload->entry_price].remove(&order);
    }
    else
    {
        book[*((order.tag == Order::Tag::TAKE_PROFIT)
                   ? order.payload->take_profit_price
                   : order.payload->stop_loss_price)]
            .remove(&order);
    }
}

std::pair<int, int> OrderBook::size()
{
    return std::pair<int, int>{this->bids.size(), this->asks.size()};
}