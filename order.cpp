#include "order.h"
#include <string>
#include <iostream>

QueuePayload::QueuePayload(const Category category_, std::shared_ptr<BasePayload> payloadp_) : category(category_), payload(payloadp_) {};

NewOrderPayload::NewOrderPayload(
    const int id_,
    const ExecutionType exec_type_,
    const OrderType order_type_,
    const Side side_,
    const std::string instrument_,
    const int quantity_,
    float entry_price_,
    float *stop_loss_price_,
    float *take_profit_price_)
    : BasePayload(id_, instrument_),
        exec_type(exec_type_),
      order_type(order_type),
      side(side_),
      quantity(quantity_),
      standing_quantity(quantity_),
      entry_price(entry_price_),
      stop_loss_price(stop_loss_price_),
      take_profit_price(take_profit_price_),
      realised_pnl(0.0),
      unrealised_pnl(0.0),
      status(PENDING),
      filled_price_set(false) {};

void NewOrderPayload::set_status(Status status_)
{
    if (this->status == CLOSED)
    {
        throw std::string("Cannot set status on payload with status CLOSED");
    }
    this->status = status;
}

NewOrderPayload::Status &NewOrderPayload::get_status()
{
    return this->status;
}

void NewOrderPayload::set_filled_price(float price)
{
    if (this->filled_price_set)
    {
        throw std::string("Cannot set filled price on payload once already set");
    }

    this->filled_price = price;
    this->filled_price_set = true;
}

float NewOrderPayload::get_filled_price()
{
    if (!this->filled_price_set)
    {
        throw std::string("Filled price not set");
    }

    return this->filled_price;
}

CancelOrderPayload::CancelOrderPayload(const int id_, const std::string instrument_)
    : BasePayload(id_, instrument_) {};

ModifyOrderPayload::ModifyOrderPayload(
    const int id_,
    const std::string instrument_,
    const float stop_loss_price_,
    const float take_profit_price_,
    const float limit_price_)
    : BasePayload(id_, instrument_),
      stop_loss_price(stop_loss_price_),
      take_profit_price(take_profit_price_),
      entry_price(limit_price_) {};

Order::Order(std::shared_ptr<NewOrderPayload> payload_, const Tag tag_) : payload(payload_), tag(tag_) {};

bool Order::operator==(const Order &other) const
{
    return this->payload->id == other.payload->id;
}

Position::Position(Order *entry_order_)
    : entry_order(entry_order_),
      stop_loss_order(nullptr),
      take_profit_order(nullptr) {};
