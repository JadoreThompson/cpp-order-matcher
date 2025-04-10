#include <iostream>
#include <thread>
#include "queue.h"

void Queue::push(QueuePayload &&payload)
{
    {
        std::lock_guard<std::mutex> lock(this->m_mutex);
        this->m_queue.emplace_back(std::move(payload));
        this->m_cv.notify_one();
    }
}

void Queue::get()
{
    while (this->m_queue.empty())
    {
        std::unique_lock<std::mutex> lock(this->m_mutex);
        this->m_cv.wait(lock, [this]
                        { return !this->m_queue.empty(); });
    }

    QueuePayload payload(std::move(this->m_queue.front()));
    this->m_queue.pop_front();
}
