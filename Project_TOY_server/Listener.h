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

    
    // 콜백 대신 팩토리를 등록받습니다.
    bool StartAccept(int port, SessionFactory factory, class IocpCore& iocp);
    void Execute(IocpCore& iocp); // iocp를 인자로 받아 내부에서 등록까지 처리

private:
    SOCKET _listenSocket = INVALID_SOCKET;
    SessionFactory _sessionFactory;
    IocpCore* _iocp = nullptr;
};
