#include "JobQueue.h"
#include "JobSerializer.h"

void JobQueue::Push(JobFn&& fn)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _jobs.push(std::move(fn));

        // 이미 실행 대기 중이거나 실행 중이면 중복 등록 안 함
        if (_isInsideJobQueue == false) {
            _isInsideJobQueue = true;
            _jobserializer->Push(shared_from_this());
        }
    }
}

void JobQueue::Execute()
{
    while (true)
    {
        JobFn job;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (_jobs.empty())
                break;

            job = std::move(_jobs.front());
            _jobs.pop();
        }

        job(); // 실제 로직 실행
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _isInsideJobQueue = false; // 작업 완료 후 플래그 해제

        // 만약 그사이 새로운 작업이 들어왔다면 다시 Serializer에 등록
        if (!_jobs.empty()) {
            _isInsideJobQueue = true;
            _jobserializer->Push(shared_from_this());
        }
    }
}