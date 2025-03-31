#include <iostream>
#include <thread>
#include <math.h>
#include <memory>
#include "futures_engine.h"
#include "orderbook.h"
#include "order.h"
#include "queue.h"

void handle_engine(FuturesEngine &engine, Queue &queue);

int main()
{
    FuturesEngine engine;
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

        if (id_counter % 5 == 0)
        {
            queue.push(
                std::make_shared<QueuePayload>(
                    QueuePayload::Category::MODIFY,
                    std::make_shared<ModifyOrderPayload>(
                        id_counter,
                        "APPL",
                        float(std::rand() % 50 + 1),
                        float(std::rand() % 50 + 1))));
        }
    }

    engine_thread.join();
    return 0;
}

void handle_engine(FuturesEngine &engine, Queue &queue)
{
    engine.start(queue);
}