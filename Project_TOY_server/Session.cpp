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
    // [추가] 만약 여유 공간이 없으면 Clean을 한 번 더 시도하거나 에러 처리
    if (_recvBuffer.FreeSize() <= 0)
    {
        _recvBuffer.Clean();
        // Clean 후에도 공간이 없다면 버퍼 크기 자체가 너무 작은 것
        if (_recvBuffer.FreeSize() <= 0) { OnDisconnected(); return; }
    }

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

        //logging
        std::cout <<"header size : " << header->size << std::endl;;

        // 서버의 OnRecv 혹은 패킷 분기 로직

        uint16_t packetId = header->id;

        // GPacketHandler 크기 체크
        if (packetId >= MAX_PACKET_ID) {
            std::cout << "Error: Invalid Packet ID " << packetId << std::endl;
            return; // 여기서 걸린다면 벡터 크기 초기화 문제!
        }

        if (GPacketHandler[packetId] == nullptr) {
            std::cout << "Error: No Handler for ID " << packetId << std::endl;
            return;
        }
        // [중요] 비정상적인 대형 패킷 방어
        if (header->size > 1024 * 5) { OnDisconnected(); return; }

        // 3. 전체 패킷이 다 왔는지 확인
        if (dataSize < header->size)
            break;

        // 4. 패킷 처리 함수로 전달
        //HandlePacket(reinterpret_cast<BYTE*>(_recvBuffer.ReadPos()), header->size);

        SessionRef sessionRef = GetSessionPtr();
        PacketHandler::HandlePacket(sessionRef, reinterpret_cast<BYTE*>(_recvBuffer.ReadPos()), header->size);

        // 5. 처리한 패킷 크기만큼 읽기 커서 이동
        _recvBuffer.OnRead(header->size);
    }

    _recvBuffer.Clean();
    Receive();
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