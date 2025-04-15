#include <chrono>
#include <iostream>
#include <memory>
#include <numeric>
#include <thread>
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
      m_price(price) {};

MatchResult::MatchResult() : m_price(-1) {};

FuturesEngine::FuturesEngine() : m_handlers(std::map<QueuePayload::Category, std::function<void(std::unique_ptr<BasePayload>)>>{
                                     {QueuePayload::Category::NEW, [this](std::unique_ptr<BasePayload> param)
                                      { handle_new_order(std::move(param)); }},
                                     {QueuePayload::Category::CANCEL, [this](std::unique_ptr<BasePayload> param)
                                      { handle_cancel_order(std::move(param)); }},
                                     {QueuePayload::Category::MODIFY, [this](std::unique_ptr<BasePayload> param)
                                      { handle_modify_order(std::move(param)); }},
                                     {QueuePayload::Category::CLOSE, [this](std::unique_ptr<BasePayload> param)
                                      { handle_close_order(std::move(param)); }},
                                 }) {};

void FuturesEngine::start(Queue &queue) noexcept
{
    this->m_orderbooks.emplace("APPL", OrderBook("APPL", 50.0f));
    const std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();

    for (int _ = 0; _ < LOOPS; _++)
    // while (true)
    {
        QueuePayload message = queue.get();
        m_handlers.at(message.m_category)(std::move(message.m_payload));
    }

    const double tts = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start_time).count();

    std::cout << "Time Taken (seconds): " << tts << "\n";
    std::cout << "Time Taken (nano-seconds): " << tts * 1e9 << "\n";
    std::cout << "Average Time (nano-seconds): " << tts / LOOPS * 1e9 << std::endl;

    // std::cout << "\n";

    // OrderBook &ob = this->orderbooks.at("APPL");
    // std::cout << "\n\tAsks\n";
    // ob.print_size(ob.m_asks);
    // std::cout << "\n\n\tBids\n";
    // ob.print_size(ob.m_bids);
    // std::cout << "\n";
    // std::cout << "Tracker Size: " << ob.m_tracker.size();
}

void FuturesEngine::handle_new_order(std::unique_ptr<BasePayload> message_payload) noexcept
{
    std::shared_ptr<OrderPayload> payload(static_cast<OrderPayload *>(message_payload.release()));

    if (payload->m_order_type == OrderType::LIMIT ||
        payload->m_order_type == OrderType::STOP)
    {
        place_limit_order(std::move(payload));
    }
    else if (payload->m_exec_type == ExecutionType::GTC)
    {
        place_gtc_market_order(std::move(payload));
    }
    else
    {
        place_fok_order(std::move(payload));
    }
}

void FuturesEngine::handle_cancel_order(std::unique_ptr<BasePayload> message_payload) noexcept
{
    cancel_order(std::move(message_payload));
}

void FuturesEngine::handle_modify_order(std::unique_ptr<BasePayload> message_payload) noexcept
{
    modify_order(*static_cast<ModifyOrderPayload *>(message_payload.release()));
}

void FuturesEngine::handle_close_order(std::unique_ptr<BasePayload> message_payload) noexcept
{
    close_order(message_payload);
}

void FuturesEngine::place_fok_order(std::shared_ptr<OrderPayload> &&payloadp) noexcept
{
    OrderBook &orderbook = this->m_orderbooks.at(payloadp->m_instrument);
    Position &position = orderbook.declare(payloadp);
    const MatchResult result = match_fok(*position.m_entry_order, orderbook);

    if (result.m_result_type == MatchResult::MatchResultType::SUCCESS)
    {
        handle_successfull_match(*position.m_entry_order, orderbook, result.m_price);
        orderbook.set_price(result.m_price);
    }
    else
    {
        orderbook.rtrack(position.m_entry_order);
    }
}

void FuturesEngine::place_gtc_market_order(std::shared_ptr<OrderPayload> payloadp) noexcept
{
    OrderBook &orderbook = this->m_orderbooks.at(payloadp->m_instrument);
    Position &position = orderbook.declare(payloadp);
    Order &order = *position.m_entry_order;
    const MatchResult result = match_gtc(order, orderbook);

    if (result.m_result_type == MatchResult::MatchResultType::SUCCESS)
    {
        handle_successfull_match(order, orderbook, result.m_price);
        orderbook.set_price(result.m_price);
    }
    else
    {
        if (result.m_result_type == MatchResult::MatchResultType::PARTIAL)
        {
            order.m_payload->m_status = OrderStatus::PARTIALLY_FILLED;
        }

        orderbook.push_order(position.m_entry_order);
    }
}

