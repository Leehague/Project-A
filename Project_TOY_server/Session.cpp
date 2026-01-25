#include "Session.h"
#include <iostream>

Session::Session() : _recvBuffer(1024 * 10)
{
}

Session::Session(SOCKET socket) : _socket(socket), _recvBuffer(1024 * 10) // 10KB 버퍼
{
    
}

Session::~Session()
{
    if (_socket != INVALID_SOCKET)
        closesocket(_socket);
}

//수신 예약 (WSARecv)
void Session::Receive()
{
    // Overlapped 정보를 설정 (나중에 IOCP에서 이 정보를 보고 처리함)
    OverlappedEx* overlapped = new OverlappedEx(); // 실제로는 풀링해서 써야 함
    memset(overlapped, 0, sizeof(WSAOVERLAPPED));
    overlapped->type = IO_TYPE::RECV;
    overlapped->owner = this;

    // WSABUF 설정: RecvBuffer에서 쓸 수 있는 공간을 OS에 넘겨줌
    WSABUF wsaBuf;
    wsaBuf.buf = _recvBuffer.WritePos();
    wsaBuf.len = _recvBuffer.FreeSize();

    DWORD bytesReceived = 0;
    DWORD flags = 0;

    // 비동기 수신 호출
    int errorCode = ::WSARecv(_socket, &wsaBuf, 1, &bytesReceived, &flags, &overlapped->overlapped, nullptr);

    if (errorCode == SOCKET_ERROR)
    {
        int err = ::WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            // 진짜 에러 발생 시 처리
            std::cout << "WSARecv Error: " << err << std::endl;
        }
    }
}

//수신 완료 콜백 (IOCP Worker Thread에 의해 호출됨)
void Session::OnRecv(int bytesTransferred)
{
    if (bytesTransferred == 0) 
    {
        OnDisconnected(); 
        return; 
    }
    if (_recvBuffer.OnWrite(bytesTransferred) == false) { OnDisconnected(); return; }

    while (true)
    {
        int dataSize = _recvBuffer.DataSize();

        // 1. 헤더(4바이트)만큼은 왔는지 확인
        if (dataSize < sizeof(PacketHeader))
            break;

        // 2. 패킷 헤더를 읽어 전체 크기 확인
        PacketHeader* header = reinterpret_cast<PacketHeader*>(_recvBuffer.ReadPos());

        // 3. 전체 패킷이 다 왔는지 확인
        if (dataSize < header->size)
            break;

        // 4. 패킷 처리 함수로 전달
        HandlePacket(reinterpret_cast<BYTE*>(_recvBuffer.ReadPos()), header->size);

        // 5. 처리한 패킷 크기만큼 읽기 커서 이동
        _recvBuffer.OnRead(header->size);
    }

    _recvBuffer.Clean();
    Receive();
}


void Session::HandlePacket(BYTE* buffer, int32 len)
{
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    // 헤더 바로 뒷부분(실제 Protobuf 데이터가 시작되는 곳)
    BYTE* payload = buffer + sizeof(PacketHeader);
    int32 payloadSize = len - sizeof(PacketHeader);

    switch (header->id)
    {
    case Protocol::PKT_CS_LOGIN:
    {
        Protocol::CS_LOGIN pkt;
        if (pkt.ParseFromArray(payload, payloadSize))
        {
            // 성공! 이제 pkt.userid() 등으로 데이터를 꺼내 씁니다.
            Handle_CS_LOGIN(pkt);
        }
        break;
    }
    // ... 다른 패킷들
    }
}


void Session::Handle_CS_LOGIN(const Protocol::CS_LOGIN& pkt)
{
    std::cout << "로그인 요청 ID: " << pkt.userid() << std::endl;

    //TODO 로그인 유효성 검증 로직 추가
        
    // [수정] google protobuf 도입
    // 응답 전송 (아까 만든 ServerUtils 활용)
    Protocol::SC_LOGIN_OK resPkt;
    resPkt.set_success(true);
    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_LOGIN_OK);
    this->Send(sendBuffer);



}

