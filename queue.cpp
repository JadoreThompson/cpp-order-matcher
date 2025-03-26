#include <iostream>
#include "queue.h"
#include <chrono>
#include <thread>


template <class T>
void Queue<T>::push(T *value)
{
    cvk::locker lock(sl);
    this->queue.push_back(value);
}

template <class T>
T Queue<T>::get_nowait()
{
    T value = *this->queue.front();
    this->queue.pop_front();
    sl.unlock();
    return value;
}

template <class T>
T Queue<T>::get()
{
    sl.lock();
    while(queue.empty()){
        sl.unlock();
        std::this_thread::sleep_for(std::chrono::microseconds(50));
        sl.lock();
    }

    return get_nowait();
}
