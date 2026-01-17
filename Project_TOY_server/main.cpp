#include "IocpCore.h"
#include "Listener.h"
#include "Session.h"
#include <thread>
#include <vector>
#include <iostream>


#pragma comment(lib, "ws2_32.lib")

int main()
{
    IocpCore iocp;
    Listener listener;

    // 클라이언트 접속 시 실행될 로직 (람다 함수)
    listener.SetAcceptHandler([&](SOCKET clientSocket) {
        // 1. 세션 생성
        Session* session = new Session(clientSocket);

        // 2. IOCP 핸들에 소켓 등록
        if (iocp.Register(session))
        {
            std::cout << "Client Connected and Registered to IOCP" << std::endl;
            // 3. 최초 수신 예약
            session->Receive();
        }
        });

    // 서버 시작 (7777 포트)
    if (listener.StartAccept(7777))
    {
        // 접속 전용 스레드 실행
        std::thread t(&Listener::Execute, &listener);
        t.detach();

        // 4. Worker Thread 풀 구성 (일단 2개만 생성)
        std::vector<std::thread> workerThreads;
        for (int i = 0; i < 2; i++)
        {
            workerThreads.push_back(std::thread([&]() {
                while (true)
                {
                    iocp.Dispatch(); // 여기서 완료된 I/O 처리 (OnRecv, OnSend)
                }
                }));
        }

        for (auto& t : workerThreads) t.join();
    }

    return 0;
}