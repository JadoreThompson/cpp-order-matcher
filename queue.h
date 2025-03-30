#ifndef _QUEUE_
#define _QUEUE_

#include <deque>
#include <memory>
#include <atomic>
#include "order.h"

class Queue
{
private:
    std::deque<std::shared_ptr<QueuePayload>> queue;
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    bool locked = false;
    void lock();
    void unlock();

public:
    // template <typename X, typename... Args>
    // void push(Args &&...args);
    // template <typename... Args>
    void push(std::shared_ptr<QueuePayload> value);
    // void push(T value);
    // std::shared_ptr<T> get();
    std::shared_ptr<QueuePayload> get();
    // std::shared_ptr<T> get_nowait();
    std::shared_ptr<QueuePayload> get_nowait();
};
#endif
