#include "Session.h"
#include <iostream>
#include "RoomManager.h"
#include "ObjectManager.h"
#include "Player.h"


Session::Session() : _recvBuffer(1024 * 64)
{

}

Session::Session(SOCKET socket) : _socket(socket), _recvBuffer(1024 * 64) // 64KB 버퍼
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
    if (_socket == INVALID_SOCKET || _disconnected.load()) return;

    // [추가] 만약 여유 공간이 없으면 Clean을 한 번 더 시도하거나 에러 처리
    if (_recvBuffer.FreeSize() < sizeof(PacketHeader))
    {
        _recvBuffer.Clean();
        // Clean 후에도 공간이 없다면 버퍼 크기 자체가 너무 작은 것
        if (_recvBuffer.FreeSize() < sizeof(PacketHeader))
        { 
            std::cout << "Session::Receive , recvBuffer is small" << std::endl;
            OnDisconnected();
            return; 
        }
    }

    // Overlapped 정보를 설정 (나중에 IOCP에서 이 정보를 보고 처리함)
    OverlappedEx* overlapped = new OverlappedEx(); // 실제로는 풀링해서 써야 함
    memset(overlapped, 0, sizeof(WSAOVERLAPPED));
    overlapped->type = IO_TYPE::RECV;
    overlapped->owner = shared_from_this();

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
            // [강화] 에러 코드와 함께 현재 세션 정보 출력
            std::cout << "WSARecv Error [Socket: " << _socket << "]: " << err << std::endl;
            delete overlapped; // 에러 시 메모리 누수 방지
        }
    }
}

//수신 완료 콜백 (IOCP Worker Thread에 의해 호출됨)
void Session::OnRecv(int bytesTransferred)
{
    if (bytesTransferred == 0) 
    {
        std::cout << "bytesTransferred is zero" << std::endl;
        OnDisconnected(); 
        return; 
    }
    if (_recvBuffer.OnWrite(bytesTransferred) == false) 
    { 
        std::cout << "Session::OnRecv , recvBuffer is full (OnWrite failed)" << std::endl;
        OnDisconnected(); 
        return;
    }

    //loging
    //std::cout << "--- OnRecv Start (Bytes: " << bytesTransferred << ") ---" << std::endl;


    while (true)
    {
        int dataSize = _recvBuffer.DataSize();

        // 1. 헤더(4바이트)만큼은 왔는지 확인
        if (dataSize < sizeof(PacketHeader)) {
           // std::cout << "Wait for Header... (Current: " << dataSize << ")" << std::endl;
            break;
        }
        // 2. 패킷 헤더를 읽어 전체 크기 확인
        PacketHeader* header = reinterpret_cast<PacketHeader*>(_recvBuffer.ReadPos());

        
                
        // 서버의 OnRecv 혹은 패킷 분기 로직

        uint16_t packetId = header->id;

        
        // GPacketHandler 크기 체크
        if (packetId >= MAX_PACKET_ID) {
            std::cout << "Error: Invalid Packet ID " << packetId << std::endl;
            OnDisconnected();
            return; // 여기서 걸린다면 벡터 크기 초기화 문제!
        }

        if (GPacketHandler[packetId] == nullptr) {
            std::cout << "Error: No Handler for ID " << packetId << std::endl;
            OnDisconnected();
            return;
        }
        // [중요] 비정상적인 대형 패킷 방어
        if (header->size > 1024 * 5) 
        {
            OnDisconnected();
            std::cout << "Unnormal big packet is defend" << std::endl; 
            return;
        }

        // 3. 전체 패킷이 다 왔는지 확인
        if (dataSize < header->size)
        {
            std::cout << "Wait for Data... (Need: " << header->size << " / Have: " << dataSize << ")" << std::endl;
            break;
        }
        uint16 id = header->id;
        uint16 size = header->size;

        // [디버깅 로그]
        //std::cout << "Processing Packet ID: " << header->id << " / Size: " << header->size << std::endl;
        

        SessionPtr _sessionPtr = GetSessionPtr();
        if (PacketHandler::HandlePacket(_sessionPtr, reinterpret_cast<BYTE*>(_recvBuffer.ReadPos()), header->size)==false)
        {
            OnDisconnected();
            std::cout << "HandlePacket Fail! [PacketID: " << header->id << "][Size: " << header->size << "]" << std::endl;
                       
            return;
        }

        // 5. 처리한 패킷 크기만큼 읽기 커서 이동
        _recvBuffer.OnRead(header->size);
        //std::cout << "Packet Processed Successfully." << std::endl;
    }
    //std::cout << "--- OnRecv Loop End, Calling Receive() ---" << std::endl;
    _recvBuffer.Clean();
    
    Receive();
}

