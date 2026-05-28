#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "SendBuffer.h"
#include "BufferWriter.h"

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
    static SendBufferPtr MakeSendBuffer(T& pkt, uint16 pktId)
    {
        const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
        const uint16 headerSize = sizeof(PacketHeader);
        const uint16 totalSize = dataSize + headerSize;

        // [디버깅 로그 추가]
        //std::cout << "[Send] ID: " << pktId << ", DataSize: " << dataSize << ", TotalSize: " << totalSize << std::endl;


        SendBufferPtr sendBuffer = std::make_shared<SendBuffer>(totalSize);

        // 1. 헤더 직접 기입
        PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
        header->size = totalSize;
        header->id = pktId;

        // 2. 데이터 직렬화
        BYTE* dataStart = reinterpret_cast<BYTE*>(sendBuffer->Buffer()) + headerSize;
        if (pkt.SerializeToArray(dataStart, dataSize) == false)
            return nullptr;

        // [핵심 추가] 3. 버퍼의 사용된 크기를 강제로 설정해줌
        // SendBuffer 클래스에 public으로 추가하거나, Write 함수를 활용해야 합니다.
        sendBuffer->Close(totalSize);

        return sendBuffer;


        
    }
};