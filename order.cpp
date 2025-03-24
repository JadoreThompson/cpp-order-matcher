#include "order.h"
#include <string>

OrderPayload::OrderPayload(int quantity_, float entry_price_, Tag tag_) : quantity(quantity_), entry_price(entry_price_), tag(tag_), status(PENDING) {};

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