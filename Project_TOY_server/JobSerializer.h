#pragma once
#include <queue>
#include <memory>
#include "JobQueue.h"
#include <mutex>

class JobSerializer {
public:
    void Push(std::shared_ptr<JobQueue> jobQueue) {
        std::lock_guard<std::mutex> lock(_mutex);
        _queue.push(jobQueue);
    }

    std::shared_ptr<JobQueue> Pop() {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_queue.empty()) return nullptr;
        auto jobQueue = _queue.front();
        _queue.pop();
        return jobQueue;
    }

private:
    std::mutex _mutex;
    std::queue<std::shared_ptr<JobQueue>> _queue;
};

extern JobSerializer GJobSerializer;