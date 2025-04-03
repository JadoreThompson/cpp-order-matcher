#ifndef _ORDER_
#define _ORDER_

#include <optional>
#include <string>
#include <memory>

struct BasePayload
{
    const int m_id;
    const std::string m_instrument;
    BasePayload(const int id, const std::string instrument);
    virtual ~BasePayload() {};
};

struct StopLossOrder
{
    float m_price;
    float m_distance;
    StopLossOrder(float price = 0.0f, float distance = 0.0f);
};

struct NewOrderPayload : public BasePayload
{
    enum Side
    {
        BID,
        ASK
    };

    enum ExecutionType
    {
        GTC,
        FOK
    };

    enum OrderType
    {
        MARKET,
        LIMIT
    };

    enum Status
    {
        PENDING,
        PARTIALLY_FILLED,
        FILLED,
        PARTIALLY_CLOSED,
        CLOSED
    };

private:
    Status m_status;
    float m_filled_price;
    bool m_filled_price_set;

public:
    const ExecutionType m_exec_type;
    const OrderType m_order_type;
    const Side m_side;
    const int m_quantity;
    int m_standing_quantity;
    float m_entry_price;
    // float stop_loss_price;
    float m_take_profit_price;
    // std::shared_ptr<StopLossOrder> m_stop_loss; // Entry, SL and TP all carry
    // StopLossOrder m_stop_loss_order;
    std::unique_ptr<StopLossOrder> m_stop_loss_order;
    float m_closed_price;
    float m_realised_pnl;
    float m_unrealised_pnl;

    NewOrderPayload(
        const int id,
        const std::string instrument,
        const OrderType order_type,
        const Side side,
        const int quantity,
        float entry_price,
        const ExecutionType exec_type = GTC,
        // float *stop_loss_price = nullptr,
        std::unique_ptr<StopLossOrder> stop_loss_order = nullptr,
        // StopLossOrder stop_loss_order,
        float take_profit_price = 0.0f);

    void set_status(Status status);

    Status &get_status();

    void set_filled_price(float price);

    float get_filled_price();
};

class CancelOrderPayload : public BasePayload
{
public:
    CancelOrderPayload(const int id, const std::string instrument);
};

class ModifyOrderPayload : public BasePayload
{
public:
    const float m_stop_loss_price;
    const float m_take_profit_price;
    const float m_entry_price;
    ModifyOrderPayload(
        const int id,
        const std::string instrument,
        const float stop_loss_price = 0.0f,
        const float take_profit_price = 0.0f,
        const float limit_price = 0.0f);
};

struct QueuePayload
{
    enum Category
    {
        NEW,
        CANCEL,
        MODIFY,
        CLOSE
    };

    const Category m_category;
    std::shared_ptr<BasePayload> m_payload;
    QueuePayload(const Category category, std::shared_ptr<BasePayload> payloadp);
};

struct Order
{
    enum Tag
    {
        ENTRY,
        STOP_LOSS,
        TAKE_PROFIT,
    };

public:
    const Tag m_tag;
    std::shared_ptr<NewOrderPayload> m_payload;
    Order(std::shared_ptr<NewOrderPayload> payload, const Tag tag);
    bool operator==(const Order &other) const;
};

class Position
{
public:
    Order *m_entry_order;
    Order *m_stop_loss_order = nullptr;
    Order *m_take_profit_order = nullptr;
    Position(Order *entry_order);
};
#endif
