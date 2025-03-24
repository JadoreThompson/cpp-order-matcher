#ifndef _ORDER_
#define _ORDER_

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
    int quantity;
    const float entry_price;
    const Tag tag;

    OrderPayload(int quantity_, float entry_price_);
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
#endif