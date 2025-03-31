#include <iostream>
#include <thread>
#include <math.h>
#include <memory>
#include "futures_engine.h"
#include "orderbook.h"
#include "order.h"
#include "queue.h"
// #include "queue.cpp"

void handle_engine(FuturesEngine &engine, Queue &queue);

int main()
{
    FuturesEngine engine;
    // Queue<OrderPayload> queue;
    Queue queue;
    int id_counter = 0;
    const NewOrderPayload::Side sides[] = {NewOrderPayload::Side::BID, NewOrderPayload::Side::ASK};
    const NewOrderPayload::OrderType ots[] = {NewOrderPayload::OrderType::MARKET, NewOrderPayload::OrderType::LIMIT};

    std::thread engine_thread(handle_engine, std::ref(engine), std::ref(queue));

    while (true)
    {
        id_counter++;

        std::shared_ptr<NewOrderPayload> p = std::make_shared<NewOrderPayload>(
            id_counter,
            NewOrderPayload::OrderType::MARKET,
            sides[std::rand() % 2],
            "APPL",
            std::rand() % 10,
            std::rand() % 50 + 1);

        queue.push(std::make_shared<QueuePayload>(QueuePayload::Category::NEW, p));
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "Next Payload ID: " << id_counter + 1 << std::endl;
    }

    engine_thread.join();
    return 0;
}

void handle_engine(FuturesEngine &engine, Queue &queue)
{
    engine.start(queue);
}