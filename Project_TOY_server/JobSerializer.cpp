#include "JobSerializer.h"

JobSerializer GJobSerializer;

void JobSerializer::Push(std::shared_ptr<JobQueue> jobQueue) {
    std::lock_guard<std::mutex> lock(_mutex);
    _queue.push(jobQueue);
}

std::shared_ptr<JobQueue> JobSerializer::Pop() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_queue.empty()) return nullptr;
    auto jobQueue = _queue.front();
    _queue.pop();
    return jobQueue;
}