#include "Listener.h"
#include "Session.h"
#include "IocpCore.h"
#include <iostream>

Listener::Listener()
{
}

Listener::~Listener()
{
    if (_listenSocket != INVALID_SOCKET)
        ::closesocket(_listenSocket);
    ::WSACleanup();
}

bool Listener::StartAccept(int port, SessionFactory factory, IocpCore& iocp)
{
    _sessionFactory = factory; // 전달받은 람다 보관
    _iocp = &iocp;             // 전달받은 IOCP 객체 주소 보관
    // 1. 윈속 초기화
    WSAData wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

    // 2. 리슨 소켓 생성
    _listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (_listenSocket == INVALID_SOCKET) return false;

    // 3. 주소 설정
    SOCKADDR_IN serverAddr;
    ::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    serverAddr.sin_port = ::htons(port);

    // 4. 바인드 및 리슨
    if (::bind(_listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) return false;
    if (::listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR) return false;

    std::cout << "Server started on port " << port << "..." << std::endl;
    return true;
}


void Listener::Execute(IocpCore& iocp) {
    while (true) {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = ::accept(_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

        if (clientSocket != INVALID_SOCKET) {
            // 1. 등록된 팩토리로 세션 생성
            SessionPtr session = _sessionFactory();
            session->SetSocket(clientSocket);
            
            // 2. 관리자 등록 및 IOCP 등록을 여기서 전담
            GSessionManager.Add(session);
            if (iocp.Register(session)) {
                session->OnConnected();
                session->Receive(); // 최초 수신 예약
            }
        }
    }
}