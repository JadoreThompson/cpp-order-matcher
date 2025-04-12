#include <chrono>
#include <iostream>
#include <memory>
#include <numeric>
#include <vector>
#include "futures_engine.h"
#include "order.h"
#include "orderbook.h"
#include "queue.h"
#include "utilities.h"

MatchResult::MatchResult(
    const MatchResultType result_type_,
    int price)
    : m_result_type(result_type_),
      m_result_set(true),
      m_price(price) {};

MatchResult::MatchResult() : m_result_set(false), m_price(-1) {};

// void MatchResult::set_result_type(const MatchResultType result_type)
// {
//     {
//         if (this->m_result_set)
//         {
//             throw std::string("Cannot set result on MatchResult once it's already set");
//         }

//         this->m_result_type = result_type;
//         this->m_result_set = true;
//     }
// }

// const MatchResult::MatchResultType &MatchResult::get_result_type() const
// {
//     if (!this->m_result_set)
//     {
//         throw std::string("Result not set");
//     }

//     return this->m_result_type;
// }

void FuturesEngine::start(Queue &queue)
{
    this->orderbooks.emplace("APPL", OrderBook("APPL", 50.0f));
    const std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();

    for (int _ = 0; _ < LOOPS; _++)
    // while (true)
    {
        QueuePayload message = queue.get();

        if (message.m_category == QueuePayload::Category::NEW)
        {
            std::shared_ptr<OrderPayload> payload(static_cast<OrderPayload *>(message.m_payload.release()));

            if (payload->m_order_type == OrderType::LIMIT)
            {
                place_limit_order(std::move(payload));
            }
            else if (payload->m_exec_type == ExecutionType::GTC)
            {
                // place_gtc_market_order(order);
                place_gtc_market_order(std::move(payload));
            }
            else
            {
                // place_fok_market_order(order);
                place_fok_market_order(std::move(payload));
            }
        }
    }

    std::cout << "Time Taken (seconds): " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time).count() << std::endl;
}

void FuturesEngine::place_fok_market_order(std::shared_ptr<OrderPayload> &&payloadp)
// void FuturesEngine::place_fok_market_order(OrderPayload &payload_p)
{
    OrderBook &orderbook = this->orderbooks.at(payloadp->m_instrument);
    Position &position = orderbook.declare(payloadp);
    const MatchResult result = match_fok(*position.m_entry_order, orderbook);

    if (result.m_result_type == MatchResult::MatchResultType::SUCCESS)
    {
        payloadp->m_standing_quantity = payloadp->m_quantity;
        // payload_p->set_status(Status::FILLED);
        // payload_p->set_filled_price(result.m_price);
        place_tp_sl(*position.m_entry_order, orderbook);
        orderbook.set_price(result.m_price);
    }
    else
    {
        orderbook.rtrack(position.m_entry_order);
    }
}

void FuturesEngine::place_gtc_market_order(std::shared_ptr<OrderPayload> &&payloadp)
// void FuturesEngine::place_gtc_market_order(OrderPayload &payload_p)
{
    OrderBook &orderbook = this->orderbooks.at(payloadp->m_instrument);
    Position &position = orderbook.declare(payloadp);
    Order &order = *position.m_entry_order;
    const MatchResult result = match_gtc(*position.m_entry_order, orderbook);

    if (result.m_result_type == MatchResult::MatchResultType::SUCCESS)
    {
        order.m_payload->m_standing_quantity = order.m_payload->m_quantity;
        // order.m_payload->set_status(Status::FILLED);
        // order.m_payload->set_filled_price(result.m_price);
        order.m_payload->m_status = Status::FILLED;
        order.m_payload->m_filled_price = result.m_price;
        orderbook.set_price(result.m_price);
        place_tp_sl(order, orderbook);
    }
    else
    {
        if (result.m_result_type == MatchResult::MatchResultType::PARTIAL)
        {
            // order.m_payload->set_status(Status::PARTIALLY_FILLED);
            order.m_payload->m_status = Status::PARTIALLY_FILLED;
        }
        orderbook.push_order(position.m_entry_order);
    }
}

