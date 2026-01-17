#pragma once
#include <winsock2.h>
#include <functional>
#include "Types.h"
#include "GameSessionManager.h"

// 세션을 생성하는 함수 타입을 정의합니다.
using SessionFactory = std::function<SessionPtr()>;



class Listener
{
public:
    Listener();
    ~Listener();

    // 서버 소켓 초기화 및 바인드/리슨
    /*bool StartAccept(int port);*/

    //// 접속된 소켓을 처리할 콜백 등록 (보통 Session 생성 로직)
    //void SetAcceptHandler(std::function<void(SOCKET)> handler) { _onAcceptHandler = handler; }

    //// 실제 접속을 대기하는 루프 (별도 스레드에서 실행)
    //void Execute();

    // 콜백 대신 팩토리를 등록받습니다.
    bool StartAccept(int port, SessionFactory factory, class IocpCore& iocp);
    void Execute(IocpCore& iocp); // iocp를 인자로 받아 내부에서 등록까지 처리

private:
    /*SOCKET _listenSocket = INVALID_SOCKET;
    std::function<void(SOCKET)> _onAcceptHandler;*/

    SOCKET _listenSocket = INVALID_SOCKET;
    SessionFactory _sessionFactory;
    IocpCore* _iocp = nullptr;
};