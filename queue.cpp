#include <iostream>
#include "queue.h"

// template <class T>
// void Queue<T>::lock()
void Queue::lock()
{
    while (this->flag.test_and_set(std::memory_order_acquire))
    {
    };
    this->locked = true;
}

// template <class T>
// void Queue<T>::unlock()
void Queue::unlock()
{
    this->flag.clear(std::memory_order_release);
    this->locked = false;
}

// template <class T>
// template <typename X, typename... Args>
// template <typename... Args>
// void Queue<T>::push(Args &&...args)
// void Queue<T>::push(T value)
void Queue::push(std::shared_ptr<QueuePayload> value)
{
    lock();
    // this->queue.emplace_back(X(std::forward<Args>(args)...));
    // this->queue.push_back(std::make_shared<QueuePayload>(args));
    this->queue.push_back(value);
    unlock();
}

// template <class T>
// std::shared_ptr<T> Queue<T>::get_nowait()
std::shared_ptr<QueuePayload> Queue::get_nowait()
{
    lock();
    std::shared_ptr<QueuePayload> value = this->queue.front();
    this->queue.pop_front();
    unlock();

    return value;
}

// template <class T>
// std::shared_ptr<T> Queue<T>::get()
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
