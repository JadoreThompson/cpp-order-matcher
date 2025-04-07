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

void Queue::push(QueuePayload &&value)
{
    lock();
    this->queue.push_back(std::make_unique<QueuePayload>(value));
    unlock();
}

std::unique_ptr<QueuePayload> Queue::get_nowait()
{
    lock();
    std::unique_ptr<QueuePayload> value = std::move(this->queue.front());
    this->queue.pop_front();
    unlock();
    return std::move(value);
}

std::unique_ptr<QueuePayload> Queue::get()
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
