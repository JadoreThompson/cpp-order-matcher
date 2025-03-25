#ifndef _QUEUE_
#define _QUEUE_

#include <deque>
#include <future>
#include <list>

template <class T>
class Queue
{
private:
    std::deque<T *> queue;
    std::promise<bool> prom;
    std::list<std::promise<bool>*> getters;
public:
    void push(T *value);
    T &get();
    T &get_nowait();
};
#endif