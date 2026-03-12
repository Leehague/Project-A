#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "Room.h"


class Player :public std::enable_shared_from_this<Player>
{
public:
    uint64 playerId;
    Protocol::PosInfo posInfo;
    std::weak_ptr<Session> session; // 순환 참조 방지
    std::weak_ptr<Room> room;       // 현재 내가 속한 방
};