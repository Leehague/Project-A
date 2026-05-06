#pragma once
#include <queue>
#include <memory>
#include <mutex>
#include "JobQueue.h"

class JobSerializer {
public:
    void Push(std::shared_ptr<JobQueue> jobQueue);
    std::shared_ptr<JobQueue> Pop();
private:
    std::mutex _mutex;
    std::queue<std::shared_ptr<JobQueue>> _queue;
};

extern JobSerializer GJobSerializer;
