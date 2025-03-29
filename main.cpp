#include <iostream>
#include <utility>
#include <thread>
#include <math.h>
#include <memory>
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
    int id_counter = 0;
    const Side sides[] = {BID, ASK};
    const OrderType ots[] = {MARKET, LIMIT};

    std::thread engine_thread(handle_engine, std::ref(engine), std::ref(queue));

    while (true)
    {
        id_counter++;
        
        std::shared_ptr<OrderPayload> p = std::make_shared<OrderPayload>(
            id_counter,
            MARKET,
            sides[std::rand() % 2],
            "APPL",
            std::rand() % 10,
            std::rand() % 50 + 1);

        queue.push(p);
        // std::this_thread::sleep_for(std::chrono::milliseconds(5));
        std::cout << "Next Payload ID: " << id_counter + 1 << std::endl;
    }

    engine_thread.join();
    return 0;
}

void handle_engine(FuturesEngine &engine, Queue<OrderPayload> &queue)
{
    engine.start(queue);
}