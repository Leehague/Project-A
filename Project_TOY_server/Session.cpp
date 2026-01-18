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

// [1] 수신 예약 (WSARecv)
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

// [2] 수신 완료 콜백 (IOCP Worker Thread에 의해 호출됨)
void Session::OnRecv(int bytesTransferred)
{
    if (bytesTransferred == 0) { OnDisconnected(); return; }
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
        HandlePacket(header);

        // 5. 처리한 패킷 크기만큼 읽기 커서 이동
        _recvBuffer.OnRead(header->size);
    }

    _recvBuffer.Clean();
    Receive();
}

void Session::HandlePacket(PacketHeader* header)
{
    switch (header->type)
    {
    case PKT_CS_LOGIN:
        Handle_CS_LOGIN(header);
        break;
    case PKT_CS_CHAT:
        Handle_CS_CHAT(header);
        break;
    }
}
void Session::Handle_CS_LOGIN(PacketHeader* header)
{
    PKT_CS_LOGIN_DATA* pkt = reinterpret_cast<PKT_CS_LOGIN_DATA*>(header);
    std::cout << "로그인 요청 ID: " << pkt->userId << std::endl;

    // 응답 패킷 만들기 (LOGIN_OK)
    PKT_SC_LOGIN_OK_DATA resPkt;
    resPkt.header.size = sizeof(resPkt);
    resPkt.header.type = PKT_SC_LOGIN_OK;
    resPkt.success = true;
    resPkt.playerGuid = 1234; //TEMP 클라이언트 고유 식별 번호 부여 알고리즘 필요 /DB 추가시 DB를 활용
    //[수정] , 스마트포인터 사용
    SendBufferRef sb = std::make_shared<SendBuffer>(sizeof(PKT_SC_LOGIN_OK_DATA));
    sb->CopyData(resPkt);
    Send(sb);
}
void Session::Handle_CS_CHAT(PacketHeader* header)
{
    PKT_CS_CHAT_DATA* pkt = reinterpret_cast<PKT_CS_CHAT_DATA*>(header);

    // [수정] 스마트 포인터로 생성
    SendBufferRef sendBuffer = std::make_shared<SendBuffer>(sizeof(PKT_SC_CHAT_BROADCAST_DATA));

    PKT_SC_CHAT_BROADCAST_DATA res;
    res.header.size = sizeof(res);
    res.header.type = PKT_SC_CHAT_BROADCAST;
    res.playerId = (int)GetSocket();
    strcpy_s(res.chatMsg, pkt->chatMsg);

    sendBuffer->CopyData(res);

    // 브로드캐스팅
    GSessionManager.Broadcast(sendBuffer);
}

void Session::OnDisconnected()
{
    
    // 2. 연결 종료 시 매니저에서 삭제 (필수!)
    GSessionManager.Remove(shared_from_this());
    std::cout << "Client Disconnected" << std::endl;
    
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

void Session::Handle_CS_WHISPER(PacketHeader* header) {
    PKT_CS_WHISPER_DATA* pkt = reinterpret_cast<PKT_CS_WHISPER_DATA*>(header);

    // 전달할 패킷 생성
    SendBufferRef sendBuffer = std::make_shared<SendBuffer>(sizeof(PKT_SC_WHISPER_DATA));

    PKT_SC_WHISPER_DATA res;
    res.header.size = sizeof(res);
    res.header.type = PKT_SC_WHISPER;
    res.fromId = this->GetGuid(); // 보낸 사람 (나)
    strcpy_s(res.chatMsg, pkt->chatMsg);

    sendBuffer->CopyData(res);

    // 매니저를 통해 특정 타겟에게만 발송
    GSessionManager.SendTo(pkt->targetId, sendBuffer);
}

void Session::OnConnected()
{
    // 접속한 상대방의 정보를 로그로 출력하거나 
    // 서버 환경에 맞는 초기화 패킷 전송 등의 로직을 넣습니다.
    std::cout << "Session Connected: [GUID " << GetGuid() << "]" << std::endl;

    // 만약 접속하자마자 클라이언트에게 환영 패킷을 보내야 한다면 여기서 작성합니다.
    // 예: Send(WelcomePacket);
}