void FuturesEngine::place_ioc_order(std::shared_ptr<OrderPayload> payloadp) noexcept
{
    OrderBook &orderbook = this->m_orderbooks.at(payloadp->m_instrument);
    Position &position = orderbook.declare(payloadp);
    Order &order = *position.m_entry_order;
    const MatchResult result = match_gtc(order, orderbook);

    if (order.m_payload->m_quantity != order.m_payload->m_standing_quantity)
    {
        handle_successfull_match(order, orderbook, result.m_price, false);
        order.m_payload->m_standing_quantity = order.m_payload->m_quantity - order.m_payload->m_standing_quantity;
        orderbook.set_price(result.m_price);
    }
    else
    {
        orderbook.rtrack(position.m_entry_order);
    }
}

void FuturesEngine::place_limit_order(std::shared_ptr<OrderPayload> payloadp) noexcept
{
    OrderBook &orderbook = this->m_orderbooks.at(payloadp->m_instrument);
    orderbook.push_order(orderbook.declare(payloadp).m_entry_order);
}

void FuturesEngine::cancel_order(const std::unique_ptr<BasePayload> payload)
{
    OrderBook &orderbook = this->m_orderbooks.at(payload->m_instrument);
    Position &pos = orderbook.get_position(payload->m_id);

    if (pos.m_entry_order->m_payload->m_status != OrderStatus::PENDING)
    {
        throw std::string("Cancel can only be done on Status::PENDING orders.");
    }

    orderbook.rtrack(pos.m_entry_order);
}

void FuturesEngine::modify_order(const ModifyOrderPayload &payload) noexcept
{
    OrderBook &orderbook = this->m_orderbooks.at(payload.m_instrument);
    Position &position = orderbook.get_position(payload.m_id);
    modify_entry(payload, position, orderbook);
    modify_stop_loss(payload, position, orderbook);
    modify_take_profit(payload, position, orderbook);
}

void FuturesEngine::close_order(const std::unique_ptr<BasePayload> &payload) noexcept
{
    OrderBook &orderbook = this->m_orderbooks.at(payload->m_instrument);
    orderbook.rtrack(orderbook.get_position(payload->m_id).m_entry_order);
}

// Only call with Tag::ENTRY order
const MatchResult FuturesEngine::match_fok(Order &order, OrderBook &orderbook) noexcept
{
    const int og_standing_quantity = order.m_payload->m_standing_quantity;
    const float best_price = orderbook.get_best_price((order.m_payload->m_side == Side::ASK) ? Side::BID : Side::ASK);
    std::vector<std::pair<std::shared_ptr<Order> &, int>> touched;
    std::vector<std::pair<std::shared_ptr<Order> &, int>> filled;
    std::vector<std::tuple<std::shared_ptr<Order> &, int, int>> touched_order_collection;

    for (std::shared_ptr<Order> &ex_orderp : orderbook.get_book(order)[best_price])
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
            handle_touched_orders(touched, orderbook, best_price);
        }

        if (!filled.empty())
        {
            handle_filled_orders(filled, orderbook, best_price);
        }
    }

    return gen_match_result(og_standing_quantity, order, best_price);
}

const MatchResult FuturesEngine::match_gtc(Order &order, OrderBook &orderbook) noexcept
{
    const int og_standing_quantity = order.m_payload->m_standing_quantity;
    std::vector<std::pair<std::shared_ptr<Order> &, int>> touched;
    std::vector<std::pair<std::shared_ptr<Order> &, int>> filled;
    const float best_price = orderbook.get_best_price((order.m_payload->m_side == Side::ASK) ? Side::BID : Side::ASK);

    try
    {

        for (std::shared_ptr<Order> &ex_orderp : orderbook.get_book(order).at(best_price))
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

        if (touched.size() > 1 || filled.size() > 1)
            std::this_thread::sleep_for(std::chrono::seconds(3));

        if (!touched.empty())
        {
            handle_touched_orders(touched, orderbook, best_price);
        }

        if (!filled.empty())
        {
            handle_filled_orders(filled, orderbook, best_price);
        }
    }
    catch (const std::out_of_range &e)
    {
    }

    return gen_match_result(og_standing_quantity, order, best_price);
}

void FuturesEngine::handle_filled_orders(std::vector<std::pair<std::shared_ptr<Order> &, int>> &orders, OrderBook &orderbook, const float price) noexcept
{
    for (auto &pair : orders)
    {
        Order &order = *pair.first;

        if (order.m_tag == Tag::ENTRY)
        {
            orderbook.remove_from_level(pair.first);
            handle_successfull_match(order, orderbook, price, false);
        }
        else
        {
            calc_upl(*order.m_payload, pair.second, price);
            orderbook.rtrack(orderbook.get_position(order.m_payload->m_id).m_entry_order);
        }
    }
}

