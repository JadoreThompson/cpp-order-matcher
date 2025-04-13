#include <thread>
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
    const Side sides[] = {Side::BID, Side::ASK};
    const OrderType ots[] = {OrderType::MARKET, OrderType::LIMIT};
    // int i = 0;
    for (int i = 0; i < LOOPS; i++)
    // while (true)
    {
        try
        {

            QueuePayload qp{QueuePayload::Category::NEW,
                            std::make_unique<OrderPayload>(
                                i,
                                "APPL",
                                ExecutionType::FOK,
                                OrderType::MARKET,
                                sides[std::rand() % 2],
                                std::rand() % 30 + 1,
                                std::rand() % 50 + 1,
                                // StopLossOrder(std::rand() % 100 + 1))};
                                StopLossOrder())};
            queue.push(std::move(qp));
        }
        catch (const std::string &e)
        {
        }
        // i++;
    }
}