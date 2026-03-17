#pragma once
#include <memory>
enum class GameObjectType 
{
	None,
	Player,
	Monster,
};

class GameObject : public std::enable_shared_from_this<GameObject>
{
public:
	
	
	
	GameObject(int32 objectId ) : _objectId(objectId), _roomId(0), _type(GameObjectType::None){ _pos.set_object_id(static_cast<uint64_t>(objectId)); }
	GameObject(int32 objectId,  GameObjectType type) : _objectId(objectId), _roomId(0), _type(type) { _pos.set_object_id(static_cast<uint64_t>(objectId)); }

	
	int32 GetObjectId() { return _objectId; }

	void Setpos(Protocol::PosInfo pos) { _pos = pos; }
	Protocol::PosInfo Getpos() { return _pos; }

	void SetroomId(int32 roomid) { _roomId = roomid; }
	int32 GetroomId() { return _roomId; }

	GameObjectType GetType() { return _type; }
protected:

	int32 _objectId = 0;
	int32 _roomId = 0; //소속된 Room의 roomId, initialized to 0

	Protocol::PosInfo _pos; //여기에 objectId도 있음
	GameObjectType _type;
};
