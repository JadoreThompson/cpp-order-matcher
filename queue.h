#ifndef _QUEUE_
#define _QUEUE_

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include "order.h"

class Queue
{
private:
    std::deque<QueuePayload> m_queue;
    std::condition_variable m_cv;
    std::mutex m_mutex;

public:
    void push(QueuePayload &&payload) noexcept;

    QueuePayload get() noexcept;
};
#endif