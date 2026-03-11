#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include <memory>
#include <map>
#include <mutex>



class Room : public std::enable_shared_from_this<Room>
{
    void Enter(PlayerPtr player);
    void Leave(PlayerPtr player);
    void Broadcast(SendBufferPtr sendBuffer);

    // 이동 패킷 처리 루틴
    void HandleMove(Protocol::CS_MOVING& pkt);

private:
    std::mutex _lock;
    std::map<uint64, PlayerPtr> _players;
};