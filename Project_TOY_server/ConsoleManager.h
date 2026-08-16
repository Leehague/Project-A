#pragma once
#include "Types.h"
#include <string>
#include <vector>
#include <thread>
#include <atomic>

class ConsoleManager
{
public:
    // 싱글톤 인스턴스 획득
    static ConsoleManager& GetInstance()
    {
        static ConsoleManager instance;
        return instance;
    }

    // 콘솔 입력 감시 스레드를 시작합니다.
    void StartConsoleThread();

    // 콘솔 스레드를 정지시킵니다.
    void StopConsoleThread();

private:
    ConsoleManager() : _isRunning(false), _showStatus(false) {}
    ~ConsoleManager();

    // 백그라운드 콘솔 입력 대기 루프
    void ConsoleLoop();

    // 문자열 명령어를 분석하여 하위 메소드로 분기
    void ProcessCommand(const std::string& fullCommand);

    // 명령어별 세부 처리 메소드 (핸들러)
    void CmdCreateRoom(const std::vector<std::string>& args);
    void CmdListRooms();
    void CmdSpawn(const std::vector<std::string>& args);
    void CmdRLSpawn(const std::vector<std::string>& args);
    void CmdHusuabiSpawn(const std::vector<std::string>& args);
    void CmdMonitor(const std::vector<std::string>& args);
    void CmdQuestCreate(const std::vector<std::string>& args);
    void CmdListQuests(const std::vector<std::string>& args);
    
private:
    std::thread _consoleThread;
    std::atomic<bool> _isRunning;
    std::atomic<bool> _showStatus; // 실시간 모니터링 출력 활성화 상태
};