// [수정]
void Session::Handle_CS_CHAT(const Protocol::CS_CHAT& pkt)
{
    // 1. 받은 패킷에서 데이터 추출 (pkt->chatMsg 대신 pkt.msg() 등 사용)
    // .proto 파일에 정의한 필드명에 따라 함수명이 결정됩니다. (예: string msg -> msg())
    std::string receivedMsg = pkt.msg();

    // 2. 보낼 응답 패킷 생성 및 데이터 채우기
    Protocol::SC_CHAT_BROADCAST res;

    // 플레이어 ID 설정 (소켓 번호보다는 시스템에서 부여한 고유 ID가 좋습니다)
    res.set_playerid(static_cast<uint64>(GetSocket()));

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
}

void Session::Handle_CS_WHISPER(const Protocol::CS_WHISPER& pkt) {

    uint64 targetId = pkt.targetplayerid(); // Protobuf getter
    std::string msg = pkt.msg();

    // 1. 타겟에게 보낼 패킷 생성 (SC_WHISPER 권장)
    Protocol::SC_WHISPER res;
    res.set_fromplayerid(this->GetPlayerId()); // 보낸 사람 ID
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
}

void Session::Send(SendBufferRef sendBuffer)
{
    std::lock_guard<std::mutex> lock(_lock);

    // 1. 보낼 데이터를 큐에 삽입
    _sendQueue.push(sendBuffer);

    // 2. 만약 현재 전송 중인 작업이 없다면 전송 예약 실행
    if (_sendRegistered == false)
    {
        RegisterSend();
    }
}

// 실제로 WSASend를 호출하는 함수
void Session::RegisterSend()
{
    if (_sendQueue.empty())
        return;

    _sendRegistered = true;

    // 큐에 쌓인 버퍼들을 하나로 묶어서 보낼 수 있음 (Scatter-Gather)
    // 여기서는 단순화를 위해 하나만 꺼내 보냄
    SendBufferRef sendBuffer = _sendQueue.front();

    OverlappedEx* overlapped = new OverlappedEx(); // 풀링 권장
    memset(overlapped, 0, sizeof(WSAOVERLAPPED));
    overlapped->type = IO_TYPE::SEND;
    overlapped->owner = this;

    WSABUF wsaBuf;
    wsaBuf.buf = sendBuffer->Buffer();
    wsaBuf.len = sendBuffer->Size();

    DWORD bytesSent = 0;
    // 비동기 전송 호출
    int errorCode = ::WSASend(_socket, &wsaBuf, 1, &bytesSent, 0, &overlapped->overlapped, nullptr);

    if (errorCode == SOCKET_ERROR)
    {
        int err = ::WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            _sendRegistered = false;
            std::cout << "WSASend Error: " << err << std::endl;
        }
    }
}

void Session::OnSend(int bytesTransferred)
{
    std::lock_guard<std::mutex> lock(_lock);

    _sendRegistered = false;

    // 1. 전송이 완료된 버퍼는 큐에서 제거
    if (_sendQueue.empty() == false)
    {
       
        _sendQueue.pop();
        
    }

    // 2. 만약 큐에 더 보낼 데이터가 쌓여있다면 다시 전송 예약
    if (_sendQueue.empty() == false)
    {
        RegisterSend();
    }
}

void Session::Disconnect()
{
    // 중복 호출 방지
    if (_socket == INVALID_SOCKET) return;

    // 소켓을 닫으면 진행 중이던 모든 비동기 IO(WSARecv, WSASend)가 
    // 에러를 발생시키며 GQCS(Dispatch)에서 감지됩니다.
    ::closesocket(_socket);
    _socket = INVALID_SOCKET;

}   

void Session::OnDisconnected()
{

    bool expected = false;
    if (_disconnected.compare_exchange_strong(expected, true))
    {
        GSessionManager.Remove(shared_from_this());

        if (_socket != INVALID_SOCKET) {
            ::closesocket(_socket);
            _socket = INVALID_SOCKET;
        }

        std::cout << "Client Disconnected: " << GetGuid() << std::endl;
    }

}

void Session::OnConnected()
{
    // 접속한 상대방의 정보를 로그로 출력하거나 
    // 서버 환경에 맞는 초기화 패킷 전송 등의 로직을 넣습니다.
    std::cout << "Session Connected: [GUID " << GetGuid() << "]" << std::endl;

    // 만약 접속하자마자 클라이언트에게 환영 패킷을 보내야 한다면 여기서 작성합니다.
    // 예: Send(WelcomePacket);
}