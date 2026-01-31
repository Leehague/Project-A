#pragma once
#include "Protocol/Protocol.pb.h"
#include "Types.h"

// 패킷 처리용 함수 시그니처 (세션과 패킷 데이터, 길이를 받음)
using PacketHandlerFunc = std::function<bool(PacketSessionRef&, BYTE*, int32)>;

class PacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			_handlerButtons[i] = Handle_INVALID;

		// TODO: 여기서 파이썬이 생성한 핸들러 등록 함수를 호출할 예정입니다.
		// CustomPacketHandler::RegisterHandlers(); 
	}

	// 핵심 진입점: 세션에서 패킷이 오면 이걸 호출
	static bool HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return _handlerButtons[header->id](session, buffer, len);
	}

	// 공통: 잘못된 패킷 처리
	static bool Handle_INVALID(PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		// 로그를 남기거나 세션을 종료
		return false;
	}

protected:
	// 공통 템플릿: Protobuf 패킷 파싱 및 실제 로직 호출을 자동화
	template<typename T, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, PacketSessionRef& session, BYTE* buffer, int32 len)
	{
		T pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

protected:
	// 함수 테이블: 패킷 ID를 인덱스로 바로 실행
	static PacketHandlerFunc _handlerButtons[UINT16_MAX];
};