#include "GameSessionManager.h"
#include "Session.h"

GameSessionManager GSessionManager;

void GameSessionManager::Add(Session* session)
{
    std::lock_guard<std::mutex> lock(_lock);
    _sessions.insert(session);
}

void GameSessionManager::Remove(Session* session)
{
    std::lock_guard<std::mutex> lock(_lock);
    _sessions.erase(session);
}

void GameSessionManager::Broadcast(SendBufferRef sendBuffer)
{
    std::lock_guard<std::mutex> lock(_lock);
    for (Session* session : _sessions)
    {
        
        session->Send(sendBuffer);
    }
    
}