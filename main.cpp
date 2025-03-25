#include <iostream>
#include <thread>
#include <deque>
#include <functional>
#include <future>
#include "futures_engine.h"
#include "orderbook.h"
#include "order.h"
#include "queue.h"
#include "queue.cpp"

void handle_engine(FuturesEngine &engine, Queue<OrderPayload> &queue);

int main()
{
    FuturesEngine engine;
    Queue<OrderPayload> queue;
    OrderBook ob("APPL");
    int id_counter = 0;
    const Side sides[] = {BID, ASK};
    const OrderType ots[] = {MARKET, LIMIT};

    std::thread engine_thread(handle_engine, std::ref(engine), std::ref(queue));

    while (true)
    {
        OrderPayload p(
            id_counter,
            // ots[std::rand() % 2],
            MARKET,
            sides[std::rand() % 2],
            "APPL",
            std::rand() % 10,
            std::rand() % 50);
        queue.push(&p);
        id_counter += 1;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "Next Id to be passed => " << std::to_string(id_counter) << std::endl;
    }

    engine_thread.join();
    return 0;
}

void handle_engine(FuturesEngine &engine, Queue<OrderPayload> &queue)
{
    engine.start(queue);
}
