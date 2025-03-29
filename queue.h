#ifndef _QUEUE_
#define _QUEUE_

#include <deque>
#include <future>
#include <list>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>

template <class T>
class Queue
{
private:    
    std::deque<std::shared_ptr<T>> queue;
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    bool locked = false;
    void lock();
    void unlock();

public:
    void push(std::shared_ptr<T> value);
    std::shared_ptr<T> get();
    std::shared_ptr<T> get_nowait();
};
#endif
