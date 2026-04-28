#pragma once
#include <functional>
#include <queue>
#include <mutex>

using JobFn = std::function<void()>;

class JobQueue : public std::enable_shared_from_this<JobQueue>
{
public:
    // 일감을 큐에 추가
    void Push(JobFn&& fn)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _jobs.push(std::move(fn));
        }
    }

    // 쌓인 일감들을 실행 (한 쓰레드만 진입하도록 설계)
    void Execute()
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
    }

private:
    std::mutex _mutex;
    std::queue<JobFn> _jobs;
};