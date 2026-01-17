#pragma once
#include <winsock2.h>
#include <functional>



class Listener
{
public:
    Listener();
    ~Listener();

    // 서버 소켓 초기화 및 바인드/리슨
    bool StartAccept(int port);

    // 접속된 소켓을 처리할 콜백 등록 (보통 Session 생성 로직)
    void SetAcceptHandler(std::function<void(SOCKET)> handler) { _onAcceptHandler = handler; }

    // 실제 접속을 대기하는 루프 (별도 스레드에서 실행)
    void Execute();

private:
    SOCKET _listenSocket = INVALID_SOCKET;
    std::function<void(SOCKET)> _onAcceptHandler;
};