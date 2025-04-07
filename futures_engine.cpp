#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>
#include "futures_engine.h"
#include "order.h"
#include "orderbook.h"
#include "utilities.h"
#include "queue.h"

MatchResult::MatchResult(
    MatchResultType result_type_,
    int price)
    : m_result_type(result_type_),
      m_result_set(true),
      m_price(price) {};

MatchResult::MatchResult() : m_result_set(false), m_price(-1) {};

void MatchResult::set_result_type(MatchResultType result_type)
{
    {
        if (this->m_result_set)
        {
            throw std::string("Cannot set result on MatchResult once it's already set");
        }

        this->m_result_type = result_type;
        this->m_result_set = true;
    }
}

const MatchResultType &MatchResult::get_result_type() const
{
    if (!this->m_result_set)
    {
        throw std::string("Result not set");
    }

    return this->m_result_type;
}

void FuturesEngine::start(Queue &queue)
{
    this->orderbooks.emplace("APPL", OrderBook("APPL", 50.0));
    const time_t start_time = time(NULL);
    int count = 0;

    while (true)
    {
        std::unique_ptr<QueuePayload> qpayload = queue.get();

        if (qpayload->m_category == QueuePayload::Category::NEW)
        {
            std::shared_ptr<NewOrderPayload> payload = std::dynamic_pointer_cast<NewOrderPayload>(qpayload->m_payload);

            if (payload->m_order_type == NewOrderPayload::OrderType::LIMIT)
            {
                place_limit_order(payload);
            }
            else
            {
                if (payload->m_exec_type == NewOrderPayload::ExecutionType::GTC)
                {

                    place_gtc_market_order(payload);
                }
                else
                {
                    place_fok_market_order(payload);
                }
            }
        }
        else if (qpayload->m_category == QueuePayload::Category::MODIFY)
        {
            modify_order(std::dynamic_pointer_cast<ModifyOrderPayload>(qpayload->m_payload));
        }

        count += 1;

        if (count % 1000000 == 0)
        {
            std::cout << "Processed " << std::to_string(count) << " orders in " << std::to_string(time(NULL) - start_time) << " seconds" << std::endl;
        }
    }
}

void FuturesEngine::place_limit_order(std::shared_ptr<NewOrderPayload> &payload)
{
    std::cout << "Placing limit order" << std::to_string(payload->m_id) << std::endl;
    OrderBook &orderbook = this->orderbooks.at(payload->m_instrument);
    orderbook.push_order(*orderbook.declare(payload).m_entry_order);
}

void FuturesEngine::place_gtc_market_order(std::shared_ptr<NewOrderPayload> &payload_p)
{
    // NewOrderPayload &payload = *payload_p;
    OrderBook &orderbook = this->orderbooks.at(payload_p->m_instrument);
    Position &position = orderbook.declare(payload_p);

    Order &order = *position.m_entry_order;
    const MatchResult result = match_gtc(order, orderbook);
    const MatchResultType &result_type = result.get_result_type();

    if (result.get_result_type() == SUCCESS)
    {
        order.m_payload->m_standing_quantity = order.m_payload->m_quantity;
        order.m_payload->set_status(NewOrderPayload::Status::FILLED);
        order.m_payload->set_filled_price(result.m_price);
        place_tp_sl(order, orderbook);
        orderbook.set_price(result.m_price);
    }
    else
    {
        if (result_type == PARTIAL)
        {
            order.m_payload->set_status(NewOrderPayload::Status::PARTIALLY_FILLED);
        }
        orderbook.push_order(order);
    }
}

void FuturesEngine::place_fok_market_order(std::shared_ptr<NewOrderPayload> &payload_p)
{
    OrderBook &orderbook = this->orderbooks.at(payload_p->m_instrument);
    Position &position = orderbook.declare(payload_p);
    const MatchResult result = match_fok(position.m_entry_order, orderbook);

    if (result.get_result_type() == SUCCESS)
    {
        payload_p->m_standing_quantity = payload_p->m_quantity;
        payload_p->set_status(NewOrderPayload::Status::FILLED);
        payload_p->set_filled_price(result.m_price);
        place_tp_sl(*position.m_entry_order, orderbook);
        orderbook.set_price(result.m_price);
    }
    else
    {
        orderbook.rtrack(*position.m_entry_order);
    }
}

