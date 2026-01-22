#pragma once
#include <memory>
#include <winsock2.h>
#include <queue>
#include <mutex>
#include <atomic> 
#include "RecvBuffer.h"
#include "SendBuffer.h"
#include "Protocol.h"
#include "GameSessionManager.h"
#include "Types.h"

enum class IO_TYPE { RECV, SEND };

struct OverlappedEx {
    WSAOVERLAPPED overlapped; // OS가 요구하는 기본 구조체 (반드시 맨 앞)
    IO_TYPE type;             // 어떤 작업인지 구분
    void* owner;              // 이 작업을 요청한 Session 객체 주소
};

class Session :public std::enable_shared_from_this<Session>
{
public:
    Session();
    Session(SOCKET socket);
    ~Session(); 

    // 외부에서 호출하는 송수신 명령
    void Send(SendBufferRef sendBuffer);
    void Receive();

    // IOCP 워커 스레드가 완료 통보를 받았을 때 호출할 콜백
    void OnRecv(int bytesTransferred);
    void OnSend(int bytesTransferred);
    void Disconnect();
    void OnDisconnected();

    // 자기 자신을 SessionPtr로 반환하는 헬퍼 함수
    SessionPtr GetSessionPtr() { return shared_from_this(); }

    uint64 GetGuid() { return static_cast<uint64>(_socket); }

    // 소켓 설정 및 게터
    void SetSocket(SOCKET socket) { _socket = socket; }
    SOCKET GetSocket() { return _socket; }

    // 접속 완료 처리
    void OnConnected();

private:
    SOCKET      _socket= INVALID_SOCKET;
    RecvBuffer  _recvBuffer;
    
    
    std::atomic<bool> _disconnected = false; //Remove 중복실행 방지 플래그
    
private:
    std::mutex              _lock;
    std::queue<SendBufferRef> _sendQueue; // 보낼 데이터 대기열
    bool                    _sendRegistered = false; // 현재 전송 예약 중인지 여부

private:
    void RegisterSend(); // 실제로 WSASend를 호출하는 함수
    //handler 함수들
    void HandlePacket(PacketHeader* header);
    void Handle_CS_LOGIN(PacketHeader* header);
    void Handle_CS_CHAT(PacketHeader* header);
    void Handle_CS_WHISPER(PacketHeader* header);
};