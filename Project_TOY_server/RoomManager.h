#pragma once
#include "Types.h"


class RoomManager 
{
public:
	int32 Create(int32 mapId);
	RoomPtr FindRoom(int32 roomId);
    RoomPtr FindRoom(int32 mapid , int numberOfSameMapidrooms);
	RoomPtr FindLastRoom();
	void RemoveRoom(int32 roomId);
	int32 GetRoomCounter();
	std::vector<RoomPtr> GetRooms(); // 모든 방 목록 가져오기 추가
private:
	std::mutex _lock;
	std::map<int32, RoomPtr> _rooms;
    std::map<int32, std::vector<RoomPtr>> _mapid_mapping_rooms;


	int32 RoomCounter = 0;
};

extern RoomManager GRoomManager;
