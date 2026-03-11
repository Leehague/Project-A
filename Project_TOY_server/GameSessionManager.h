#pragma once
#include "Types.h"
#include <map>
#include <set>
#include <mutex>
#include <memory>
#include "SendBuffer.h"


class Session;


class GameSessionManager
{
public:
    void Add(SessionPtr session);
    void Remove(SessionPtr session);
    void Broadcast(SendBufferPtr sendBuffer);
    void SendTo(uint64 playerId, SendBufferPtr sendBuffer);
private:
    std::mutex _lock;
    std::map<uint64, SessionPtr> _sessions;
};

extern GameSessionManager GSessionManager;