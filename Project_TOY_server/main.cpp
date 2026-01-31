#ifndef ABSL_CONSUME_DLL
#define ABSL_CONSUME_DLL
#endif

#ifndef PROTOBUF_USE_DLLS
#define PROTOBUF_USE_DLLS
#endif

#include "IocpCore.h"
#include "Listener.h"
#include "Session.h"
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include <thread>
#include <vector>
#include <iostream>
#include "PacketHandler.h"


#pragma comment(lib, "ws2_32.lib")


int main()
{
    PacketHandler::Init();

    IocpCore iocp;
    Listener listener;

    // 1. 서버 시작 (포트, 세션 생성 방식, IOCP 핵심 객체 전달)
    // 람다 함수는 단순히 "어떤 세션 객체를 만들지"만 결정해서 리턴합니다.
    bool success = listener.StartAccept(
        7777,
        []() { return std::make_shared<Session>(); },
        iocp
    );

    if (success)
    {
        // 2. 접속 전용 스레드 실행
        // 이제 Listener::Execute 내부에서 factory를 써서 세션을 만들고 iocp에 등록합니다.
        std::thread t(&Listener::Execute, &listener, std::ref(iocp));
        t.detach();

        // 3. Worker Thread 풀 구성 
        std::vector<std::thread> workerThreads;
        for (int i = 0; i < 4; i++)
        {
            workerThreads.push_back(std::thread([&iocp]() {
                while (true)
                {
                    iocp.Dispatch();
                }
                }));
        }

        for (auto& t : workerThreads)
            t.join();
    }

    return 0;
}

