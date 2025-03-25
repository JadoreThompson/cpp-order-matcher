#include "order.h"
#include <string>
#include <iostream>

OrderPayload::OrderPayload(
    const int id_,
    const OrderType order_type_,
    const Side side_,
    const std::string instrument_,
    const int quantity_,
    float entry_price_,
    float *stop_loss_price_,
    float *take_profit_price_)
    : id(id_),
      order_type(order_type),
      side(side_),
      instrument(instrument_),
      quantity(quantity_),
      standing_quantity(quantity_),
      entry_price(entry_price_),
      stop_loss_price(stop_loss_price_),
      take_profit_price(take_profit_price_),
      status(PENDING),
      filled_price_set(false) {};

void OrderPayload::set_status(Status status_)
{
    if (this->status == CLOSED)
    {
        throw std::string("Cannot set status on payload with status CLOSED");
    }
    this->status = status;
}

Status &OrderPayload::get_status()
{
    return this->status;
}

void OrderPayload::set_filled_price(float price)
{
    if (this->filled_price_set)
    {
        // std::cout << "Filled price already set to "<< std::to_string(this->filled_price) << std::endl;
        throw std::string("Cannot set filled price on payload once already set");
    }

    std::cout
        << "Setting filled price to => " << std::to_string(price)
        << " Status => " << std::to_string(this->status) << std::endl;

    this->filled_price = price;
    this->filled_price_set = true;
}

float OrderPayload::get_filled_price()
{
    if (!this->filled_price_set)
    {
        throw std::string("Filled price not set");
    }

    return this->filled_price;
}

Order::Order(OrderPayload &payload_, const Tag tag_) : payload(payload_), tag(tag_) {};

bool Order::operator==(const Order &other) const
{
    return this->payload.id == other.payload.id;
}

Position::Position(Order &entry_order_)
    : entry_order(entry_order_),
      stop_loss_order(nullptr),
      take_profit_order(nullptr) {};
