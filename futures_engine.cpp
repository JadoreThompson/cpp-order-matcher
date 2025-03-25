#include <iostream>
#include <thread>
#include "futures_engine.h"
#include "order.h"
#include "orderbook.h"
#include "queue.h"
#include "queue.cpp"

MatchResult::MatchResult(
    MatchResultType result_type_,
    int price)
    : result_type(result_type_),
      result_set(true),
      price(price) {};

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

    while (true)
    {
        OrderPayload &payload = queue.get();
        std::cout
            << "Handling payload with id " << std::to_string(payload.id)
            << std::endl;

        try
        {
            if (payload.order_type == LIMIT)
            {
                place_limit_order(payload);
            }
            else
            {
                place_market_order(payload);
            }
        }
        catch (const std::string &e)
        {
            std::cout << "* " << e << " *" << std::endl;
        }
        catch (const std::exception &e)
        {
            // std::cout << "* " << e.what() << " *" << std::endl;
        }
    }
}

void FuturesEngine::handler(OrderPayload &payload)
{
}

void FuturesEngine::place_limit_order(OrderPayload &payload)
{
    try
    {
        std::cout << "Placing limit order" << std::to_string(payload.id) << std::endl;
        OrderBook &orderbook = this->orderbooks.at(payload.instrument);
        Order order(payload, ENTRY);
        orderbook.push_order(order);
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
        std::cout << "Placing market order " << std::to_string(payload.id) << std::endl;
        OrderBook &orderbook = this->orderbooks.at(payload.instrument);
        std::cout << "Orderbook found" << std::endl;
        Order order(payload, ENTRY);
        std::cout << "Order created" << std::endl;
        MatchResult result = match(order, orderbook);
        std::cout << "Matched" << std::endl;
        MatchResultType &result_type = result.get_result_type();
        std::cout << "Got result type" << std::endl;

        if (result_type == SUCCESS)
        {
            std::cout
                << "match success at " << std::to_string(result.price)
                << " Status " << std::to_string(payload.get_status())
                << " Id " << std::to_string(payload.id)
                << std::endl;
            order.payload.standing_quantity = order.payload.quantity;
            order.payload.set_filled_price(result.price);
            // order.payload.set_status(FILLED);
            // orderbook.track(order);
            // place_tp_sl(order, orderbook);
        }
        else
        {
            std::cout
                << "Matching failed ID " << std::to_string(payload.id)
                << std::endl;
            if (result_type == PARTIAL)
            {
                order.payload.set_status(PARTIALLY_FILLED);
            }
            orderbook.push_order(order);
        }
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Out of range for payload instrument => " + payload.instrument);
    }
}

MatchResult FuturesEngine::match(Order &order, OrderBook &orderbook)
{
    auto &book = orderbook.get_book(order);
    std::cout << "Matching order" << std::endl;
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

    std::cout << "Price: " << std::to_string(price) << std::endl;

    const int og_standing_quantity = order.payload.standing_quantity;
    std::list<Order *> touched;
    std::list<Order *> filled;

    for (auto &existing_order : book[price])
    {
        const int reduce_amount = std::min(order.payload.standing_quantity, existing_order.payload.standing_quantity);
        existing_order.payload.standing_quantity -= reduce_amount;
        order.payload.standing_quantity -= reduce_amount;

        if (existing_order.payload.standing_quantity == 0)
        {
            filled.push_back(&existing_order);
        }
        else
        {
            touched.push_back(&existing_order);
        }

        if (order.payload.standing_quantity == 0)
        {
            break;
        }
    }

    std::cout << "Handling filled orders" << std::endl;
    handle_filled_orders(filled, orderbook, price);
    std::cout << "Handling touched orders" << std::endl;
    handle_touched_orders(touched, orderbook);

    std::cout << "Creating match result" << std::endl;
    MatchResult result;
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

    std::cout << "Returning result" << std::endl;
    return result;
}

void FuturesEngine::handle_filled_orders(std::list<Order *> &orders, OrderBook &orderbook, const float price)
{
    for (auto &op : orders)
    {
        auto order = *op;
        std::cout << "Handling filled order " << std::to_string(order.payload.id) << std::endl;
        if (order.tag == ENTRY)
        {
            orderbook.remove_from_level(order);
            std::cout
                << "Handling filled order: " << std::to_string(order.payload.id)
                << " Status: " << std::to_string(order.payload.get_status())
                << " Tag: " << std::to_string(order.tag)
                << " Id: " << std::to_string(order.payload.id)
                << std::endl;
            std::cout << "Removed from level" << std::endl;
            std::cout
                << "Current standing quantity: " << std::to_string(order.tag)
                << " Quantity: " << std::to_string(order.payload.quantity)
                << std::endl;
            order.payload.standing_quantity = order.payload.quantity;
            std::cout << "Set standing quantity" << std::endl;
            order.payload.set_filled_price(price);
            std::cout << "Set filled price" << std::endl;
            order.payload.set_status(FILLED);
            std::cout << "Set status" << std::endl;
            place_tp_sl(order, orderbook);
            std::cout << "Placed TP/SL" << std::endl;
        }
        else
        {
            std::cout << "Closed order at " << std::to_string(price) << std::endl;
            order.payload.set_status(CLOSED);
            orderbook.rtrack(orderbook.get_position(order.payload.id).entry_order);
        }
    }
}

void FuturesEngine::handle_touched_orders(std::list<Order *> &orders, OrderBook &orderbook)
{
    for (auto &op : orders)
    {
        auto &order = *op;
        // if (op.tag == ENTRY)
        if (order.tag == ENTRY)
        {
            // op.payload.set_status(PARTIALLY_FILLED);
            order.payload.set_status(PARTIALLY_FILLED);
        }
        else
        {
            // op.payload.set_status(PARTIALLY_CLOSED);
            order.payload.set_status(PARTIALLY_CLOSED);
        }
    }
}

void FuturesEngine::place_tp_sl(Order &order, OrderBook &orderbook) const
{
    bool placed = false;
    std::cout << "Placing TP/SL" << std::endl;
    if (order.payload.take_profit_price)
    {
        Order tp_order(order.payload, TAKE_PROFIT);
        orderbook.push_order(tp_order);
        placed = true;
    }

    if (order.payload.stop_loss_price)
    {
        Order sl_order(order.payload, STOP_LOSS);
        orderbook.push_order(sl_order);
        placed = true;
    }

    if (!placed)
    {
        std::cout << "No TP/SL placed" << std::endl;
    }
}
