#include "order.h"
#include <string>

OrderPayload::OrderPayload(
    const int id_,
    const Side side_,
    const int quantity_,
    float entry_price_,
    float *stop_loss_price_,
    float *take_profit_price_)
    : id(id_),
      side(side_),
      quantity(quantity_),
      standing_quantity(quantity_),
      entry_price(entry_price_),
      filled_price(nullptr),
      stop_loss_price(stop_loss_price_),
      take_profit_price(take_profit_price_),
      status(PENDING) {};

void OrderPayload::set_status(Status status_)
{
    if (this->status == CLOSED)
    {
        throw std::string("Cannot set status on payload with status CLOSED");
    }
}

Status &OrderPayload::get_status()
{
    return this->status;
}

Order::Order(OrderPayload &payload_,  const Tag tag_) : payload(payload_), tag(tag_) {};

Position::Position(Order &entry_order_)
    : entry_order(entry_order_),
      stop_loss_order(nullptr),
      take_profit_order(nullptr) {};
