#pragma once
#include "Types.h"
#include "GameObject.h"


class ObjcetManager 
{
public:
	//생성자 호출, 초기화(Init 호출) , _objects에 추가
	GameObjectPtr Create(GameObjectType type, std::shared_ptr<Session> session, int32 templateId);
	
	GameObjectPtr Find(int32 objectId);
	void Removeobjcet(int32 objectId);
	int32 GetobjcetCounter();
	
private:
	std::mutex _lock;
	std::map<int32, GameObjectPtr> _objects; //이 딕셔너리에서의 key 값이 곧 objectId 임 _pos.objectId 와 같아야함
	int32 GameobjcetCounter = 0;
};


extern ObjcetManager GObjcetManager;