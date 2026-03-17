#pragma once
#include "Types.h"
#include "Room.h"

class RoomManager 
{
public:
	int32 Create();
	RoomPtr FindRoom(int32 roomId);
	RoomPtr FindLastRoom();
	void RemoveRoom(int32 roomId);
	int32 GetRoomCounter();
	


private:
	std::mutex _lock;
	std::map<int32, RoomPtr> _rooms;
	int32 RoomCounter = 0;
};

extern RoomManager GRoomManager;