#ifndef _ORDER_
#define _ORDER_

#include <optional>
#include <string>
#include <memory>

enum Side
{
    BID,
    ASK
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
    float filled_price;
    bool filled_price_set;

public:
    const int id;
    const OrderType order_type;
    const Side side;
    const std::string instrument;
    const int quantity;
    int standing_quantity;
    float entry_price;
    float *stop_loss_price;
    float *take_profit_price;
    float closed_price;
    float realised_pnl;
    float unrealised_pnl;

    OrderPayload(
        const int id_,
        const OrderType order_type_,
        const Side side_,
        const std::string instrument,
        const int quantity_,
        float entry_price_,
        float *stop_loss_price_ = nullptr,
        float *take_profit_price_ = nullptr);
    void set_status(Status status_);
    Status &get_status();

    void set_filled_price(float price);
    float get_filled_price();

};

class Order
{
public:
    const Tag tag;
    std::shared_ptr<OrderPayload> payload;
    Order(std::shared_ptr<OrderPayload> payload_, const Tag tag_);
    bool operator==(const Order &other) const;
};

class Position
{
public:
    Order *entry_order;
    Order *stop_loss_order = nullptr;
    Order *take_profit_order = nullptr;
    Position(Order *entry_order_);
};
#endif
