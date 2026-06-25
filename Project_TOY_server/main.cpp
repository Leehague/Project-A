// MSVC 컴파일러에게 문자열 리터럴을 UTF-8로 컴파일하라고 강제 지시
#pragma execution_character_set("utf-8")


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
#include <windows.h> // GetConsoleOutputCP 등을 사용하기 위해 추가
#include "RLModelManager.h"
#include "Creature.h"
#include "Player.h"
#include "Projectile.h"
#include "CoreRoom.h"
#include "Map.h"
#include "Room.h"
#include <future>
#include <chrono>
#include <iomanip>


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

volatile bool g_showStatus = false;

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
        else if (command =="RLspawn")
        {
            std::cout << "Admin: Spawning test five RL monsters..." << std::endl;
            room->MonsterSpawn(1, 1, true);
        }
        else if (command == "monitor")
        {
            std::cout << "Starting real-time monitor. Press Enter to exit..." << std::endl;

            // Windows Console ANSI Virtual Terminal Processing Enable
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode))
            {
                dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
                SetConsoleMode(hOut, dwMode);
            }

            g_showStatus = true;

            std::thread monitorThread([room]() {
                while (g_showStatus)
                {
                    std::promise<void> p;
                    auto f = p.get_future();

                    room->Push([&p, room]() {
                        // Clear screen and home cursor
                        std::cout << "\x1B[2J\x1B[H";

                        std::cout << "======================================================================\n";
                        std::cout << "                PROJECT TOY SERVER REAL-TIME MONITOR                  \n";
                        std::cout << "======================================================================\n";
                        std::cout << "Room ID: " << room->GetRoomid() << " | Map ID: " 
                                  << (room->GetMapptr() ? room->GetMapptr()->GetMapId() : 0) << "\n";

                        auto coreRoom = room->GetCoreRoom();
                        if (coreRoom)
                        {
                            std::cout << "Total Objects: " << coreRoom->_objects.size() << "\n\n";

                            // 1. PLAYERS
                            std::cout << "[PLAYERS]\n";
                            std::cout << "----------------------------------------------------------------------\n";
                            std::cout << std::left << std::setw(8) << "ID" 
                                      << std::setw(15) << "Name" 
                                      << std::setw(25) << "Position (X, Y, Z)" 
                                      << std::setw(12) << "HP" 
                                      << std::setw(10) << "State" << "\n";
                            std::cout << "----------------------------------------------------------------------\n";
                            for (auto& pair : coreRoom->_objects)
                            {
                                if (pair.second->GetType() == GameObjectType::Player)
                                {
                                    auto player = std::static_pointer_cast<Player>(pair.second);
                                    Vector3 pos = player->Getpos_As_Vector3();
                                    char posStr[64];
                                    sprintf_s(posStr, "(%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
                                    char hpStr[32];
                                    sprintf_s(hpStr, "%d/%d", player->GetCurrentHp(), player->GetMaxHP());

                                    std::cout << std::left << std::setw(8) << player->GetObjectId()
                                              << std::setw(15) << player->GetName()
                                              << std::setw(25) << posStr
                                              << std::setw(12) << hpStr
                                              << std::setw(10) << static_cast<int>(player->GetState()) << "\n";
                                }
                            }

                            // 2. MONSTERS
                            std::cout << "\n[MONSTERS]\n";
                            std::cout << "----------------------------------------------------------------------\n";
                            std::cout << std::left << std::setw(8) << "ID" 
                                      << std::setw(15) << "Name" 
                                      << std::setw(25) << "Position (X, Y, Z)" 
                                      << std::setw(12) << "HP" 
                                      << std::setw(10) << "State" 
                                      << std::setw(6) << "RL" << "\n";
                            std::cout << "----------------------------------------------------------------------\n";
                            for (auto& pair : coreRoom->_objects)
                            {
                                if (pair.second->GetType() == GameObjectType::Monster)
                                {
                                    auto monster = std::static_pointer_cast<Monster>(pair.second);
                                    Vector3 pos = monster->Getpos_As_Vector3();
                                    char posStr[64];
                                    sprintf_s(posStr, "(%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
                                    char hpStr[32];
                                    sprintf_s(hpStr, "%d/%d", monster->GetCurrentHp(), monster->GetMaxHP());

                                    std::cout << std::left << std::setw(8) << monster->GetObjectId()
                                              << std::setw(15) << monster->GetName()
                                              << std::setw(25) << posStr
                                              << std::setw(12) << hpStr
                                              << std::setw(10) << static_cast<int>(monster->GetState())
                                              << std::setw(6) << (monster->IsRLControlled() ? "ON" : "OFF") << "\n";
                                }
                            }

                            // 3. PROJECTILES
                            std::cout << "\n[PROJECTILES]\n";
                            std::cout << "----------------------------------------------------------------------\n";
                            std::cout << std::left << std::setw(8) << "ID" 
                                      << std::setw(25) << "Position (X, Y, Z)" 
                                      << std::setw(10) << "State" << "\n";
                            std::cout << "----------------------------------------------------------------------\n";
                            for (auto& pair : coreRoom->_objects)
                            {
                                if (pair.second->GetType() == GameObjectType::Projectile)
                                {
                                    auto proj = std::static_pointer_cast<Projectile>(pair.second);
                                    Vector3 pos = proj->Getpos_As_Vector3();
                                    char posStr[64];
                                    sprintf_s(posStr, "(%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);

                                    std::cout << std::left << std::setw(8) << proj->GetObjectId()
                                              << std::setw(25) << posStr
                                              << std::setw(10) << static_cast<int>(proj->GetState()) << "\n";
                                }
                            }
                        }
                        std::cout << "======================================================================\n";
                        std::cout << "Press Enter to exit status monitoring mode.\n";

                        p.set_value();
                    });

                    f.wait(); // Wait for job to finish printing
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
            });

            std::string dummy;
            std::getline(std::cin, dummy);
            g_showStatus = false;
            monitorThread.join();

            // Clear console and stop
            std::cout << "\x1B[2J\x1B[H";
            std::cout << "Status monitoring stopped." << std::endl;
        }
        else if (command == "exit")
        {
            exit(0);
        }
    }
}
int main()
{
    
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
