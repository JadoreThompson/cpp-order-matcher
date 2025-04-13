#include <iostream>
#include <iomanip>
#include <thread>
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
std::map<float, std::list<std::shared_ptr<Order>>> &OrderBook::get_book(const Order &order)
{
    if (order.m_tag == Tag::ENTRY)
    {
        return (order.m_payload->m_side == Side::ASK) ? this->m_bids : this->m_asks;
    }

    return (order.m_payload->m_side == Side::ASK) ? this->m_bids : this->m_asks;
}

// Specifically used for ENTRY orders.
Position &OrderBook::declare(std::shared_ptr<OrderPayload> payload) noexcept
{
    return this->m_tracker.emplace(payload->m_id, std::make_shared<Order>(Tag::ENTRY, payload)).first->second;
}

// Used for STOP_LOSS and TAKE_PROFIT orders.
void OrderBook::track(std::shared_ptr<Order> order)
{
    Position &position = this->m_tracker.at(order->m_payload->m_id);

    if (order->m_tag == Tag::ENTRY)
    {
        throw std::string("Position already exists.");
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
void OrderBook::rtrack(std::shared_ptr<Order> &order) noexcept
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

            if (order->m_payload->m_stop_loss_details.m_distance > -1.0f)
            {
                this->m_trailing_stop_loss_orders.erase(order);
            }
        }

        if (order->m_payload->m_status == Status::PENDING)
        {
            remove_from_level(order);
        }

        this->m_tracker.erase(order->m_payload->m_id);
    }
}

Position &OrderBook::get_position(const int id)
{
    return this->m_tracker.at(id);
}

const float OrderBook::get_price() noexcept { return this->m_price; }

void OrderBook::set_price(float price)
{
    if (price < 0.0f)
        throw std::invalid_argument("Cannot set price below 0.");

    this->m_price = price;
    update_trailing_stop_loss_orders(price);
    this->m_last_price = price;
}

const float OrderBook::get_best_price(Side side) noexcept
{
    float price;
    std::map<float, std::list<std::shared_ptr<Order>>> book;

    if (side == Side::ASK)
    {
        price = (this->m_asks.empty()) ? this->m_price : this->m_asks.begin()->first;
        book = this->m_asks;
    }
    else
    {
        price = (this->m_bids.empty()) ? this->m_price : this->m_bids.rbegin()->first;
        book = this->m_bids;
    }

    return price;
}

void OrderBook::push_order(std::shared_ptr<Order> &order) noexcept
{
    auto &book = get_book(*order);
    float price;

    if (order->m_tag == Tag::ENTRY)
    {
        price = order->m_payload->m_entry_price;
    }
    else if (order->m_tag == Tag::TAKE_PROFIT)
    {
        price = order->m_payload->m_take_profit_price;
    }
    else if (order->m_payload->m_stop_loss_details.m_distance > -1.0f)
    {
        if (order->m_payload->m_side == Side::BID)
        {
            price = this->m_price - order->m_payload->m_stop_loss_details.m_distance;
        }
        else
        {
            price = this->m_price - order->m_payload->m_stop_loss_details.m_distance;
        }

        this->m_trailing_stop_loss_orders.emplace(order);
    }
    else
    {
        price = order->m_payload->m_stop_loss_details.m_price;
    }

    book[price].push_back(order);
}

void OrderBook::remove_from_level(std::shared_ptr<Order> &order) noexcept
{
    OrderPayload &payload = *order->m_payload;
    auto &book = get_book(*order);
    float price;

    if (order->m_tag == Tag::ENTRY)
    {
        price = payload.m_entry_price;
    }
    else
    {

        if (order->m_tag == Tag::TAKE_PROFIT)
        {
            price = payload.m_take_profit_price;
        }
        else if (payload.m_stop_loss_details.m_distance > -1.0f)
        {
            if (payload.m_side == Side::BID)
            {
                price = this->m_last_price - payload.m_stop_loss_details.m_distance;
            }
            else
            {
                price = this->m_last_price + payload.m_stop_loss_details.m_distance;
            }
        }
        else
        {
            price = payload.m_stop_loss_details.m_price;
        }
    }

    book[price].remove(order);
    if (!book[price].size())
        book.erase(price); // Custom implementation?
}

std::pair<int, int> OrderBook::size() noexcept
{
    return std::pair<int, int>{this->m_bids.size(), this->m_asks.size()};
}

void OrderBook::print_size(const std::map<float, const std::list<std::shared_ptr<Order>>> &bids) noexcept
{
    std::cout << std::left << std::setw(10) << "Price" << " | " << "List Size\n";
    std::cout << "--------------------------\n";

    size_t totalSize = 0;
    for (const auto &[price, orderList] : bids)
    {
        std::cout << std::left << std::setw(10) << price << " | " << orderList.size() << "\n";
        totalSize += orderList.size();
    }

    std::cout << "--------------------------\n";
    std::cout << "Total Orders: " << totalSize << "\n";
}

void OrderBook::update_trailing_stop_loss_orders(float price) noexcept
{
    for (auto it = this->m_trailing_stop_loss_orders.begin(); it != this->m_trailing_stop_loss_orders.end(); ++it)
    {
        std::shared_ptr<Order> order = *it;
        remove_from_level(order);
        push_order(order);
    }
}
