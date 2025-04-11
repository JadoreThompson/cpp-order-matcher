#include <iostream>
#include <thread>
#include "queue.h"

void Queue::push(QueuePayload &&payload)
{
    {
        // std::cout << "Received payload param" << "\n";
        std::lock_guard<std::mutex> lock(this->m_mutex);
        // std::cout << "Pushing payload into queue" << "\n";
        this->m_queue.push_back(std::move(payload));
        // std::cout << "Payload pushed into queue" << "\n";
        this->m_cv.notify_one();
    }
}

void Queue::get() noexcept
{
    while (this->m_queue.empty())
    {
        std::unique_lock<std::mutex> lock(this->m_mutex);
        this->m_cv.wait(lock, [this]
                        { return !this->m_queue.empty(); });
        // std::cout << "Size: " << this->m_queue.size() << "\n";
        // std::cout << "Empty? " << this->m_queue.empty() << "\n";
    }

    auto &val = this->m_queue.front();
    // std::cout << "Instrument: " << val.m_payload->m_instrument;
    // std::cout << " Id: " << val.m_payload->m_id << std::endl;
    // QueuePayload payload(std::move(val));
    // std::cout << " Id: " << payload.m_payload->m_id << std::endl;
    this->m_queue.pop_front();
}
