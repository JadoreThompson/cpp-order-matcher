#include "order.h"
#include <string>
#include <iostream>

BasePayload::BasePayload(const int id, const std::string instrument)
    : m_id(id), m_instrument(instrument) {};

StopLossOrder::StopLossOrder(float price, float distance) : m_price(price), m_distance(distance) {};

NewOrderPayload::NewOrderPayload(
    const int id,
    const std::string instrument,
    const OrderType order_type,
    const Side side,
    const int quantity,
    float entry_price,
    const ExecutionType exec_type,
    // float stop_loss_price,
    // StopLossOrder stop_loss_order,
    std::unique_ptr<StopLossOrder> stop_loss_order,
    float take_profit_price)
    : BasePayload(id, instrument),
      m_exec_type(exec_type),
      m_order_type(order_type),
      m_side(side),
      m_quantity(quantity),
      m_standing_quantity(quantity),
      m_entry_price(entry_price),
      //   m_stop_loss_price(stop_loss_price),
      //   m_stop_loss(std::make_shared<StopLossOrder>(stop_loss)),
      m_stop_loss_order(std::move(stop_loss_order)),
      m_take_profit_price(take_profit_price),
      m_realised_pnl(0.0),
      m_unrealised_pnl(0.0),
      m_status(PENDING),
      m_filled_price_set(false) {};

void NewOrderPayload::set_status(Status status)
{
    if (this->m_status == CLOSED)
    {
        throw std::string("Cannot set status on payload with status CLOSED");
    }
    this->m_status = status;
}

NewOrderPayload::Status &NewOrderPayload::get_status()
{
    return this->m_status;
}

void NewOrderPayload::set_filled_price(float price)
{
    if (this->m_filled_price_set)
    {
        throw std::string("Cannot set filled price on payload once already set");
    }

    this->m_filled_price = price;
    this->m_filled_price_set = true;
}

float NewOrderPayload::get_filled_price()
{
    if (!this->m_filled_price_set)
    {
        throw std::string("Filled price not set");
    }

    return this->m_filled_price;
}

CancelOrderPayload::CancelOrderPayload(const int id, const std::string instrument)
    : BasePayload(id, instrument) {};

ModifyOrderPayload::ModifyOrderPayload(
    const int id,
    const std::string instrument,
    const float stop_loss_price,
    const float take_profit_price,
    const float limit_price)
    : BasePayload(id, instrument),
      m_stop_loss_price(stop_loss_price),
      m_take_profit_price(take_profit_price),
      m_entry_price(limit_price) {};

QueuePayload::QueuePayload(
    const Category category, std::shared_ptr<BasePayload> payloadp)
    : m_category(category), m_payload(payloadp) {};

Order::Order(std::shared_ptr<NewOrderPayload> payload, const Tag tag) : m_payload(payload), m_tag(tag) {};

bool Order::operator==(const Order &other) const
{
    return this->m_payload->m_id == other.m_payload->m_id;
}

Position::Position(Order *entry_order)
    : m_entry_order(entry_order),
      m_stop_loss_order(nullptr),
      m_take_profit_order(nullptr) {};
