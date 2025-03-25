#include "queue.h"


template <class T>
void Queue<T>::push(T *value)
{
    this->queue.push_back(value);
    
    if (this->getter) {
        (*this->getter).set_value(true);
        this->getter = nullptr;
    }
}

template <class T>
T &Queue<T>::get_nowait()
{
    T *value = this->queue.front();
    this->queue.pop_front();
    return *value;
}

template <class T>
T &Queue<T>::get()
{
    if (this->queue.empty())
    {
        std::promise<bool> prom;
        this->getter = &prom;
        prom.get_future().get();
    }

    return get_nowait();
}
