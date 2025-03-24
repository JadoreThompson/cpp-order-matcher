#include <iostream>
#include <thread>
#include "orderbook.h"
#include "order.h"

int main()
{
    OrderBook ob("APPL");
    int id_counter = 0;
    const Side sides[] = {BID, ASK};

    while (true)
    {
        OrderPayload p(id_counter, sides[std::rand() % 2], std::rand() % 10, std::rand() % 50);
        Order order(p, ENTRY);
        ob.push_order(order);
        id_counter += 1;
        const auto book_size = ob.size();
        std::cout << "Ask " << std::to_string(book_size.first) << " Bids " << std::to_string(book_size.second) << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}