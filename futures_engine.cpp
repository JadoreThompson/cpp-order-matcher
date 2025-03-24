#include "futures_engine.h"
#include "order.h"
#include "orderbook.h"

void FuturesEngine::match(Order &order, OrderBook &orderbook)
{
    std::map<float, std::list<Order>> &book = orderbook.get_opposite_book(order);
    const int og_standing_quantity = order.payload.standing_quantity;
    std::list<Order> touched;
    std::list<Order> filled;
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
            order.payload.set_status(CLOSED);
            orderbook.rtrack(orderbook.get_position(order.payload.id).entry_order);
        }
    }
}

void FuturesEngine::place_tp_sl(Order &order, OrderBook &orderbook)
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