void FuturesEngine::handle_touched_orders(std::vector<std::pair<std::shared_ptr<Order> &, int>> &orders, OrderBook &orderbook, const float price) noexcept
{
    for (auto &pair : orders)
    {
        Order &order = *pair.first;

        if (order.m_tag == Tag::ENTRY)
        {
            order.m_payload->m_status = OrderStatus::PARTIALLY_FILLED;
        }
        else
        {
            calc_upl(*order.m_payload, pair.second, price);

            if (order.m_payload->m_status == OrderStatus::CLOSED)
            {
                orderbook.rtrack(orderbook.get_position(order.m_payload->m_id).m_entry_order);
            }
            else
            {
                order.m_payload->m_status = OrderStatus::PARTIALLY_CLOSED;
            }
        }
    }
}

void FuturesEngine::place_tp_sl(Order &order, OrderBook &orderbook) noexcept
{
    Position &pos = orderbook.get_position(order.m_payload->m_id);

    if (order.m_payload->m_take_profit_price != -1.0f)
    {
        std::shared_ptr<Order> tp_order = std::make_shared<Order>(Tag::TAKE_PROFIT, order.m_payload);
        orderbook.track(tp_order);
        orderbook.push_order((tp_order));
    }

    if (order.m_payload->m_stop_loss_details.m_price > -1.0f ||
        order.m_payload->m_stop_loss_details.m_distance > -1.0f)
    {
        std::shared_ptr<Order> sl_order = std::make_shared<Order>(Tag::STOP_LOSS, order.m_payload);
        orderbook.track(sl_order);
        orderbook.push_order(sl_order);
    }
}

const MatchResult FuturesEngine::gen_match_result(const float og_standing_quantity, Order &order, const float price) noexcept
{
    MatchResult result;

    if (order.m_payload->m_standing_quantity == 0)
    {
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

void FuturesEngine::handle_successfull_match(Order &order, OrderBook &orderbook, float price, bool reset_standing_quantity)
{
    if (reset_standing_quantity)
    {
        order.m_payload->m_standing_quantity = order.m_payload->m_quantity;
    }

    order.m_payload->m_status = OrderStatus::FILLED;
    order.m_payload->m_filled_price = price;
    place_tp_sl(order, orderbook);
}

void FuturesEngine::modify_entry(const ModifyOrderPayload &payload, Position &position, OrderBook &orderbook)
{
    if (position.m_entry_order->m_payload->m_status != OrderStatus::PENDING)
        return;

    orderbook.remove_from_level(position.m_entry_order);
    position.m_entry_order = std::make_shared<Order>(Tag::ENTRY, position.m_entry_order->m_payload);
}

void FuturesEngine::modify_stop_loss(const ModifyOrderPayload &payload, Position &position, OrderBook &orderbook) noexcept
{
    std::shared_ptr<OrderPayload> &order_payload = position.m_entry_order->m_payload;

    if (payload.m_stop_loss_distance > -1.0f)
    {
        if (payload.m_stop_loss_distance != order_payload->m_stop_loss_details.m_distance)
        {
            order_payload->m_stop_loss_details.m_distance = payload.m_stop_loss_distance;
        }

        return;
    }

    if (order_payload->m_status < OrderStatus::FILLED ||
        payload.m_stop_loss_price == order_payload->m_stop_loss_details.m_price)
        return;

    if (position.m_stop_loss_order)
        orderbook.remove_from_level(position.m_stop_loss_order);

    if (payload.m_stop_loss_price > -1.0f)
    {
        position.m_stop_loss_order = std::make_shared<Order>(Tag::STOP_LOSS, position.m_entry_order->m_payload);
        orderbook.push_order(position.m_stop_loss_order);
    }
}

void FuturesEngine::modify_take_profit(const ModifyOrderPayload &payload, Position &position, OrderBook &orderbook)
{
    std::shared_ptr<OrderPayload> &order_payload = position.m_entry_order->m_payload;

    if (order_payload->m_status < OrderStatus::FILLED ||
        payload.m_take_profit_price == order_payload->m_take_profit_price)
        return;

    if (position.m_take_profit_order)
        orderbook.remove_from_level(position.m_take_profit_order);

    if (payload.m_take_profit_price > -1.0f)
    {
        position.m_take_profit_order = std::make_shared<Order>(Tag::TAKE_PROFIT, position.m_entry_order->m_payload);
        orderbook.push_order(position.m_take_profit_order);
    }
}