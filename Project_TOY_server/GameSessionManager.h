#pragma once
#include <map>
#include <set>
#include <mutex>
#include <memory>
#include "SendBuffer.h"
#include "Types.h"

class Session;


class GameSessionManager
{
public:
    void Add(SessionPtr session);
    void Remove(SessionPtr session);
    void Broadcast(SendBufferRef sendBuffer);
    void SendTo(uint64 playerId, SendBufferRef sendBuffer);
private:
    std::mutex _lock;
    std::map<uint64, SessionPtr> _sessions;
};

extern GameSessionManager GSessionManager;