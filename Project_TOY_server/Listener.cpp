#include "Listener.h"
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

bool Listener::StartAccept(int port)
{
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

void Listener::Execute()
{
    while (true)
    {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);

        // 5. 클라이언트 접속 대기 (동기 accept)
        SOCKET clientSocket = ::accept(_listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

        if (clientSocket != INVALID_SOCKET)
        {
            // 접속 성공 시 등록된 핸들러 호출 (세션 생성 및 IOCP 등록)
            if (_onAcceptHandler)
                _onAcceptHandler(clientSocket);
        }
    }
}