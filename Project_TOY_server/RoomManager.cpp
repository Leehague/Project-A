#include "RoomManager.h"

RoomManager GRoomManager;

int32 RoomManager::Create(int32 mapId)
{
    std::lock_guard<std::mutex> lock(_lock);  
    int32 roomId = ++RoomCounter;
    RoomPtr room = std::make_shared<Room>(roomId,mapId);
    room->Init();
    // ... 초기화 로직 ...
    _rooms[roomId] = room;
    //room->SetRoomid(roomId); [수정] Room 클래스의 생성자에서 수행함
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

std::vector<RoomPtr> RoomManager::GetRooms()
{
    std::lock_guard<std::mutex> lock(_lock);
    std::vector<RoomPtr> rooms;

    for (auto& pair : _rooms)
    {
        rooms.push_back(pair.second);
    }

    return rooms;
}
