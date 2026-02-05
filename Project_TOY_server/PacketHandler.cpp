#include "PacketHandler.h"
#include "Session.h"

// 헤더에 있는 extern 선언과 타입이 정확히 일치해야 합니다.
PacketHandlerFunc GPacketHandler[65535];

bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len)
{
    return false;
}

bool Handle_CS_LOGIN(SessionRef& session, Protocol::CS_LOGIN& pkt)
{
    std::cout << "로그인 요청 ID: " << pkt.userid() << std::endl;

    //TODO 로그인 유효성 검증 로직 추가

    // [수정] google protobuf 도입
    // 응답 전송 
    Protocol::SC_LOGIN_OK resPkt;
    resPkt.set_success(true);
    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_LOGIN_OK);
    session->Send(sendBuffer);

    return true;
}



bool Handle_CS_CHAT(SessionRef& session, Protocol::CS_CHAT& pkt)
{
    // 1. 받은 패킷에서 데이터 추출 (pkt->chatMsg 대신 pkt.msg() 등 사용)
    // .proto 파일에 정의한 필드명에 따라 함수명이 결정됩니다. (예: string msg -> msg())
    std::string receivedMsg = pkt.msg();

    // 2. 보낼 응답 패킷 생성 및 데이터 채우기
    Protocol::SC_CHAT_BROADCAST res;

    // 플레이어 ID 설정 (소켓 번호보다는 시스템에서 부여한 고유 ID가 좋습니다)
    res.set_playerid(static_cast<uint64>(session->Session::GetSocket()));

    // 채팅 내용 설정 (Protobuf가 내부적으로 메모리 할당을 관리하므로 strcpy_s가 필요 없습니다)
    res.set_msg(receivedMsg);

    // 3. ServerUtils를 사용하여 가변 크기용 SendBuffer 생성
    // 내부에서 ByteSizeLong() 호출, 헤더 기입, 직렬화가 모두 처리됩니다.
    SendBufferRef sendBuffer = ServerUtils::MakeSendBuffer(res, Protocol::PKT_SC_CHAT_BROADCAST);

    // 4. 브로드캐스팅
    if (sendBuffer)
    {
        GSessionManager.Broadcast(sendBuffer);
    }
    return true;
}


bool Handle_CS_WHISPER(SessionRef& session, Protocol::CS_WHISPER& pkt)
{
    uint64 targetId = pkt.targetplayerid(); // Protobuf getter
    std::string msg = pkt.msg();

    // 1. 타겟에게 보낼 패킷 생성 (SC_WHISPER 권장)
    Protocol::SC_WHISPER res;
    res.set_fromplayerid(session->GetPlayerId()); // 보낸 사람 ID
    res.set_msg(msg);

    // 2. 버퍼 생성
    SendBufferRef sendBuffer = ServerUtils::MakeSendBuffer(res, Protocol::PKT_SC_WHISPER);

    if (sendBuffer)
    {
        // 3. 타겟에게 전송
        GSessionManager.SendTo(targetId, sendBuffer);

        // 4. (옵션) 보낸 사람 본인에게도 "전송 성공" 피드백 패킷을 보내면 좋습니다.
        // 또는 그냥 클라가 보낸 메시지를 본인 화면에 바로 띄우도록 설계할 수도 있습니다.
    }
    return true;
}
