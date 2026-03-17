#include "RoomManager.h"

RoomManager GRoomManager;

int32 RoomManager::Create()
{
    std::lock_guard<std::mutex> lock(_lock);  
    int32 roomId = ++RoomCounter;
    RoomPtr room = std::make_shared<Room>();
    // ... 초기화 로직 ...
    _rooms[roomId] = room;
    room->SetRoomid(roomId);
    return roomId;
}

RoomPtr RoomManager::FindRoom(int32 roomId)
{
	return _rooms[roomId];
}

RoomPtr RoomManager::FindLastRoom()
{
    return FindRoom(RoomCounter);
}

void RoomManager::RemoveRoom(int32 roomId)
{
    _rooms.erase(roomId);
}

int32 RoomManager::GetRoomCounter()
{
    return RoomCounter;
}
