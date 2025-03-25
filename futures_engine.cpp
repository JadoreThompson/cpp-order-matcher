#include <iostream>
#include <thread>
#include "futures_engine.h"
#include "order.h"
#include "orderbook.h"
#include "queue.h"
#include "queue.cpp"

MatchResult::MatchResult(MatchResultType result_type_,
                         int price) : result_type(result_type_), result_set(true), price(price) {};

MatchResult::MatchResult() : result_set(false), price(-1) {}

void MatchResult::set_result_type(MatchResultType result_type)
{
    {
        if (this->result_set)
        {
            throw std::string("Cannot set result on MatchResult once it's already set");
        }

        this->result_type = result_type;
        this->result_set = true;
    }
}

MatchResultType &MatchResult::get_result_type()
{
    if (!this->result_set)
    {
        throw std::string("Result not set");
    }

    return this->result_type;
}

void FuturesEngine::start(Queue<OrderPayload> &queue)
{
    this->orderbooks.emplace("APPL", OrderBook("APPL"));
    int counter = 0;

    while (true)
    {
        try
        {
            OrderPayload &payload = queue.get();

            if (payload.order_type == LIMIT)
            {
                // place_limit_order(payload);
            }
            else
            {
                place_market_order(payload);
            }

        }
        catch (const std::string &e)
        {
            std::cout << "*" << e << std::endl;
        }
        // counter += 1;
        // if (counter % 500)
        // {
        //     const auto &size = this->orderbooks.at("APPL").size();
        //     std::cout
        //         << "Asks " << std::to_string(size.first)
        //         << "Bids " << std::to_string(size.second)
        //         << std::endl;

        //     std::this_thread::sleep_for(std::chrono::seconds(5));
        // }
    }
}

void FuturesEngine::handler(OrderPayload &payload)
{
}

void FuturesEngine::place_limit_order(OrderPayload &payload)
{
    try
    {
        OrderBook &orderbook = this->orderbooks.at(payload.instrument);
        Order order(payload, ENTRY);
        orderbook.push_order(order);
        orderbook.track(order);
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Orderbook for " + payload.instrument + " doesn't exist");
    }
}

void FuturesEngine::place_market_order(OrderPayload &payload)
{
    try
    {
        OrderBook &orderbook = this->orderbooks.at(payload.instrument);
        Order order(payload, ENTRY);
        MatchResult result = match(order, orderbook);
        MatchResultType &result_type = result.get_result_type();

        if (result_type == SUCCESS)
        {
            order.payload.set_status(FILLED);
            order.payload.standing_quantity = order.payload.quantity;
            order.payload.set_filled_price(result.price);
        }
        else
        {
            if (result_type == PARTIAL)
            {
                order.payload.set_status(PARTIALLY_FILLED);
            }

            orderbook.push_order(order);
        }

        orderbook.track(order);
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Out of range for payload instrument => " + std::to_string(payload.instrument));
    }
}

MatchResult FuturesEngine::match(Order &order, OrderBook &orderbook)
{
    std::map<float, std::list<Order>> &book = orderbook.get_opposite_book(order);
    float price;

    if (order.tag == ENTRY)
    {
        price = order.payload.entry_price;
    }
    else if (order.tag == STOP_LOSS)
    {
        price = *order.payload.stop_loss_price;
    }
    else
    {
        price = *order.payload.take_profit_price;
    }

    const int og_standing_quantity = order.payload.standing_quantity;
    std::list<Order> touched;
    std::list<Order> filled;
    MatchResult result;

    for (auto &existing_order : book[price])
    {
        const int reduce_amount = std::min(order.payload.standing_quantity, existing_order.payload.standing_quantity);
        existing_order.payload.standing_quantity -= reduce_amount;
        order.payload.standing_quantity -= reduce_amount;

        if (existing_order.payload.standing_quantity == 0)
        {
            filled.push_back(existing_order);
        }
        else
        {
            touched.push_back(existing_order);
        }

        if (order.payload.standing_quantity == 0)
        {
            break;
        }
    }

    handle_filled_orders(filled, orderbook);
    handle_touched_orders(touched, orderbook);

    if (order.payload.standing_quantity == 0)
    {
        result.set_result_type(SUCCESS);
        result.price = price;
    }
    else if (order.payload.standing_quantity == og_standing_quantity)
    {
        result.set_result_type(FAILURE);
    }
    else
    {
        result.set_result_type(PARTIAL);
    }

    return result;
}

void FuturesEngine::handle_filled_orders(std::list<Order> &orders, OrderBook &orderbook)
{
    for (auto &order : orders)
    {
        if (order.tag == ENTRY)
        {
            order.payload.set_status(FILLED);
            order.payload.standing_quantity = order.payload.quantity;
            place_tp_sl(order, orderbook);
        }
        else
        {
            orderbook.rtrack(orderbook.get_position(order.payload.id).entry_order);
            order.payload.set_status(CLOSED);
        }

        orderbook.remove_from_level(order);
    }
}

void FuturesEngine::handle_touched_orders(std::list<Order> &orders, OrderBook &orderbook)
{
    for (auto &order : orders)
    {
        if (order.tag == ENTRY)
        {
            order.payload.set_status(PARTIALLY_FILLED);
        }
        else
        {
            order.payload.set_status(PARTIALLY_CLOSED);
        }
    }
}

void FuturesEngine::place_tp_sl(Order &order, OrderBook &orderbook) const
{
    if (order.payload.take_profit_price)
    {
        Order tp_order(order.payload, TAKE_PROFIT);
        orderbook.push_order(tp_order);
    }

    if (order.payload.stop_loss_price)
    {
        Order sl_order(order.payload, STOP_LOSS);
        orderbook.push_order(sl_order);
    }
}
