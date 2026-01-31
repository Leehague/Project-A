#pragma once
#include <cstdint>
#include <memory>

class PacketSession;

using BYTE = unsigned char;

using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;

class Session; class SendBuffer;

// 스마트 포인터 별칭 정의
using SessionPtr = std::shared_ptr<Session>;
using SendBufferRef = std::shared_ptr<SendBuffer>;
using PacketSessionRef = std::shared_ptr<PacketSession>;

#pragma pack(push, 1) // 메모리 정렬을 1바이트 단위로 (빈틈 없게)
struct PacketHeader
{
    uint16 size; // 패킷의 전체 크기 (헤더 포함)
    uint16 id;   // 패킷 종류 (예: 1번은 로그인, 2번은 이동...)
};
#pragma pack(pop)