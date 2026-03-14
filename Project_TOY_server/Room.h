#pragma once
#include "Types.h"



class Room : public std::enable_shared_from_this<Room>
{
public:
    void Enter(GameObjectPtr go);
    void Leave(PlayerPtr player);
    void Broadcast(SendBufferPtr sendBuffer);
    void SendTo(PlayerPtr player, SendBufferPtr sendBuffer);

    // 이동 패킷 처리 루틴
    void HandleMove(PlayerPtr player ,Protocol::CS_MOVING& pkt);

    void SetRoomid(int32 roomid) { _roomid = roomid; }
    int32 GetRoomid() {return _roomid;}
private:
    std::mutex _lock;
    std::map<uint64, GameObjectPtr> _objects;

    int32 _roomid;
};