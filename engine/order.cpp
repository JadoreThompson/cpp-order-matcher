#include <sstream>
#include "engine/order.h"

BasePayload::BasePayload(const int id, const std::string instrument)
    : m_id(id), m_instrument(instrument) {};

StopLossOrder::StopLossOrder(float price, float distance)
{
    if (price != -1.0f && price <= 0.0f)
        throw std::invalid_argument("Price must be greater than 0.0.");
    if (price != -1.0f && price <= 0.0f)
        throw std::invalid_argument("Distance must be greater than 0.0.");

    this->m_price = price;
    this->m_distance = distance;
};

OrderPayload::OrderPayload(
    const int id,
    const std::string instrument,
    const ExecutionType exec_type,
    const OrderType order_type,
    const Side side,
    const int quantity,
    float entry_price,
    StopLossOrder stop_loss_details,
    float take_profit_price)
    : BasePayload(id, instrument),
      m_exec_type(exec_type),
      m_order_type(order_type),
      m_side(side),
      m_quantity(quantity),
      m_standing_quantity(quantity),
      m_entry_price(entry_price),
      m_realised_pnl(0.0),
      m_unrealised_pnl(0.0),
      m_status(PENDING)
{
    if (stop_loss_details.m_price > -1.0f)
    {
        if (side == Side::ASK)
        {
            if (m_entry_price >= stop_loss_details.m_price)
                throw std::invalid_argument("Stop loss price must be greater than entry price for OrderPayload with Side::ASK.");
        }
        else if (side == Side::BID)
        {
            if (m_entry_price <= stop_loss_details.m_price)
                throw std::invalid_argument("Stop loss price must be less than entry price for OrderPayload with Side::BID.");
        }
    }

    if (take_profit_price > -1.0f)
    {
        if (side == Side::ASK)
        {
            if (m_entry_price <= take_profit_price)
                throw std::invalid_argument("Take profit price must be less than entry price for OrderPayload with Side::ASK.");
        }
        else if (side == Side::BID)
        {
            if (m_entry_price >= take_profit_price)
                throw std::invalid_argument("Take profit price must be greater than entry price for OrderPayload with Side::ASK.");
        }
    }

    this->m_stop_loss_details = stop_loss_details;
    this->m_take_profit_price = take_profit_price;
};

std::string OrderPayload::to_string() const noexcept
{
    std::ostringstream oss;
    oss << "OrderPayload("
        << "id=" << this->m_id
        << ", status=" << this->m_status
        << ", exec_type=" << this->m_exec_type
        << ", quantity=" << this->m_quantity
        << ", standing_quantity=" << this->m_standing_quantity
        << ")";
    return oss.str();
}

ModifyOrderPayload::ModifyOrderPayload(
    const int id,
    const std::string instrument,
    const float stop_loss_distance,
    const float stop_loss_price,
    const float take_profit_price,
    const float limit_price)
    : BasePayload(id, instrument),
      m_stop_loss_distance(stop_loss_distance),
      m_stop_loss_price(stop_loss_price),
      m_take_profit_price(take_profit_price),
      m_limit_price(limit_price)
{
    if (stop_loss_distance != -1.0f && stop_loss_price != -1.0f)
        throw std::invalid_argument("Must choose between distance or price.");
};

QueuePayload::QueuePayload(const Category category, std::unique_ptr<BasePayload> &&payloadp)
    : m_category(category), m_payload(std::move(payloadp)) {};

QueuePayload::QueuePayload(QueuePayload &&other)
    : m_category(other.m_category), m_payload(std::move(other.m_payload)) {

      };

QueuePayload &QueuePayload::operator=(const QueuePayload &&other)
{
    this->m_category = other.m_category;
    this->m_payload = std::make_unique<BasePayload>(*other.m_payload);
    return *this;
}

QueuePayload::~QueuePayload() {}

Order::Order(const Tag tag, std::shared_ptr<OrderPayload> payload)
    : m_tag(tag), m_payload(payload) {};

std::string Order::to_string() const noexcept
{
    std::ostringstream oss;
    oss << "Order("
        << "tag=" << this->m_tag
        << ", payload=" << this->m_payload->to_string()
        << ")";
    return oss.str();
}

Position::Position(std::shared_ptr<Order> entry_order)
    : m_entry_order(entry_order),
      m_stop_loss_order(nullptr),
      m_take_profit_order(nullptr) {};
