#pragma once

#include "Protocol/Protocol.pb.h"
#include "SendBuffer.h"
#include "Types.h"
#pragma comment(lib, "libprotobufd.lib") // 또는 libprotobufd.lib

#pragma pack(push, 1) // 1바이트 단위로 정렬
// 패킷의 가장 앞에 붙는 메타데이터
struct PacketHeader {
    uint16 size; // 패킷의 전체 크기 (헤더 + 데이터)
    uint16 id;   // 패킷 종류 (Protocol.proto의 PacketId)
};
#pragma pack(pop)

class ServerUtils
{
public:
    template<typename T>
    static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
    {
        const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
        const uint16 headerSize = sizeof(PacketHeader);
        const uint16 totalSize = dataSize + headerSize;

        SendBufferRef sendBuffer = std::make_shared<SendBuffer>(totalSize);

        BYTE* bufferStart = reinterpret_cast<BYTE*>(sendBuffer->Buffer());

        //헤더 채우기
        PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
        header->size = totalSize;
        header->id = pktId;

        //데이터가 들어갈 위치 계산
        BYTE* dataStart = bufferStart + headerSize;

        if (pkt.SerializeToArray(dataStart, dataSize) == false)
        {
            return nullptr;
        }

        return sendBuffer;
    }
};