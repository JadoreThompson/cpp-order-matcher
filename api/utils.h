#ifndef _JSON_VALIDATORS_
#define _JSON_VALIDATORS_

#include <array>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "crow.h"
#include "engine/order.h"

template <typename T>
class Set
{
public:
    Set(std::initializer_list<T> members) : m_set(members)
    {
        int index = 0;

        for (const T val : members)
        {
            this->m_map.emplace(val, index++);
        }
    };

    bool contains(const T val) const
    {
        return this->m_set.find(val) != this->m_set.end();
    }

    int index(const T val) const
    {
        auto ind = this->m_map.find(val);
        return (ind == this->m_map.end()) ? -1 : ind->second;
    }

private:
    std::unordered_map<T, int> m_map;
    const std::unordered_set<T> m_set;
};

struct TypeSchema
{
    const std::type_index m_underlying_type;
    const crow::json::type m_crow_type;
    TypeSchema(const std::type_index underlying_type, const crow::json::type crow_type)
        : m_underlying_type(underlying_type), m_crow_type(crow_type) {};
};

template <typename T>
TypeSchema build_schema(const crow::json::type crow_type)
{
    return TypeSchema(std::type_index(typeid(T)), crow_type);
}

const std::map<std::string, TypeSchema> CREATE_ORDER_FIELDS{
    {"instrument", build_schema<std::string>(crow::json::type::String)},
    {"exec_type", build_schema<std::string>(crow::json::type::String)},
    {"order_type", build_schema<std::string>(crow::json::type::String)},
    {"side", build_schema<std::string>(crow::json::type::String)},
    {"quantity", build_schema<int>(crow::json::type::Number)},
    {"entry_price", build_schema<float>(crow::json::type::Number)},
    {"stop_loss_details", build_schema<crow::json::rvalue>(crow::json::type::Object)},
    {"take_profit_price", build_schema<float>(crow::json::type::Number)}};

const std::map<std::string, Set<std::string>> ENUM_STRINGS = {
    {"exec_type", Set<std::string>({"fok", "gtc", "ioc"})},
    {"order_type", Set<std::string>({"market", "limit", "stop"})},
    {"side", Set<std::string>({"bid", "ask"})}};

void validate_num_value(const crow::json::rvalue &val, const std::type_index &type, const std::string &field_name)
{
    if (type == typeid(int))
    {
        try
        {
            (void)val.i();
        }
        catch (...)
        {
            throw std::invalid_argument(field_name + "is not of type int.");
        }
    }
    else if (type == typeid(double))
    {
        try
        {
            (void)val.d();
        }
        catch (...)
        {
            throw std::invalid_argument(field_name + "is not of type double.");
        }
    }
}

OrderPayload validate_order(const crow::json::rvalue &json)
{
    for (const auto &pair : CREATE_ORDER_FIELDS)
    {
        if (!json.has(pair.first) && pair.first != "stop_loss_details" && pair.first != "take_profit_price")
            throw std::invalid_argument(pair.first + " missing in body.");

        const auto val = json[pair.first];

        if (val.t() != pair.second.m_crow_type)
            throw std::invalid_argument(pair.first + " is of wrong type.");

        if (pair.second.m_crow_type == crow::json::type::Number)
            validate_num_value(val, pair.second.m_underlying_type, pair.first);

        if (pair.first == "exec_type" || pair.first == "order_type" || pair.first == "side")
        {
            const auto it = ENUM_STRINGS.find(pair.first);
            if (it->second.index(static_cast<std::string>(val)) == -1)
                throw std::invalid_argument(pair.first + " does not include " + static_cast<std::string>(val) + ".");
        }
    }

    return OrderPayload(
        1,
        static_cast<std::string>(json["instrument"].s()),
        ExecutionType(ENUM_STRINGS.at("exec_type").index(json["exec_type"].s())),
        OrderType(ENUM_STRINGS.at("order_type").index(json["order_type"].s())),
        Side(ENUM_STRINGS.at("side").index(json["side"].s())),
        static_cast<int>(json["quantity"].i()),
        static_cast<float>(json["entry_price"].d()),
        (json.has("stop_loss_details"))
            ? StopLossOrder(
                  static_cast<float>(json["stop_loss_details"]["price"].d()),
                  (json["stop_loss_details"].has("distance"))
                      ? static_cast<float>(json["stop_loss_details"]["distance"].d())
                      : -1.0f)
            : StopLossOrder{},
        static_cast<float>((json.has("take_profit_price"))
                               ? static_cast<float>(json["take_profit_price"].d())
                               : -1.0f));
}

#endif