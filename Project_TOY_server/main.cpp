#include "IocpCore.h"
#include "Listener.h"
#include "Session.h"
#include "Types.h"
#include <thread>
#include <vector>
#include <iostream>


#pragma comment(lib, "ws2_32.lib")


int main()
{
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

        // 3. Worker Thread 풀 구성 (동일)
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


//int main()
//{
//    IocpCore iocp;
//    Listener listener;
//
//    // "나는 Session 클래스를 사용할 거야"라고 선언만 하면 끝
//    listener.StartAccept(7777, []() { return std::make_shared<Session>(); },iocp);
//
//    // 접속 스레드 실행
//    std::thread t(&Listener::Execute, &listener, std::ref(iocp));
//    t.detach();
//
//    // 서버 시작 (7777 포트)
//    if (listener.StartAccept(7777))
//    {
//        // 접속 전용 스레드 실행
//        std::thread t(&Listener::Execute, &listener);
//        t.detach();
//
//        // 4. Worker Thread 풀 구성 (일단 2개만 생성)
//        std::vector<std::thread> workerThreads;
//        for (int i = 0; i < 2; i++)
//        {
//            workerThreads.push_back(std::thread([&]() {
//                while (true)
//                {
//                    iocp.Dispatch(); // 여기서 완료된 I/O 처리 (OnRecv, OnSend)
//                }
//                }));
//        }
//
//        for (auto& t : workerThreads) t.join();
//    }
//
//    return 0;
//}