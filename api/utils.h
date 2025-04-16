#ifndef _JSON_VALIDATORS_
#define _JSON_VALIDATORS_

#include <string>
#include <map>
#include "crow.h"
#include "engine/order.h"

const std::map<std::string, crow::json::type> CREATE_ORDER_FIELDS{
    {"name", crow::json::type::String}};

void validate_order(crow::json::rvalue &json)
{
    for (const auto &pair : CREATE_ORDER_FIELDS)
    {
        if (!json.has(pair.first))
            throw std::invalid_argument(pair.first + " missing in body.");

        if (json[pair.first].t() != pair.second)
            throw std::invalid_argument(pair.first + " is of wrong type.");
    }
}

#endif