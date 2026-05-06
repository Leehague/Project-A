#pragma once
#include <functional>
#include <queue>
#include <mutex>

class JobSerializer; // 전방 선언
//extern JobSerializer GJobSerializer; // 외부 어딘가(main.cpp 등)에 선언된 객체임을 알림

using JobFn = std::function<void()>;

class JobQueue : public std::enable_shared_from_this<JobQueue>
{
public:
    JobQueue(JobSerializer* jobserializer): _jobserializer(jobserializer){}
    virtual ~JobQueue() {}

    // 일감을 큐에 추가
    void Push(JobFn&& fn);
    

    // 쌓인 일감들을 실행 (한 쓰레드만 진입하도록 설계)
    virtual void Execute();
    

private:
    std::mutex _mutex;
    std::queue<JobFn> _jobs;
    bool _isInsideJobQueue = false;
protected:
    JobSerializer* _jobserializer;
};