#include <iostream>
#include "queue.h"

void Queue::lock()
{
    while (this->flag.test_and_set(std::memory_order_acquire))
    {
    };
    this->locked = true;
}


void Queue::unlock()
{
    this->flag.clear(std::memory_order_release);
    this->locked = false;
}

void Queue::push(std::shared_ptr<QueuePayload> value)
{
    lock();
    this->queue.push_back(value);
    unlock();
}

std::shared_ptr<QueuePayload> Queue::get_nowait()
{
    lock();
    std::shared_ptr<QueuePayload> value = this->queue.front();
    this->queue.pop_front();
    unlock();
    return value;
}

std::shared_ptr<QueuePayload> Queue::get()
{
    lock();
    while (this->queue.empty())
    {
        unlock();
        lock();
    }

    unlock();
    return get_nowait();
}
