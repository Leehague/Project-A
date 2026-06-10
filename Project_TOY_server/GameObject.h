#pragma once
#include <memory>
#include <string>
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "Vector3.h"
#include "DataContents.h"
#include "InfoSturct.h"


struct SkillRecord {
	int64 lastUseTime = 0; // 밀리초(ms) 단위
};


enum class GameObjectType 
{
	None,
	Player,
	Monster,
	Projectile,
    Item,
};


class Vector3;

class GameObject : public std::enable_shared_from_this<GameObject>
{
public:
	
	
	
	GameObject(int32 objectId ) : _objectId(objectId), _roomId(0), _type(GameObjectType::None){ _pos.object_id=static_cast<uint64_t>(objectId); }
	GameObject(int32 objectId,  GameObjectType type) : _objectId(objectId), _roomId(0), _type(type) { _pos.object_id=static_cast<uint64_t>(objectId); }
	

	
	int32 GetObjectId() { return _objectId; }



    void Setpos(Protocol::PosInfo pos);

    void Setpos(Vector3 vecpos);

    void Setpos(Core::PosInfo pos);

	void Set_x(float x) { _pos.x=x; }; void Set_y(float y) { _pos.y=y; }; void Set_z(float z) { _pos.z=z; };
	const Core::PosInfo* Getpos() { return &_pos; }
    Vector3 Getpos_As_Vector3();

	CreatureState GetState() { return static_cast<CreatureState>(_pos.state); }
	void SetState(CreatureState state) { _pos.state= state; }
	void SetroomId(int32 roomid) { _roomId = roomid; }
	int32 GetroomId() { return _roomId; }

    RoomPtr Getroomptr();

	GameObjectType GetType() { return _type; }

public:
	virtual void Init(int32 templateId)
	{
		_templateId = templateId;
	}

    
public:
	int32 GetTemplateId() { return _templateId; }

protected:

	int32 _objectId = 0;
	int32 _roomId = 0; //소속된 Room의 roomId, initialized to 0
	int32 _templateId = 0; // 모든 오브젝트 공통 데이터 ID

	Core::PosInfo _pos; //여기에 objectId도 있음
	GameObjectType _type;


public:
    float CalculateYaw(Vector3 dir);
	
};