void FuturesEngine::place_limit_order(std::shared_ptr<OrderPayload> &&payloadp)
// void FuturesEngine::place_limit_order(OrderPayload &payload)
{
    // std::cout << "Placing limit order" << std::to_string(payloadp->m_id) << std::endl;
    OrderBook &orderbook = this->orderbooks.at(payloadp->m_instrument);
    orderbook.push_order(orderbook.declare(payloadp).m_entry_order);
}

void FuturesEngine::cancel_order(std::shared_ptr<BasePayload> &payload)
{

    OrderBook &orderbook = this->orderbooks.at(payload->m_instrument);
    Position &pos = orderbook.get_position(payload->m_id);

    if (pos.m_entry_order->m_payload->m_status != Status::PENDING)
    {
        throw std::string("Cancel can only be done on PENDING orders");
    }

    orderbook.rtrack(pos.m_entry_order);
}

void FuturesEngine::modify_order(std::shared_ptr<ModifyOrderPayload> &&payloadp)
{
    OrderBook &orderbook = this->orderbooks.at(payloadp->m_instrument);
    Position &position = orderbook.get_position(payloadp->m_id);

    if (payloadp->m_stop_loss_price == 0.0f)
    {
        if (position.m_stop_loss_order)
        {
            orderbook.remove_from_level(position.m_stop_loss_order);
        }
    }
    else
    {
        // if (position.m_entry_order->m_payload->m_stop_loss_order != nullptr)
        // {
        //     if (payloadp->m_stop_loss_price != position.m_entry_order->m_payload->m_stop_loss_order->m_price)
        //     {
        //         orderbook.remove_from_level(*position.m_stop_loss_order);
        //         delete position.m_stop_loss_order;
        //     }

        //     position.m_entry_order->m_payload->m_stop_loss_order->m_price = payloadp->m_stop_loss_price;
        // }
        // else
        // {
        //     position.m_entry_order->m_payload->m_stop_loss_order = std::make_unique<StopLossOrder>(payloadp->m_stop_loss_price);
        // }

        // position.m_stop_loss_order = new Order(position.m_entry_order->m_payload, Order::Tag::STOP_LOSS);
        // orderbook.push_order(*position.m_stop_loss_order);
    }

    if (payloadp->m_take_profit_price == 0.0f)
    {
        if (position.m_take_profit_order)
        {
            orderbook.remove_from_level(position.m_take_profit_order);
        }
    }
    else
    {
        if (position.m_entry_order->m_payload->m_take_profit_price != 0.0f)
        {
            if (payloadp->m_take_profit_price != position.m_entry_order->m_payload->m_take_profit_price)
            {
                orderbook.remove_from_level(position.m_take_profit_order);
                position.m_take_profit_order = nullptr;
            }

            position.m_entry_order->m_payload->m_take_profit_price = payloadp->m_take_profit_price;
            position.m_take_profit_order = std::make_shared<Order>(Tag::TAKE_PROFIT, position.m_entry_order->m_payload);
            orderbook.push_order(position.m_take_profit_order);
        }
    }

    if (
        payloadp->m_entry_price == 0.0f ||
        position.m_entry_order->m_payload->m_order_type != OrderType::LIMIT)
    {
        return;
    }

    if (payloadp->m_entry_price != position.m_entry_order->m_payload->m_entry_price)
    {
        orderbook.remove_from_level(position.m_entry_order);
        position.m_entry_order = nullptr;
        position.m_entry_order->m_payload->m_entry_price = payloadp->m_entry_price;
        position.m_entry_order = std::make_shared<Order>(Tag::ENTRY, position.m_entry_order->m_payload);
        orderbook.push_order(position.m_entry_order);
    }
}

