// MSVC 컴파일러에게 문자열 리터럴을 UTF-8로 컴파일하라고 강제 지시
#pragma execution_character_set("utf-8")

#pragma once
#include "IocpCore.h"
#include "Listener.h"
#include "Session.h"
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "PacketHandler.h"
#include "RoomManager.h"
#include "DataManager.h"
#include "MapManager.h"
#include "Monster.h"
#include "GameObject.h"
#include "ObjectManager.h"
#include "JobSerializer.h"
#include "DBConnection.h"
#include "DBManager.h"
#include "RLModelManager.h"
#include "Creature.h"
#include "Player.h"
#include "Projectile.h"
#include "CoreRoom.h"
#include "Map.h"
#include "Room.h"
#include "ConsoleManager.h"
#include "LoginManager.h"
#include <thread>
#include <vector>
#include <future>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sqlext.h>
#include <windows.h> // GetConsoleOutputCP 등을 사용하기 위해 추가


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

//volatile bool g_showStatus = false;


int main()
{
    //ServerInit
    ::SetConsoleOutputCP(CP_UTF8); 

    PacketHandler::Init();
    DataManager::GetInstance().Init();
    GMapManager.Init();
    RLModelManager::GetInstance().Init(L"models/monster_ppo_model.onnx");

    IocpCore iocp;
    Listener listener;

    // DB 매니저 초기화 (커넥션 풀 5개 생성 및 워커 스레드 시작)
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

    // Redis 매니저 초기화 (로컬 도커 Redis 기본 포트 6379로 연결)
    if (LoginManager::GetInstance().Init("127.0.0.1", 6379) == false)
    {
        std::cout << "Redis Initialization Failed. Exit Server." << std::endl;
        return 0;
    }


    
    ConsoleManager::GetInstance().StartConsoleThread();

  

    bool success = listener.StartAccept(
        7777,
        []() { return std::make_shared<Session>(); },
        iocp
    );

    if (success)
    {
        std::thread t(&Listener::Execute, &listener, std::ref(iocp));
        t.detach();

        // Worker Thread 풀 구성 
        // 정의해둔 WorkerThread 함수를 사용하여 네트워크와 로직을 모두 처리하게 합니다.
        std::vector<std::thread> workerThreads;
        for (int i = 0; i < 4; i++)
        {
            workerThreads.push_back(std::thread(WorkerThread, std::ref(iocp)));
        }

        //[Main Thread 전용] 주기적인 로직 업데이트 (Tick 관리)
        
        while (true)
        {
            auto rooms = GRoomManager.GetRooms();
            for (auto& room : rooms)
            {
                // 룸 자체를 시리얼라이저에 등록하여 워커 쓰레드가 처리하게 함
                //GJobSerializer.Push(room);

                // 강제 Push 대신, 룸의 JobQueue를 경유하여 Update 일감을 등록합니다.
                // 이렇게 하면 _isInsideJobQueue가 내부적으로 true가 되어 중복 Push가 차단됩니다.
                std::weak_ptr<Room> weakRoom = room;
                room->Push([weakRoom]() {
                    if (auto r = weakRoom.lock()) {
                        r->Execute(); // 기존 Execute() 로직 수행
                    }
                });
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 적절한 틱 간격
        }

        for (auto& t : workerThreads)
            t.join();
    }

    return 0;
}
