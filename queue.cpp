#include <iostream>
#include "queue.h"

template <class T>
void Queue<T>::lock()
{
    while (this->flag.test_and_set(std::memory_order_acquire))
    {
    };
    this->locked = true;
}

template <class T>
void Queue<T>::unlock()
{
    this->flag.clear(std::memory_order_release);
    this->locked = false;
}

template <class T>
void Queue<T>::push(std::shared_ptr<T> value)
{
    lock();
    this->queue.push_back(value);
    unlock();
}

template <class T>
T &Queue<T>::get_nowait()
{
    lock();
    T &value = *(this->queue.front());
    this->queue.pop_front();
    unlock();

    return value;
}

template <class T>
T &Queue<T>::get()
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