void FuturesEngine::close_order(std::shared_ptr<BasePayload> &payload)
{
    OrderBook &orderbook = this->orderbooks.at(payload->m_instrument);
    Position &pos = orderbook.get_position(payload->m_id);
    orderbook.rtrack(pos.m_entry_order);
}

const MatchResult FuturesEngine::match_gtc(Order &order, OrderBook &orderbook)
{
    const int og_standing_quantity = order.m_payload->m_standing_quantity;
    std::vector<std::pair<std::shared_ptr<Order> &, int>> touched;
    std::vector<std::pair<std::shared_ptr<Order> &, int>> filled;
    const float best_price = orderbook.get_best_price((order.m_payload->m_side == Side::ASK) ? Side::BID : Side::ASK);
    MatchResult result;
    // MatchResult result;

    // auto &book = orderbook.get_book(order);

    // for (Order *&ex_order_p : book[best_price])
    // for (Order *&ex_order_p : orderbook.get_book(order)[best_price])
    for (std::shared_ptr<Order> &ex_orderp : orderbook.get_book(order)[best_price])
    {
        const int ex_order_pre_standing_quantity = ex_orderp->m_payload->m_standing_quantity;
        const int reduce_amount = std::min(order.m_payload->m_standing_quantity, ex_order_pre_standing_quantity);
        ex_orderp->m_payload->m_standing_quantity -= reduce_amount;
        order.m_payload->m_standing_quantity -= reduce_amount;

        if (ex_orderp->m_payload->m_standing_quantity == 0)
        {
            filled.push_back(std::pair<std::shared_ptr<Order> &, int>{ex_orderp, ex_order_pre_standing_quantity});
        }
        else
        {
            touched.push_back(std::pair<std::shared_ptr<Order> &, int>{ex_orderp, ex_order_pre_standing_quantity});
        }

        if (order.m_payload->m_standing_quantity == 0)
        {
            break;
        }
    }

    if (!touched.empty())
    {
        handle_touched_orders(touched, orderbook, best_price);
    }

    if (!filled.empty())
    {
        handle_filled_orders(filled, orderbook, best_price);
    }

    return gen_match_result(og_standing_quantity, order, best_price);
}

// Only to be used on Tag::ENTRY orders
const MatchResult FuturesEngine::match_fok(Order &order, OrderBook &orderbook)
{
    const int og_standing_quantity = order.m_payload->m_standing_quantity;
    auto &book = orderbook.get_book(order);
    // std::vector<std::tuple<Order *&, int, int>> touched_order_collection;
    std::vector<std::tuple<std::shared_ptr<Order> &, int, int>> touched_order_collection;

    for (std::shared_ptr<Order> &ex_orderp : book[order.m_payload->m_entry_price])
    {
        const int ex_order_pre_standing_quantity = ex_orderp->m_payload->m_standing_quantity;
        const int reduce_amount = std::min(order.m_payload->m_standing_quantity, ex_order_pre_standing_quantity);
        order.m_payload->m_standing_quantity -= reduce_amount;
        touched_order_collection.push_back(std::tuple<std::shared_ptr<Order> &, int, int>{ex_orderp, ex_order_pre_standing_quantity, reduce_amount});

        if (order.m_payload->m_standing_quantity == 0)
        {
            break;
        }
    }

    if (order.m_payload->m_standing_quantity == 0)
    {
        std::vector<std::pair<std::shared_ptr<Order> &, int>> touched;
        std::vector<std::pair<std::shared_ptr<Order> &, int>> filled;

        for (auto &tup : touched_order_collection)
        {
            std::shared_ptr<Order> &ex_order = std::get<0>(tup);
            ex_order->m_payload->m_standing_quantity -= std::get<2>(tup);

            if (ex_order->m_payload->m_standing_quantity == 0)
            {
                filled.push_back(std::pair<std::shared_ptr<Order> &, int>{ex_order, std::get<1>(tup)});
            }
            else
            {
                touched.push_back(std::pair<std::shared_ptr<Order> &, int>{ex_order, std::get<1>(tup)});
            }
        }

        if (!touched.empty())
        {
            handle_touched_orders(touched, orderbook, order.m_payload->m_entry_price);
        }

        if (!filled.empty())
        {
            handle_filled_orders(filled, orderbook, order.m_payload->m_entry_price);
        }
    }

    return gen_match_result(og_standing_quantity, order, order.m_payload->m_entry_price);
}

