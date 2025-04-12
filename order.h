#ifndef _ORDER_
#define _ORDER_

#include <string>
#include <memory>

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

enum Side
{
    BID,
    ASK
};

enum Status
{
    PENDING,
    PARTIALLY_FILLED,
    FILLED,
    PARTIALLY_CLOSED,
    CLOSED
};

enum Tag
{
    ENTRY,
    STOP_LOSS,
    TAKE_PROFIT,
};

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

struct OrderPayload : public BasePayload
{
    const ExecutionType m_exec_type;
    const OrderType m_order_type;
    const Side m_side;
    const int m_quantity;
    float m_entry_price;

    Status m_status;
    float m_filled_price = -1.0f;
    float m_closed_price;
    float m_take_profit_price;
    float m_realised_pnl;
    float m_unrealised_pnl;
    int m_standing_quantity;
    // std::unique_ptr<StopLossOrder> m_stop_loss_order;
    StopLossOrder m_stop_loss_order;

    OrderPayload(
        const int id,
        const std::string instrument,
        const OrderType order_type,
        const Side side,
        const int quantity,
        float entry_price,
        const ExecutionType exec_type = GTC,
        // std::unique_ptr<StopLossOrder> stop_loss_order = nullptr,
        StopLossOrder stop_loss_order = {},
        float take_profit_price = 0.0f);

    // void set_status(Status status);

    // Status &get_status();

    // void set_filled_price(float price);

    // float get_filled_price();
};

struct CancelOrderPayload : public BasePayload
{
public:
    CancelOrderPayload(const int id, const std::string instrument);
};

struct ModifyOrderPayload : public BasePayload
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
public:
    enum Category
    {
        NEW,
        CANCEL,
        MODIFY,
        CLOSE,
        _UNKNOWN
    };

    Category m_category;
    std::unique_ptr<BasePayload> m_payload;

    QueuePayload(const Category category, std::unique_ptr<BasePayload> &&payloadp);

    QueuePayload(QueuePayload &&other);

    QueuePayload &operator=(const QueuePayload &&other);

    ~QueuePayload();

private:
    QueuePayload(QueuePayload &other);

    QueuePayload &operator=(const QueuePayload &other);
};

// struct BaseOrder
// {
// public:
//     const Tag m_tag;
//     BaseOrder(const Tag m_tag);
// };

// struct EntryOrder : public BaseOrder
// {
// public:
//     std::unique_ptr<OrderPayload> m_payload;
//     EntryOrder(const Tag m_tag, std::unique_ptr<OrderPayload> payload);
// };

// // Represents StopLoss and TakeProfit orders
// struct ExitOrder : public BaseOrder
// {
// public:
//     std::unique_ptr<OrderPayload> &m_payload;
//     ExitOrder(const Tag m_tag, std::unique_ptr<OrderPayload> &payload);
// };

class Order
{
public:
    const Tag m_tag;
    std::shared_ptr<OrderPayload> m_payload;
    Order(const Tag m_tag, std::shared_ptr<OrderPayload> payload);
};

class Position
{
public:
    // Order *m_entry_order;
    // Order *m_stop_loss_order = nullptr;
    // Order *m_take_profit_order = nullptr;

    // std::unique_ptr<EntryOrder> m_entry_order;
    // std::unique_ptr<ExitOrder> m_stop_loss_order = nullptr;
    // std::unique_ptr<ExitOrder> m_take_profit_order = nullptr;

    std::shared_ptr<Order> m_entry_order;
    std::shared_ptr<Order> m_stop_loss_order = nullptr;
    std::shared_ptr<Order> m_take_profit_order = nullptr;

    // Position(Order *entry_order);
    Position(std::shared_ptr<Order> entry_order);
};

#endif