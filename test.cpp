// #include <condition_variable>
// #include <deque>
// #include <iostream>
// #include <mutex>
// #include <thread>
// #include <chrono>
// #include <vector>
// #include <numeric>
// #include <memory>

// int LOOPS = 1e6;

// enum Side
// {
//     BID,
//     ASK
// };

// enum ExecutionType
// {
//     GTC,
//     FOK
// };

// enum OrderType
// {
//     MARKET,
//     LIMIT
// };

// enum Status
// {
//     PENDING,
//     PARTIALLY_FILLED,
//     FILLED,
//     PARTIALLY_CLOSED,
//     CLOSED
// };

// class OrderPayload
// {

// private:
//     Status m_status;
//     float m_filled_price;
//     bool m_filled_price_set;

// public:
//     const ExecutionType m_exec_type;
//     const OrderType m_order_type;
//     const Side m_side;
//     const int m_quantity;
//     int m_standing_quantity;
//     float m_entry_price;
//     float m_take_profit_price;
//     float m_closed_price;
//     float m_realised_pnl;
//     float m_unrealised_pnl;
//     OrderPayload(
//         const int id,
//         const std::string instrument = "APPL",
//         const OrderType order_type = MARKET,
//         const Side side = BID,
//         const int quantity = 0,
//         float entry_price = -1.0f,
//         const ExecutionType exec_type = GTC,
//         float take_profit_price = 0.0f) : m_exec_type(exec_type), m_order_type(order_type), m_side(side), m_quantity(quantity), m_entry_price(entry_price), m_take_profit_price(take_profit_price) {} // Constructor
// };

// struct QueuePayload
// {
//     enum Category
//     {
//         NEW,
//         CANCEL,
//         MODIFY,
//         CLOSE
//     };

//     const Category m_category;
//     // int64_t m_payload;
//     // QueuePayload(const Category category, int payload) : m_category(category), m_payload(payload) {};
//     OrderPayload m_payload;
//     QueuePayload(int i) : m_category(NEW), m_payload(OrderPayload(i)) {};
// };

// // class A
// // {
// // public:
// //     int x;
// //     A() : x(123) {};
// //     // std::unique_ptr<int> x;
// //     // A() : x(std::make_unique<int>(123)) {} // Use unique_ptr for better performance
// // };

// template <typename A>
// class Queue
// {
// private:
//     std::deque<A> m_queue;
//     std::condition_variable cv;
//     std::mutex m_mutex;

// public:
//     Queue() = default;

//     void push(A &&value) // Move instead of copying
//     {
//         {
//             std::lock_guard<std::mutex> lock(m_mutex);
//             m_queue.push_back(std::move(value));
//             cv.notify_one();
//         }
//     }

//     A get()
//     {
//         std::unique_lock<std::mutex> lock(m_mutex);
//         cv.wait(lock, [this]
//                 { return !m_queue.empty(); });

//         A value = std::move(m_queue.front());
//         m_queue.pop_front();
//         return value;
//     }
// };

// template <typename A>
// void consumer_func(Queue<A> &queue)
// {
//     std::vector<double> times;
//     for (int i = 0; i < LOOPS; i++)
//     {
//         auto t0 = std::chrono::high_resolution_clock::now();
//         auto value = queue.get();
//         double time_taken = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - t0).count();
//         times.push_back(time_taken);
//     }

//     double average_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
//     std::cout << "Average time per get operation (ns): "
//               << average_time * 1e9 << std::endl;
// }

// template <typename A>
// void push_func(Queue<A> &queue)
// {
//     for (int i = 0; i < LOOPS; i++)
//     {
//         A a(i);
//         queue.push(std::move(a));
//     }
// }

// int main()
// {
//     Queue<QueuePayload> queue;
//     std::thread consumer_thread(consumer_func<QueuePayload>, std::ref(queue));
//     push_func(queue);
//     consumer_thread.join();

//     return 0;
// }
