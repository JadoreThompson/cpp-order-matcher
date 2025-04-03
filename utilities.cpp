#include "utilities.h"

float calc_sell_pl(const float amount, const float open_price, const float close_price)
{
    return round(100 * (amount * 1 + (open_price - close_price) / open_price)) / 100;
}

float calc_buy_pl(const float amount, const float open_price, const float close_price)
{
    return round(100 * (close_price / open_price * amount)) / 100;
}

/// @brief
/// @param order
/// @param standing_quantity: Standing quantity of order pre match subtractions
/// @param price
void calc_upl(Order &order, const float standing_quantity, const float price)
{
    float filled_price;
    try
    {
        filled_price = order.m_payload->get_filled_price();
    }
    catch (const std::string &e)
    {
        return;
    }

    const float pos_value = filled_price * order.m_payload->m_standing_quantity;
    float upl;

    if (order.m_payload->m_side == NewOrderPayload::Side::ASK)
    {
        upl = calc_sell_pl(pos_value, filled_price, price);
    }
    else
    {
        upl = calc_sell_pl(pos_value, filled_price, price);
    }

    const float new_upl = round(100 * (-(pos_value - upl))) / 100;

    if (new_upl)
    {
        if (new_upl <= -pos_value)
        {
            order.m_payload->set_status(NewOrderPayload::Status::CLOSED);
            order.m_payload->m_closed_price = price;
            order.m_payload->m_standing_quantity = order.m_payload->m_unrealised_pnl = 0;
            order.m_payload->m_realised_pnl += new_upl;
        }
    }
}