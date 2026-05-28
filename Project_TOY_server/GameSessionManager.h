#pragma once
#include "Types.h"
#include <map>
#include <set>
#include <mutex>
#include <memory>
#include "SendBuffer.h"


class Session;
class Room;


class GameSessionManager
{
public:
    void Add(SessionPtr session);
    void Remove(SessionPtr session);
    void GlobalBroadcast(SendBufferPtr sendBuffer); //특수한 상황에서 서버 전체에 패킷을 보내야할때만 사용.
    void SendTo(uint64 playerSessionId, SendBufferPtr sendBuffer);


private:
    std::mutex _lock;
    std::map<uint64, SessionPtr> _sessions;
    std::map<uint64, RoomPtr> _rooms;
};

extern GameSessionManager GSessionManager;