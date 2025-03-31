#ifndef _ORDERBOOK_
#define _ORDERBOOK_

#include <list>
#include <map>
#include <memory>
#include <string>
#include "order.h"

class OrderBook
{
private:
    std::map<int, Position> tracker;

public:
    std::map<float, std::list<Order *>> bids;
    std::map<float, std::list<Order *>> asks;
    const std::string instrument;
    
    OrderBook(const std::string instrument);
    
    std::map<float, std::list<Order *>> &get_book(const Order &order) const;
    
    Position &declare(std::shared_ptr<NewOrderPayload> payload);
    
    Position &track(Order &order);
    
    void rtrack(Order &order);
    
    Position &get_position(const int id);
    
    void remove_from_level(Order &order);
    
    void push_order(Order &order);
    
    std::pair<int, int> size();
};
#endif