void Session::Send(SendBufferPtr sendBuffer)
{
    std::lock_guard<std::mutex> lock(_lock);

    // 1. 보낼 데이터를 큐에 삽입
    _sendQueue.push(sendBuffer);

    // 2. 만약 현재 전송 중인 작업이 없다면 전송 예약 실행
    if (_sendRegistered == false)
    {
        //틱 체크, 패킷전송이 '너무 자주' 되는 것 방지
        uint64 currentTick = ::GetTickCount64();
        if (currentTick - _lastSendTick >= SEND_TICK_INTERVAL)
        {
            _lastSendTick = currentTick;
            RegisterSend();
        }
    }
}

// 실제로 WSASend를 호출하는 함수
void Session::RegisterSend()
{
    if (_socket == INVALID_SOCKET || _disconnected.load()) return;
    if (_sendQueue.empty())
        return;

    _sendRegistered = true;

    // 큐에 쌓인 버퍼들을 하나로 묶어서 보낼 수 있음 (Scatter-Gather)
    // 여기서는 단순화를 위해 하나만 꺼내 보냄
    SendBufferPtr sendBuffer = _sendQueue.front();

    if (!sendBuffer) {
        // defensive: drop invalid entry
        _sendQueue.pop();
        _sendRegistered = false;
        if (!_sendQueue.empty()) RegisterSend();
        return;
    }


    OverlappedEx* overlapped = new OverlappedEx(); // 풀링 권장
    memset(overlapped, 0, sizeof(WSAOVERLAPPED));
    overlapped->type = IO_TYPE::SEND;
    overlapped->owner = shared_from_this();

    WSABUF wsaBuf;
    wsaBuf.buf = sendBuffer->Buffer();
    wsaBuf.len = sendBuffer->Size();

    DWORD bytesSent = 0;
    // 비동기 전송 호출
    if (::WSASend(_socket, &wsaBuf, 1, &bytesSent, 0, &overlapped->overlapped, nullptr) == SOCKET_ERROR)
    {
        int err = ::WSAGetLastError();
        if (err != WSA_IO_PENDING)
        {
            std::cout << "WSASend Error [Socket: " << _socket << "]: " << err << std::endl;
            _sendRegistered = false;
            // treat as fatal: ensure proper cleanup
            OnDisconnected();
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
    // 원자적으로 체크하여 딱 한 번만 실행되도록 보장
    if (_disconnected.exchange(true) == true)
        return;
    std::cout << "OnDisconnected Called. Stack Trace Trace..." << std::endl;
    std::cout << "Client Disconnected: " << GetGuid() << std::endl;
    // 여기서 세션 매니저에서 제거
    auto self = shared_from_this();
    GSessionManager.Remove(self);

    if (_socket != INVALID_SOCKET) {
        ::closesocket(_socket);
        _socket = INVALID_SOCKET;
    }
    
    //게임 오브젝트 해제

    PlayerPtr player = GetPlayerPtr();
    if (player)
    {
        RoomPtr room = GRoomManager.FindRoom(player->GetroomId());
        if (room)
            room->Leave(player); // 룸에서 퇴장 처리

        GObjcetManager.Removeobjcet(player->GetObjectId()); // 전역 매니저에서도 제거
    }

}

void Session::OnConnected()
{
    // 접속한 상대방의 정보를 로그로 출력하거나 
    // 서버 환경에 맞는 초기화 패킷 전송 등의 로직을 넣습니다.
    std::cout << "Session Connected: [GUID " << GetGuid() << "]" << std::endl;

    
}