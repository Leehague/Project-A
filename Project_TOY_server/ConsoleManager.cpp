#include "ConsoleManager.h"
#include "RoomManager.h"
#include "Room.h"
#include "Map.h"
#include "CoreRoom.h"
#include "Player.h"
#include "Monster.h"
#include "Projectile.h"
#include "Vector3.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <future>
#include <windows.h>

ConsoleManager::~ConsoleManager()
{
    StopConsoleThread();
}

void ConsoleManager::StartConsoleThread()
{
    if (_isRunning) return;
    _isRunning = true;
    _consoleThread = std::thread(&ConsoleManager::ConsoleLoop, this);
    _consoleThread.detach();
}

void ConsoleManager::StopConsoleThread()
{
    _isRunning = false;
}

// 문자열을 공백 단위로 쪼개주는 토큰화 헬퍼 함수
static std::vector<std::string> Tokenize(const std::string& str)
{
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (ss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

void ConsoleManager::ConsoleLoop()
{
    std::cout << "[Admin Console Active] Type 'help' for commands.\n" << std::endl;
    while (_isRunning)
    {
        std::string command;
        std::cout << ">> ";
        if (!std::getline(std::cin, command))
            break;

        if (command.empty()) continue;

        ProcessCommand(command);
    }
}

void ConsoleManager::ProcessCommand(const std::string& fullCommand)
{
    std::vector<std::string> tokens = Tokenize(fullCommand);
    if (tokens.empty()) return;

    std::string baseCmd = tokens[0];
    std::vector<std::string> args(tokens.begin() + 1, tokens.end());

    if (baseCmd == "help")
    {
        std::cout << "=================== Command List ===================\n"
            << "  help                                 : 도움말 표시\n"
            << "  room_create [mapId]                  : 신규 방 동적 생성\n"
            << "  room_list                            : 개설된 모든 방 상태 출력\n"
            << "  spawn [roomId] [count] [templateId]  : 특정 방에 몬스터 스폰\n"
            << "  rlspawn [roomId] [count] [templateId]: 특정 방에 RL 몬스터 스폰\n"
            << "  husuabi_spawn [roomId]               : 특정 방에 허수아비 몬스터 스폰\n"
            << "  monitor [roomId]                     : 특정 방 실시간 모니터링 시작\n"
            << "====================================================\n" << std::endl;
    }
    else if (baseCmd == "room_create")      CmdCreateRoom(args);
    else if (baseCmd == "room_list")        CmdListRooms();
    else if (baseCmd == "spawn")            CmdSpawn(args);
    else if (baseCmd == "rlspawn")          CmdRLSpawn(args);
    else if (baseCmd == "husuabi_spawn")    CmdHusuabiSpawn(args);
    else if (baseCmd == "monitor")          CmdMonitor(args);
    else
    {
        std::cout << "Unknown command. Type 'help'." << std::endl;
    }
}

// room_create [mapId]
void ConsoleManager::CmdCreateRoom(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        std::cout << "Usage: room_create [mapId]" << std::endl;
        return;
    }

    int32 mapId = std::stoi(args[0]);
    int32 roomId = GRoomManager.Create(mapId);
    std::cout << "Successfully created Room ID: " << roomId << " with Map ID: " << mapId << std::endl;
}

// room_list
void ConsoleManager::CmdListRooms()
{
    auto rooms = GRoomManager.GetRooms();
    std::cout << "================= Active Room List =================" << std::endl;
    std::cout << std::left << std::setw(10) << "RoomID"
        << std::setw(10) << "MapID"
        << std::setw(15) << "PlayersCount" << std::endl;
    std::cout << "----------------------------------------------------" << std::endl;

    for (auto& room : rooms)
    {
        int32 mapId = 0;
        if (room->GetMapptr()) mapId = room->GetMapptr()->GetMapId();

        int32 playersCount = 0;
        if (room->GetCoreRoom())
        {
            for (auto& pair : room->GetCoreRoom()->_objects)
            {
                if (pair.second->GetType() == GameObjectType::Player)
                    playersCount++;
            }
        }
        std::cout << std::left << std::setw(10) << room->GetRoomid()
            << std::setw(10) << mapId
            << std::setw(15) << playersCount << std::endl;
    }
    std::cout << "====================================================\n" << std::endl;
}

// spawn [roomId] [count] [templateId]
void ConsoleManager::CmdSpawn(const std::vector<std::string>& args)
{
    if (args.size() < 3)
    {
        std::cout << "Usage: spawn [roomId] [count] [templateId]" << std::endl;
        return;
    }
    int32 roomId = std::stoi(args[0]);
    int32 count = std::stoi(args[1]);
    int32 templateId = std::stoi(args[2]);

    RoomPtr room = GRoomManager.FindRoom(roomId);
    if (!room)
    {
        std::cout << "Room ID " << roomId << " not found." << std::endl;
        return;
    }

    room->Push([room, count, templateId]() {
        room->MonsterSpawn(count, templateId);
        std::cout << "Admin: Spawned " << count << " monster(s) in Room " << room->GetRoomid() << std::endl;
        });
}

// rlspawn [roomId] [count] [templateId]
void ConsoleManager::CmdRLSpawn(const std::vector<std::string>& args)
{
    if (args.size() < 3)
    {
        std::cout << "Usage: rlspawn [roomId] [count] [templateId]" << std::endl;
        return;
    }
    int32 roomId = std::stoi(args[0]);
    int32 count = std::stoi(args[1]);
    int32 templateId = std::stoi(args[2]);

    RoomPtr room = GRoomManager.FindRoom(roomId);
    if (!room)
    {
        std::cout << "Room ID " << roomId << " not found." << std::endl;
        return;
    }

    room->Push([room, count, templateId]() {
        room->MonsterSpawn(count, templateId, true);
        std::cout << "Admin: Spawned " << count << " RL monster(s) in Room " << room->GetRoomid() << std::endl;
        });
}

// husuabi_spawn [roomId]
void ConsoleManager::CmdHusuabiSpawn(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        std::cout << "Usage: husuabi_spawn [roomId]" << std::endl;
        return;
    }
    int32 roomId = std::stoi(args[0]);

    RoomPtr room = GRoomManager.FindRoom(roomId);
    if (!room)
    {
        std::cout << "Room ID " << roomId << " not found." << std::endl;
        return;
    }

    room->Push([room]() {
        room->HusuabiMonsterSpawn(1, 1);
        std::cout << "Admin: Spawned 1 husuabi in Room " << room->GetRoomid() << std::endl;
        });
}

