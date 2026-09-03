#include "RoomManager.h"
#include "Room.h"

RoomManager GRoomManager;

int32 RoomManager::Create(int32 mapId)
{
    std::lock_guard<std::mutex> lock(_lock);  
    int32 roomId = ++RoomCounter;
    RoomPtr room = std::make_shared<Room>(roomId,mapId);
    room->Init();
    // ... 초기화 로직 ...
    _rooms[roomId] = room;

    _mapid_mapping_rooms[mapId].push_back(room);


    return roomId;
}

RoomPtr RoomManager::FindRoom(int32 roomId)
{
    std::lock_guard<std::mutex> lock(_lock);
	return _rooms[roomId];
}

RoomPtr RoomManager::FindRoom(int32 mapid, int numberOfSameMapidrooms)
{
    std::lock_guard<std::mutex> lock(_lock);
    
    auto it = _mapid_mapping_rooms.find(mapid);
    if (it != _mapid_mapping_rooms.end()) {
        std::vector<RoomPtr> roomvec = it->second;

        return roomvec[numberOfSameMapidrooms];
    }
    
    return nullptr;
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
