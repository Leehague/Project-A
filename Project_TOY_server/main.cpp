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
#include "JobSerializer.h"
#include <sqlext.h>
#include "DBConnection.h"
#include "DBManager.h"


#pragma comment(lib, "ws2_32.lib")




void WorkerThread(IocpCore& iocp)
{
    while (true)
    {
        // [네트워크 일감] 우선 순위 1
        // 타임아웃을 짧게 주어(예: 10ms) 네트워크가 없으면 빠르게 다음으로 넘어갑니다.
        iocp.Dispatch(10);

        // [게임 로직 일감] 우선 순위 2
        // 실행 대기 중인 JobQueue(Room 등)를 꺼내어 처리합니다.
        while (auto jobQueue = GJobSerializer.Pop())
        {
            
            // Monster 등 다른 JobQueue인 경우 처리
            jobQueue->Execute();
            
        }
    }
}

void ConsoleThread(RoomPtr room)
{
    while (true)
    {
        std::string command;
        std::cout << ">> ";
        std::getline(std::cin, command);

        if (command == "spawn")
        {
            
            std::cout << "Admin: Spawning test five monsters..." << std::endl;

            room->MonsterSpawn(5,1);
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
    GMapManager.Init();
    IocpCore iocp;
    Listener listener;

    // [수정] DB 매니저 초기화 (커넥션 풀 5개 생성 및 워커 스레드 시작)
   

    std::wstring connStr = L"Driver={ODBC Driver 17 for SQL Server};Server=DESKTOP-IFVE1ON\\SQLEXPRESS;Database=ProjectA_DB;Trusted_Connection=yes;";
    if (DBManager::GetInstance().Init(5, connStr))
    {
        std::cout << "DBManager Init Success!" << std::endl;
    }
    else
    {
        std::cout << "DBManager Init Failed..." << std::endl;
        return -1; // DB 연결 실패 시 서버 종료
    }


    std::cout << GRoomManager.Create(1) << "번 방 생성" << std::endl;

    RoomPtr defaultRoom = GRoomManager.FindRoom(1);
    std::thread consoleThread(ConsoleThread, defaultRoom);
    consoleThread.detach();

    bool success = listener.StartAccept(
        7777,
        []() { return std::make_shared<Session>(); },
        iocp
    );

    if (success)
    {
        std::thread t(&Listener::Execute, &listener, std::ref(iocp));
        t.detach();

        // [수정] 3. Worker Thread 풀 구성 
        // 정의해둔 WorkerThread 함수를 사용하여 네트워크와 로직을 모두 처리하게 합니다.
        std::vector<std::thread> workerThreads;
        for (int i = 0; i < 4; i++)
        {
            workerThreads.push_back(std::thread(WorkerThread, std::ref(iocp)));
        }

        // 4. [Main Thread 전용] 주기적인 로직 업데이트 (Tick 관리)
        // main.cpp의 while 루프
        while (true)
        {
            auto rooms = GRoomManager.GetRooms();
            for (auto& room : rooms)
            {
                // 룸 자체를 시리얼라이저에 등록하여 워커 쓰레드가 처리하게 함
                GJobSerializer.Push(room);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 적절한 틱 간격
        }

        for (auto& t : workerThreads)
            t.join();
    }

    return 0;
}