const MatchResult FuturesEngine::gen_match_result(const float og_standing_quantity, Order &order, const float price)
{
    MatchResult result;

    if (order.m_payload->m_standing_quantity == 0)
    {
        // result.set_result_type(MatchResult::MatchResultType::SUCCESS);
        result.m_result_type = MatchResult::MatchResultType::SUCCESS;
        result.m_price = price;
    }
    else if (order.m_payload->m_standing_quantity == og_standing_quantity)
    {
        result.m_result_type = MatchResult::MatchResultType::FAILURE;
    }
    else
    {
        result.m_result_type = MatchResult::MatchResultType::PARTIAL;
    }

    return result;
}

// void FuturesEngine::handle_filled_orders(std::vector<std::pair<Order *, int>> &orders, OrderBook &orderbook, const float price)
void FuturesEngine::handle_filled_orders(std::vector<std::pair<std::shared_ptr<Order> &, int>> &orders, OrderBook &orderbook, const float price)
{
    for (auto &pair : orders)
    {
        Order &order = *pair.first;

        if (order.m_tag == Tag::ENTRY)
        {
            orderbook.remove_from_level(pair.first);
            order.m_payload->m_standing_quantity = order.m_payload->m_quantity;
            // order.m_payload->set_status(Status::FILLED);
            // order.m_payload->set_filled_price(price);
            order.m_payload->m_status = Status::FILLED;
            order.m_payload->m_filled_price = price;
            place_tp_sl(order, orderbook);
        }
        else
        {
            calc_upl(order, pair.second, price);
            orderbook.rtrack(orderbook.get_position(order.m_payload->m_id).m_entry_order);
        }
    }
}

// void FuturesEngine::handle_touched_orders(std::vector<std::pair<Order *, int>> &orders, OrderBook &orderbook, const float price)
void FuturesEngine::handle_touched_orders(std::vector<std::pair<std::shared_ptr<Order> &, int>> &orders, OrderBook &orderbook, const float price)
{
    for (auto &pair : orders)
    {
        Order &order = *pair.first;

        if (order.m_tag == Tag::ENTRY)
        {
            // order.m_payload->set_status(Status::PARTIALLY_FILLED);
            order.m_payload->m_status = Status::PARTIALLY_FILLED;
        }
        else
        {
            calc_upl(order, pair.second, price);

            if (order.m_payload->m_status == Status::CLOSED)
            {
                orderbook.rtrack(orderbook.get_position(order.m_payload->m_id).m_entry_order);
            }
            else
            {
                order.m_payload->m_status = Status::PARTIALLY_CLOSED;
            }
        }
    }
}

void FuturesEngine::place_tp_sl(Order &order, OrderBook &orderbook)
{
    Position &pos = orderbook.get_position(order.m_payload->m_id);

    if (order.m_payload->m_take_profit_price != 0.0f)
    {
        std::shared_ptr<Order> tp_order = std::make_shared<Order>(Tag::TAKE_PROFIT, order.m_payload);
        orderbook.track(tp_order);
        orderbook.push_order((tp_order));
    }

    if (order.m_payload->m_stop_loss_order.m_price != 0.0f ||
        order.m_payload->m_stop_loss_order.m_distance != 0.0f)
    {
        std::shared_ptr<Order> sl_order = std::make_shared<Order>(order.m_payload, Tag::STOP_LOSS);
        orderbook.track(sl_order);
        orderbook.push_order(sl_order);
    }
}
