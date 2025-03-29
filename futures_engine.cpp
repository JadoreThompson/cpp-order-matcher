#include <iostream>
#include <thread>
#include "futures_engine.h"
#include "order.h"
#include "orderbook.h"
#include "utilities.h"
#include "queue.h"
#include "queue.cpp"

MatchResult::MatchResult(
    MatchResultType result_type_,
    int price)
    : result_type(result_type_),
      result_set(true),
      price(price) {};

MatchResult::MatchResult() : result_set(false), price(-1) {};

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
        std::shared_ptr<OrderPayload> payload_p = queue.get();
        auto &payload = *payload_p;

        try
        {
            if (payload.order_type == LIMIT)
            {
                place_limit_order(payload_p);
            }
            else
            {
                place_market_order(payload_p);
            }
        }
        catch (const std::string &e)
        {
            std::cout << "* " << e << " *" << std::endl;
            throw e;
        }
    }
}

void FuturesEngine::handler(OrderPayload &payload)
{
}

void FuturesEngine::place_limit_order(std::shared_ptr<OrderPayload> &payload)
{
    try
    {
        std::cout << "Placing limit order" << std::to_string(payload->id) << std::endl;
        OrderBook &orderbook = this->orderbooks.at(payload->instrument);
        orderbook.push_order(*orderbook.declare(payload).entry_order);
    }
    catch (const std::out_of_range &e)
    {
        throw std::string("Orderbook for " + payload->instrument + " doesn't exist");
    }
}

void FuturesEngine::place_market_order(std::shared_ptr<OrderPayload> &payload_p)
{
    auto &payload = *payload_p;
    OrderBook &orderbook = this->orderbooks.at(payload.instrument);
    Position &position = orderbook.declare(payload_p);

    Order &order = *position.entry_order;
    MatchResult result = match(order, orderbook);
    MatchResultType &result_type = result.get_result_type();

    if (result_type == SUCCESS)
    {
        order.payload->standing_quantity = order.payload->quantity;
        order.payload->set_status(FILLED);
        order.payload->set_filled_price(result.price);
        place_tp_sl(order, orderbook);
    }
    else
    {
        if (result_type == PARTIAL)
        {
            order.payload->set_status(PARTIALLY_FILLED);
        }
        orderbook.push_order(order);
    }
}

MatchResult FuturesEngine::match(Order &order, OrderBook &orderbook)
{
    float price;

    if (order.tag == ENTRY)
    {
        price = order.payload->entry_price;
    }
    else if (order.tag == STOP_LOSS)
    {
        price = *(order.payload->stop_loss_price);
    }
    else
    {
        price = *(order.payload->take_profit_price);
    }

    const int og_standing_quantity = order.payload->standing_quantity;
    std::list<std::tuple<Order *, int>> touched;
    std::list<std::tuple<Order *, int>> filled;

    for (auto &ex_order_p : orderbook.get_book(order)[price])
    {
        auto &ex_order = *ex_order_p;
        const int ex_order_pre_standing_quantity = ex_order.payload->standing_quantity;
        const int reduce_amount = std::min(order.payload->standing_quantity, ex_order_pre_standing_quantity);
        ex_order.payload->standing_quantity -= reduce_amount;
        order.payload->standing_quantity -= reduce_amount;

        if (ex_order.payload->standing_quantity == 0)
        {
            filled.push_back(std::tuple<Order *, int>{ex_order_p, ex_order_pre_standing_quantity});
        }
        else
        {
            touched.push_back(std::tuple<Order *, int>{ex_order_p, ex_order_pre_standing_quantity});
        }

        if (order.payload->standing_quantity == 0)
        {
            break;
        }
    }

    if (touched.size() > 0)
    {
        handle_touched_orders(touched, orderbook, price);
    }
    if (filled.size() > 0)
    {
        handle_filled_orders(filled, orderbook, price);
    }

    return gen_match_result(og_standing_quantity, order, price);
}

MatchResult FuturesEngine::gen_match_result(const float og_standing_quantity, Order &order, const float price)
{
    MatchResult result;

    if (order.payload->standing_quantity == 0)
    {
        result.set_result_type(SUCCESS);
        result.price = price;
    }
    else if (order.payload->standing_quantity == og_standing_quantity)
    {
        result.set_result_type(FAILURE);
    }
    else
    {
        result.set_result_type(PARTIAL);
    }

    return result;
}

void FuturesEngine::handle_filled_orders(std::list<std::tuple<Order *, int>> &orders, OrderBook &orderbook, const float price)
{
    for (auto &tup : orders)
    {
        auto &order = *std::get<0>(tup);

        if (order.tag == ENTRY)
        {
            orderbook.remove_from_level(order);
            order.payload->standing_quantity = order.payload->quantity;
            order.payload->set_status(FILLED);
            order.payload->set_filled_price(price);
            place_tp_sl(order, orderbook);
        }
        else
        {
            calc_upl(order, std::get<1>(tup), price);
            orderbook.rtrack(*orderbook.get_position(order.payload->id).entry_order);
        }
    }
}

void FuturesEngine::handle_touched_orders(std::list<std::tuple<Order *, int>> &orders, OrderBook &orderbook, const float price)
{
    for (auto &tup : orders)
    {

        auto &order = *std::get<0>(tup);
        if (order.tag == ENTRY)
        {
            order.payload->set_status(PARTIALLY_FILLED);
        }
        else
        {
            calc_upl(order, std::get<1>(tup), price);

            if (order.payload->get_status() == CLOSED)
            {
                // std::cout << "[touched] Closing order" << std::endl;
                orderbook.rtrack(*orderbook.get_position(order.payload->id).entry_order);
            }
            else
            {
                order.payload->set_status(PARTIALLY_CLOSED);
            }
        }
    }
}

void FuturesEngine::place_tp_sl(Order &order, OrderBook &orderbook) const
{
    if (order.payload->take_profit_price)
    {
        Order tp_order(order.payload, TAKE_PROFIT);
        orderbook.track(tp_order);
        orderbook.push_order(tp_order);
    }

    if (order.payload->stop_loss_price)
    {
        Order sl_order(order.payload, STOP_LOSS);
        orderbook.track(sl_order);
        orderbook.push_order(sl_order);
    }
}
