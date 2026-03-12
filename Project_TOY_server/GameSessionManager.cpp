#include "GameSessionManager.h"
#include "Session.h"

GameSessionManager GSessionManager;

void GameSessionManager::GlobalBroadcast(SendBufferPtr sendBuffer)
{
    std::lock_guard<std::mutex> lock(_lock);
    for (auto& pair: _sessions)
    {
        
        pair.second->Send(sendBuffer);
    }
    
}

void GameSessionManager::Add(SessionPtr session) {
    std::lock_guard<std::mutex> lock(_lock);
    // 세션의 GUID를 키로 사용 (세션에 GetGuid() 함수가 있다고 가정)
    _sessions[session->GetGuid()] = session;
}

void GameSessionManager::Remove(SessionPtr session) {
    std::lock_guard<std::mutex> lock(_lock);
    _sessions.erase(session->GetGuid());
}

void GameSessionManager::SendTo(uint64 playerSessionId, SendBufferPtr sendBuffer) {
    std::lock_guard<std::mutex> lock(_lock);

    // 맵에서 해당 ID를 찾음
    auto it = _sessions.find(playerSessionId);
    if (it != _sessions.end()) {
        it->second->Send(sendBuffer);
    }
}