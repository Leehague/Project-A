#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include <memory>
#include <map>
#include <mutex>



class Room : public std::enable_shared_from_this<Room>
{
public:
    void Enter(PlayerPtr player);
    void Leave(PlayerPtr player);
    void Broadcast(SendBufferPtr sendBuffer);
    void SendTo(PlayerPtr player, SendBufferPtr sendBuffer);

    // 이동 패킷 처리 루틴
    void HandleMove(PlayerPtr player ,Protocol::CS_MOVING& pkt);

private:
    std::mutex _lock;
    std::map<uint64, PlayerPtr> _players;
};