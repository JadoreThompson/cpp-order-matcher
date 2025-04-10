#include <iostream>
#include <thread>
#include <math.h>
#include <memory>
#include "futures_engine.h"
#include "orderbook.h"
#include "order.h"
#include "queue.h"


void handle_engine(FuturesEngine &engine, Queue &queue);
void push_engine(Queue &queue);

int main()
{
    Queue queue;
    FuturesEngine engine;

    std::thread engine_thread(handle_engine, std::ref(engine), std::ref(queue));
    std::thread pusher(push_engine, std::ref(queue));

    pusher.join();
    engine_thread.join();
    return 0;
}

void handle_engine(FuturesEngine &engine, Queue &queue)
{
    engine.start(queue);
}

void push_engine(Queue &queue)
{
    int id_counter = 0;
    const Side sides[] = {Side::BID, Side::ASK};
    const OrderType ots[] = {OrderType::MARKET, OrderType::LIMIT};

    for (int i = 0; i < LOOPS * 5; i++)
    {
        // QueuePayload qp(QueuePayload::Category::NEW, OrderPayload(
        //                                                  i,
        //                                                  "A",
        //                                                  OrderType::MARKET,
        //                                                  sides[std::rand() % 2],
        //                                                  std::rand() % 10 + 1,
        //                                                  std::rand() % 50 + 1,
        //                                                  // NewOrderPayload::ExecutionType::FOK);
        //                                                  ExecutionType::GTC));

        QueuePayload qp{QueuePayload::Category::NEW, std::move(std::make_unique<OrderPayload>(
                                                         i,
                                                         "A",
                                                         OrderType::MARKET,
                                                         sides[std::rand() % 2],
                                                         std::rand() % 10 + 1,
                                                         std::rand() % 50 + 1,
                                                         // NewOrderPayload::ExecutionType::FOK);
                                                         ExecutionType::GTC))};
        queue.push(std::move(qp));
    }
}