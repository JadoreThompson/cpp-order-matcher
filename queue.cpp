#include "queue.h"


template <class T>
void Queue<T>::push(T *value)
{
    this->queue.push_back(value);

    for (auto &getter : this->getters)
    {
        (*getter).set_value(true);
        this->getters.clear();
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
    if (queue.empty())
    {
        std::promise<bool> prom;
        this->getters.push_back(&prom);
        prom.get_future().get();
    }

    return get_nowait();
}