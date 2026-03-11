#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"


class Player 
{
public:
    uint64 _playerId = 1;
    Protocol::PosInfo posInfo;          // 현재 위치/상태 정보
    SessionPtr _session=nullptr;        // 통신을 위한 세션 역참조
};