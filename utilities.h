#ifndef _UTILITIES_
#define _UTILITIES_

#include <math.h>
#include "order.h"
#include "orderbook.h"

float calc_sell_pl(const float amount, const float open_price, const float close_price) noexcept;

float calc_buy_pl(const float amount, const float open_price, const float close_price) noexcept;

/*
 * Helper function to close position based on matching price
 */
void calc_upl(OrderPayload &payload, const float standing_quantity, const float price) noexcept;

#endif