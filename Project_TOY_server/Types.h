#pragma once
#include <cstdint>
#include <memory>
#include <mutex>
#include <map>
#include "Protocol/Protocol.pb.h"

using BYTE = unsigned char;

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

class Session; class SendBuffer; class Player; class Room;
class GameObject;

// 스마트 포인터 별칭 정의
using SessionPtr = std::shared_ptr<Session>;
using SendBufferPtr = std::shared_ptr<SendBuffer>;
using PlayerPtr = std::shared_ptr<Player>;
using RoomPtr = std::shared_ptr<Room>;
using GameObjectPtr = std::shared_ptr<GameObject>;

