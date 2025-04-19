#ifndef _ORDER_
#define _ORDER_

#include <string>
#include <memory>

enum ExecutionType
{
    FOK = 0,
    GTC = 1,
    IOC = 2
};

enum OrderType
{
    MARKET = 0,
    LIMIT = 1,
    STOP = 2
};

enum Side
{
    BID = 0,
    ASK = 1
};

enum OrderStatus
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

    virtual ~BasePayload() noexcept {};
};

struct StopLossOrder
{
    float m_price;
    float m_distance;
    StopLossOrder(float price = -1.0f, float distance = -1.0f);
};

struct OrderPayload : public BasePayload
{
    const ExecutionType m_exec_type;
    const OrderType m_order_type;
    const Side m_side;
    const int m_quantity;
    float m_entry_price;

    OrderStatus m_status;
    float m_filled_price{-1.0f};
    float m_closed_price;
    float m_take_profit_price;
    float m_realised_pnl;
    float m_unrealised_pnl;
    int m_standing_quantity;
    StopLossOrder m_stop_loss_details;

    OrderPayload(
        const int id,
        const std::string instrument,
        const ExecutionType exec_type,
        const OrderType order_type,
        const Side side,
        const int quantity,
        float entry_price,
        StopLossOrder stop_loss_details = {},
        float take_profit_price = -1.0f);

    std::string to_string() const noexcept;
};

struct ModifyOrderPayload : public BasePayload
{
    const float m_stop_loss_price{-1.0f};
    const float m_stop_loss_distance{-1.0f};
    const float m_take_profit_price{-1.0f};
    const float m_limit_price{-1.0f};

    ModifyOrderPayload(
        const int id,
        const std::string instrument,
        const float m_stop_loss_distance = -1.0f,
        const float stop_loss_price = -1.0f,
        const float take_profit_price = -1.0f,
        const float limit_price = -1.0f);
};

struct QueuePayload
{
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

    QueuePayload(const Category category, std::unique_ptr<BasePayload> &&payloadp) noexcept;

    QueuePayload(QueuePayload &&other) noexcept;

    QueuePayload &operator=(QueuePayload &&other) noexcept;

    QueuePayload(QueuePayload &other) = delete;

    QueuePayload &operator=(const QueuePayload &other) = delete;

    ~QueuePayload() noexcept;
};

struct Order
{
    const Tag m_tag;
    std::shared_ptr<OrderPayload> m_payload;

    Order(const Tag m_tag, std::shared_ptr<OrderPayload> payload);

    std::string to_string() const noexcept;
};

struct Position
{
    std::shared_ptr<Order> m_entry_order;
    std::shared_ptr<Order> m_stop_loss_order{nullptr};
    std::shared_ptr<Order> m_take_profit_order{nullptr};

    Position(std::shared_ptr<Order> entry_order);
};

#endif