void FuturesEngine::cancel_order(std::shared_ptr<BasePayload> &payload)
{

    OrderBook &orderbook = this->orderbooks.at(payload->m_instrument);
    Position &pos = orderbook.get_position(payload->m_id);

    if (pos.m_entry_order->m_payload->get_status() != NewOrderPayload::Status::PENDING)
    {
        throw std::string("Cancel can only be done on PENDING orders");
    }

    orderbook.rtrack(*pos.m_entry_order);
}

void FuturesEngine::modify_order(std::shared_ptr<ModifyOrderPayload> &&payloadp)
{
    OrderBook &orderbook = this->orderbooks.at(payloadp->m_instrument);
    Position &position = orderbook.get_position(payloadp->m_id);

    if (payloadp->m_stop_loss_price == 0.0f)
    {
        if (position.m_stop_loss_order)
        {
            orderbook.remove_from_level(*position.m_stop_loss_order);
        }
    }
    else
    {
        if (position.m_entry_order->m_payload->m_stop_loss_order != nullptr)
        {
            if (payloadp->m_stop_loss_price != position.m_entry_order->m_payload->m_stop_loss_order->m_price)
            {
                orderbook.remove_from_level(*position.m_stop_loss_order);
                delete position.m_stop_loss_order;
            }

            position.m_entry_order->m_payload->m_stop_loss_order->m_price = payloadp->m_stop_loss_price;
        }
        else
        {
            position.m_entry_order->m_payload->m_stop_loss_order = std::make_unique<StopLossOrder>(payloadp->m_stop_loss_price);
        }

        position.m_stop_loss_order = new Order(position.m_entry_order->m_payload, Order::Tag::STOP_LOSS);
        orderbook.push_order(*position.m_stop_loss_order);
    }

    if (payloadp->m_take_profit_price == 0.0f)
    {
        if (position.m_take_profit_order)
        {
            orderbook.remove_from_level(*position.m_take_profit_order);
        }
    }
    else
    {
        if (position.m_entry_order->m_payload->m_take_profit_price != 0.0f)
        {
            if (payloadp->m_take_profit_price != position.m_entry_order->m_payload->m_take_profit_price)
            {
                orderbook.remove_from_level(*position.m_take_profit_order);
                delete position.m_take_profit_order;
            }

            position.m_entry_order->m_payload->m_take_profit_price = payloadp->m_take_profit_price;
            position.m_take_profit_order = new Order(position.m_entry_order->m_payload, Order::Tag::TAKE_PROFIT);
            orderbook.push_order(*position.m_take_profit_order);
        }
    }

    if (
        payloadp->m_entry_price == 0.0f ||
        position.m_entry_order->m_payload->m_order_type != NewOrderPayload::OrderType::LIMIT)
    {
        return;
    }

    if (payloadp->m_entry_price != position.m_entry_order->m_payload->m_entry_price)
    {
        orderbook.remove_from_level(*position.m_entry_order);
        delete position.m_entry_order;
        position.m_entry_order->m_payload->m_entry_price = payloadp->m_entry_price;
        position.m_entry_order = new Order(position.m_entry_order->m_payload, Order::Tag::ENTRY);
        orderbook.push_order(*position.m_entry_order);
    }
}

void FuturesEngine::close_order(std::shared_ptr<BasePayload> &payload)
{
    OrderBook &orderbook = this->orderbooks.at(payload->m_instrument);
    Position &pos = orderbook.get_position(payload->m_id);
    orderbook.rtrack(*pos.m_entry_order);
}

