#pragma once
#include "Types.h"
#include "GameObject.h"


class ObjcetManager 
{
public:
	GameObjectPtr Create(int32 RoomId,GameObjectType type, std::shared_ptr<Session> session);
	
	GameObjectPtr Find(int32 objectId);
	void Removeobjcet(int32 roomId);
	int32 GetobjcetCounter();
	
private:
	std::mutex _lock;
	std::map<int32, GameObjectPtr> _objects; //이 딕셔너리에서의 key 값이 곧 objectId 임 _pos.objectId 와 같아야함
	int32 GameobjcetCounter = 0;
};


extern ObjcetManager GObjcetManager;