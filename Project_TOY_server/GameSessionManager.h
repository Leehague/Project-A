#pragma once
#include <set>
#include <mutex>

class Session;

class GameSessionManager
{
public:
    void Add(Session* session);
    void Remove(Session* session);
    void Broadcast(SendBufferRef sendBuffer);

private:
    std::mutex _lock;
    std::set<Session*> _sessions;
};

extern GameSessionManager GSessionManager;