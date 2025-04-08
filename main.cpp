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

    for (int i = 0; i < 1000000; i++)
    {
        id_counter++;

        std::shared_ptr<NewOrderPayload> p = std::make_shared<NewOrderPayload>(
            id_counter,
            "APPL",
            NewOrderPayload::OrderType::MARKET,
            sides[std::rand() % 2],
            std::rand() % 10 + 1,
            std::rand() % 50 + 1,
            std::make_unique<StopLossOrder>(),
            // NewOrderPayload::ExecutionType::FOK);
            NewOrderPayload::ExecutionType::GTC);

        queue.push(QueuePayload(QueuePayload::Category::NEW, p));
    }

    engine_thread.join();
    return 0;
}

void handle_engine(FuturesEngine &engine, Queue &queue)
{
    engine.start(queue);
}