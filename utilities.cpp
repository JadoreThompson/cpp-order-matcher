#include "utilities.h"

float calc_sell_pl(const float amount, const float open_price, const float close_price) noexcept
{
    return round(100 * (amount * 1 + (open_price - close_price) / open_price)) / 100;
}

float calc_buy_pl(const float amount, const float open_price, const float close_price) noexcept
{
    return round(100 * (close_price / open_price * amount)) / 100;
}

/// @brief
/// @param order
/// @param standing_quantity: Standing quantity of order pre match subtractions
/// @param price
void calc_upl(OrderPayload &payload, const float standing_quantity, const float price) noexcept
{
    if (payload.m_filled_price == 1.0f)
        return;

    const float filled_price = payload.m_filled_price;
    const float pos_value = filled_price * standing_quantity;
    float upl;

    if (payload.m_side == Side::ASK)
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
            // payload.set_status(Status::CLOSED);
            payload.m_status = Status::CLOSED;
            payload.m_closed_price = price;
            payload.m_standing_quantity = payload.m_unrealised_pnl = 0;
            payload.m_realised_pnl += new_upl;
        }
    }
}