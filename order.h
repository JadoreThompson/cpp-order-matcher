#ifndef _ORDER_
#define _ORDER_
#include <optional>

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

struct OrderPayload
{
private:
    Status status;

public:
    const int id;
    const Side side;
    const int quantity;
    int standing_quantity;
    float entry_price;
    float *filled_price;
    float *stop_loss_price;
    float *take_profit_price;

    OrderPayload(
        const int id_,
        const Side side_,
        const int quantity_,
        float entry_price_,
        float *stop_loss_price_ = nullptr,
        float *take_profit_price_ = nullptr);
    void set_status(Status status_);
    Status &get_status();
};

class Order
{
public:
    const Tag tag;
    OrderPayload &payload;
    Order(OrderPayload &payload_, const Tag tag_);
};

class Position
{
public:
    Order &entry_order;
    Order *stop_loss_order;
    Order *take_profit_order;
    Position(Order &entry_order_);
};
#endif