// monitor [roomId]
void ConsoleManager::CmdMonitor(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        std::cout << "Usage: monitor [roomId]" << std::endl;
        return;
    }
    int32 roomId = std::stoi(args[0]);
    RoomPtr room = GRoomManager.FindRoom(roomId);
    if (!room)
    {
        std::cout << "Room ID " << roomId << " not found." << std::endl;
        return;
    }

    std::cout << "Starting real-time monitor for Room " << roomId << ". Press Enter to exit..." << std::endl;

    // ANSI 가상 터미널 설정 지원
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    if (hOut != INVALID_HANDLE_VALUE && GetConsoleMode(hOut, &dwMode))
    {
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
    }

    _showStatus = true;

    // 모니터링을 실시간 갱신할 백그라운드 스레드 생성
    std::thread monitorThread([this, room]() {
        while (_showStatus)
        {
            std::promise<void> p;
            auto f = p.get_future();

            room->Push([&p, room]() {
                std::cout << "\x1B[2J\x1B[H"; // 화면 클리어 및 홈 위치 이동

                std::cout << "======================================================================\n";
                std::cout << "                PROJECT TOY SERVER REAL-TIME MONITOR                  \n";
                std::cout << "======================================================================\n";
                std::cout << "Room ID: " << room->GetRoomid() << " | Map ID: "
                    << (room->GetMapptr() ? room->GetMapptr()->GetMapId() : 0) << "\n";

                auto coreRoom = room->GetCoreRoom();
                if (coreRoom)
                {
                    std::cout << "Total Objects: " << coreRoom->_objects.size() << "\n\n";

                    // [기존 PLAYERS 출력부]
                    std::cout << "[PLAYERS]\n";
                    std::cout << "----------------------------------------------------------------------\n";
                    for (auto& pair : coreRoom->_objects)
                    {
                        if (pair.second->GetType() == GameObjectType::Player)
                        {
                            auto player = std::static_pointer_cast<Player>(pair.second);
                            Vector3 pos = player->Getpos_As_Vector3();
                            std::cout << "ID: " << player->GetObjectId()
                                << " | Name: " << player->GetName()
                                << " | Pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")"
                                << " | HP: " << player->GetCurrentHp() << "/" << player->GetMaxHP() << "\n";
                        }
                    }

                    // [기존 MONSTERS 출력부]
                    std::cout << "\n[MONSTERS]\n";
                    std::cout << "----------------------------------------------------------------------\n";
                    for (auto& pair : coreRoom->_objects)
                    {
                        if (pair.second->GetType() == GameObjectType::Monster)
                        {
                            auto monster = std::static_pointer_cast<Monster>(pair.second);
                            Vector3 pos = monster->Getpos_As_Vector3();
                            std::cout << "ID: " << monster->GetObjectId()
                                << " | Name: " << monster->GetName()
                                << " | Pos: (" << pos.x << ", " << pos.y << ", " << pos.z << ")"
                                << " | RL: " << (monster->IsRLControlled() ? "ON" : "OFF") << "\n";
                        }
                    }
                }
                std::cout << "======================================================================\n";
                p.set_value();
                });

            f.wait();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        });
    monitorThread.detach();

    // 엔터 대기 (모니터 종료용)
    std::string exitInput;
    std::getline(std::cin, exitInput);
    _showStatus = false;
    std::cout << "Monitor stopped.\n" << std::endl;
}
