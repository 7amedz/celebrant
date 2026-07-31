#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

#include "celebrant/types.hpp"
namespace celebrant {

class InboundQueue {
  public:
    Request pop() {
        // unique_lock allows unlocking during lifetime
        std::unique_lock<std::mutex> lock(m_); // use unique_lock with condition_variable with
                                               // predicate checking if queue is not empty
        cv_.wait(lock, [this] { return !queue_.empty(); });
        Request request = queue_.front();
        queue_.pop();
        return request;
    }
    void push(Request request) {
        {
            std::lock_guard<std::mutex> lock(m_); // no need for unique_lock here
            queue_.push(request);
        } // unlock on destructor
        cv_.notify_one();
    }

  private:
    std::queue<Request> queue_;
    std::mutex m_;
    std::condition_variable cv_;
};

} // namespace celebrant