const MatchResult FuturesEngine::match_gtc(Order &order, OrderBook &orderbook)
{
    const float price = order.m_payload->m_entry_price;
    const int og_standing_quantity = order.m_payload->m_standing_quantity;
    // std::list<std::tuple<Order *&, int>> touched;
    // std::list<std::tuple<Order *&, int>> filled;
    std::list<std::pair<Order *&, int>> touched;
    std::list<std::pair<Order *&, int>> filled;

    for (Order *&ex_order_p : orderbook.get_book(order)[price])
    {
        // Order &ex_order = *ex_order_p;
        const int ex_order_pre_standing_quantity = ex_order_p->m_payload->m_standing_quantity;
        const int reduce_amount = std::min(order.m_payload->m_standing_quantity, ex_order_pre_standing_quantity);
        ex_order_p->m_payload->m_standing_quantity -= reduce_amount;
        order.m_payload->m_standing_quantity -= reduce_amount;

        if (ex_order_p->m_payload->m_standing_quantity == 0)
        {
            // filled.push_back(std::tuple<Order *&, int>{ex_order_p, ex_order_pre_standing_quantity});
            filled.push_back(std::pair<Order *&, int>{ex_order_p, ex_order_pre_standing_quantity});
        }
        else
        {
            // touched.push_back(std::tuple<Order *&, int>{ex_order_p, ex_order_pre_standing_quantity});
            touched.push_back(std::pair<Order *&, int>{ex_order_p, ex_order_pre_standing_quantity});
        }

        if (order.m_payload->m_standing_quantity == 0)
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

// Only to be used on Tag::ENTRY orders
const MatchResult FuturesEngine::match_fok(Order *&order, OrderBook &orderbook)
{
    const int og_standing_quantity = order->m_payload->m_standing_quantity;
    auto &book = orderbook.get_book(*order);
    // std::list<std::tuple<Order *&, int, int>> touched_order_collection;
    std::vector<std::tuple<Order *&, int, int>> touched_order_collection;

    for (Order *&ex_orderp : book[order->m_payload->m_entry_price])
    {
        const int ex_order_pre_standing_quantity = ex_orderp->m_payload->m_standing_quantity;
        const int reduce_amount = std::min(order->m_payload->m_standing_quantity, ex_order_pre_standing_quantity);
        order->m_payload->m_standing_quantity -= reduce_amount;
        touched_order_collection.push_back(std::tuple<Order *&, int, int>{ex_orderp, ex_order_pre_standing_quantity, reduce_amount});

        if (order->m_payload->m_standing_quantity == 0)
        {
            break;
        }
    }

    if (order->m_payload->m_standing_quantity == 0)
    {
        // std::list<std::tuple<Order *&, int>> touched;
        // std::list<std::tuple<Order *&, int>> filled;
        std::list<std::pair<Order *&, int>> touched;
        std::list<std::pair<Order *&, int>> filled;

        for (auto &tup : touched_order_collection)
        {
            Order *&ex_order = std::get<0>(tup);
            ex_order->m_payload->m_standing_quantity -= std::get<2>(tup);

            if (ex_order->m_payload->m_standing_quantity == 0)
            {
                // filled.push_back(std::tuple<Order *&, int>{ex_order, std::get<1>(tup)});
                filled.push_back(std::pair<Order *&, int>{ex_order, std::get<1>(tup)});
            }
            else
            {
                // touched.push_back(std::tuple<Order *&, int>{ex_order, std::get<1>(tup)});
                touched.push_back(std::pair<Order *&, int>{ex_order, std::get<1>(tup)});
            }
        }

        if (touched.size() > 0)
        {
            handle_touched_orders(touched, orderbook, order->m_payload->m_entry_price);
        }

        if (filled.size() > 0)
        {
            handle_filled_orders(filled, orderbook, order->m_payload->m_entry_price);
        }
    }

    return gen_match_result(og_standing_quantity, *order, order->m_payload->m_entry_price);
}

const MatchResult FuturesEngine::gen_match_result(const float og_standing_quantity, Order &order, const float price)
{
    MatchResult result;

    if (order.m_payload->m_standing_quantity == 0)
    {
        result.set_result_type(SUCCESS);
        result.m_price = price;
    }
    else if (order.m_payload->m_standing_quantity == og_standing_quantity)
    {
        result.set_result_type(FAILURE);
    }
    else
    {
        result.set_result_type(PARTIAL);
    }

    return result;
}

// void FuturesEngine::handle_filled_orders(std::list<std::tuple<Order *&, int>> &orders, OrderBook &orderbook, const float price)
void FuturesEngine::handle_filled_orders(std::list<std::pair<Order *&, int>> &orders, OrderBook &orderbook, const float price)
{
    for (auto &pair : orders)
    {
        // Order &order = *std::get<0>(tup);
        Order *&order = pair.first;

        // if (order.m_tag == Order::Tag::ENTRY)
        // {
        //     orderbook.remove_from_level(order);
        //     order.m_payload->m_standing_quantity = order.m_payload->m_quantity;
        //     order.m_payload->set_status(NewOrderPayload::Status::FILLED);
        //     order.m_payload->set_filled_price(price);
        //     place_tp_sl(order, orderbook);
        // }
        // else
        // {
        //     calc_upl(order, std::get<1>(tup), price);
        //     orderbook.rtrack(*orderbook.get_position(order.m_payload->m_id).m_entry_order);
        // }
        if (order->m_tag == Order::Tag::ENTRY)
        {
            orderbook.remove_from_level(*order);
            order->m_payload->m_standing_quantity = order->m_payload->m_quantity;
            order->m_payload->set_status(NewOrderPayload::Status::FILLED);
            order->m_payload->set_filled_price(price);
            place_tp_sl(*order, orderbook);
        }
        else
        {
            calc_upl(*order, pair.second, price);
            orderbook.rtrack(*orderbook.get_position(order->m_payload->m_id).m_entry_order);
        }
    }
}

// void FuturesEngine::handle_touched_orders(std::list<std::tuple<Order *&, int>> &orders, OrderBook &orderbook, const float price)
void FuturesEngine::handle_touched_orders(std::list<std::pair<Order *&, int>> &orders, OrderBook &orderbook, const float price)
{
    // for (auto &tup : orders)
    for (auto &pair : orders)
    {
        // Order &order = *std::get<0>(tup);
        Order *&order = pair.first;

        // if (order.m_tag == Order::Tag::ENTRY)
        // {
        //     order.m_payload->set_status(NewOrderPayload::Status::PARTIALLY_FILLED);
        // }
        // else
        // {
        //     calc_upl(order, std::get<1>(tup), price);

        //     if (order.m_payload->get_status() == NewOrderPayload::Status::CLOSED)
        //     {
        //         orderbook.rtrack(*orderbook.get_position(order.m_payload->m_id).m_entry_order);
        //     }
        //     else
        //     {
        //         order.m_payload->set_status(NewOrderPayload::Status::PARTIALLY_CLOSED);
        //     }
        // }
        if (order->m_tag == Order::Tag::ENTRY)
        {
            order->m_payload->set_status(NewOrderPayload::Status::PARTIALLY_FILLED);
        }
        else
        {
            calc_upl(*order, pair.second, price);

            if (order->m_payload->get_status() == NewOrderPayload::Status::CLOSED)
            {
                orderbook.rtrack(*orderbook.get_position(order->m_payload->m_id).m_entry_order);
            }
            else
            {
                order->m_payload->set_status(NewOrderPayload::Status::PARTIALLY_CLOSED);
            }
        }
    }
}

void FuturesEngine::place_tp_sl(Order &order, OrderBook &orderbook)
{
    Position &pos = orderbook.get_position(order.m_payload->m_id);

    if (order.m_payload->m_take_profit_price != 0.0f)
    {
        Order tp_order(order.m_payload, Order::Tag::TAKE_PROFIT);
        orderbook.track(tp_order);
        orderbook.push_order(tp_order);
    }

    if (order.m_payload->m_stop_loss_order->m_distance != 0.0f ||
        order.m_payload->m_stop_loss_order->m_price != 0.0f)
    {
        Order sl_order(order.m_payload, Order::Tag::STOP_LOSS);
        orderbook.track(sl_order);
        orderbook.push_order(sl_order);
    }
}
