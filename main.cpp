#include <thread>
#include <memory>
#include "crow.h"
#include "api/utils.h"
#include "engine/futures_engine.h"
#include "engine/orderbook.h"
#include "engine/order.h"
#include "engine/queue.h"

void init_engine(Queue &queue, bool push = true);

void handle_engine(FuturesEngine &engine, Queue &queue);

void push_engine(Queue &queue);

void api(Queue &queue);

int main()
{
    Queue queue;
    std::thread engine_initialiser_thread(init_engine, std::ref(queue), true);
    engine_initialiser_thread.join();
    return 0;
}

void init_engine(Queue &queue, bool push)
{
    FuturesEngine engine;

    std::thread engine_thread(handle_engine, std::ref(engine), std::ref(queue));

    if (push)
    {
        std::thread pusher(push_engine, std::ref(queue));
        pusher.join();
    }

    engine_thread.join();
}

void handle_engine(FuturesEngine &engine, Queue &queue)
{
    engine.start(queue);
}

void push_engine(Queue &queue)
{
    const Side sides[] = {Side::BID, Side::ASK};
    const OrderType ots[] = {OrderType::MARKET, OrderType::LIMIT};

    for (int i = 0; i < LOOPS; i++)
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
    }
}

void api(Queue &queue)
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/order/").methods(crow::HTTPMethod::POST)([&queue](const crow::request &req)
                                                               { 
            try {
                OrderPayload order_payload = validate_order(crow::json::load(req.body));
                queue.push(QueuePayload(
                        QueuePayload::Category::NEW, 
                        std::make_unique<OrderPayload>(order_payload)));
            } catch(const std::invalid_argument& e) {
                return crow::response(400, e.what());
            }
            return crow::response(200); });
    app.port(8000).run();
}