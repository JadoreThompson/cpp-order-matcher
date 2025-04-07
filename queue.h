#ifndef _QUEUE_
#define _QUEUE_

#include <deque>
#include <memory>
#include <atomic>
#include "order.h"

class Queue
{
private:
    std::deque<std::unique_ptr<QueuePayload>> queue;
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    bool locked = false;
    void lock();
    void unlock();

public:
    void push(QueuePayload &&value);

    std::unique_ptr<QueuePayload> get();

    std::unique_ptr<QueuePayload> get_nowait();
};
#endif
