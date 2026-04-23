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
#include "RoomManager.h"
#include "DataManager.h"
#include "MapManager.h"
#include "Monster.h"
#include "GameObject.h"
#include "ObjectManager.h"

#pragma comment(lib, "ws2_32.lib")

void ConsoleThread(RoomPtr room)
{
    while (true)
    {
        std::string command;
        std::cout << ">> ";
        std::getline(std::cin, command);

        if (command == "spawn")
        {
            // 주의: 여기서 직접 Room 데이터에 접근하면 Race Condition 위험이 있음!
            // 지금은 임시로 직접 호출하거나, 나중에 JobQueue에 넣으세요.
            std::cout << "Admin: Spawning test monsters..." << std::endl;

            // 테스트용 몬스터 생성 및 입장 로직 jobqueue로 수정해야함
            std::vector<MonsterPtr> monsters;
            for (int i = 0; i < 5; i++)
            {
                MonsterPtr monster = std::static_pointer_cast<Monster>(
                    GObjcetManager.Create(GameObjectType::Monster,nullptr, 2) //몬스터는 session이 필요없기 때문에 nullptr로 전달
                );
                monster->Getpos().set_x(10.0f + i * 2.0f);
                monster->Getpos().set_z(10.0f);

                monsters.push_back(monster);
            }
            room->EnterMonsters(monsters);
        }
        else if (command == "exit")
        {
            exit(0);
        }
    }
}
int main()
{
    PacketHandler::Init();
    DataManager::GetInstance().Init();
    GMapManager.Init(); // DataManager의 Init 이후에 실행되어야 함.
    IocpCore iocp;
    Listener listener;

    std::cout << GRoomManager.Create(1) << "번 방 생성" << std::endl; //0번방 생성 , 1번 맵으로 초기화
    
    RoomPtr defaultRoom = GRoomManager.FindRoom(1);
    // 콘솔 입력 쓰레드 시작
    std::thread consoleThread(ConsoleThread, defaultRoom);
    consoleThread.detach(); // 메인 쓰레드와 별개로 동작하게 분리

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

