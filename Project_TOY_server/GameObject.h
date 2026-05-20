#pragma once
#include <memory>
#include <string>
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "Room.h"
#include "DataContents.h"
#include "DataManager.h"
#include "Vector3.h"
#include "RoomManager.h"

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

enum class CreatureState
{
	Idle,
	Moving,
	Skill,
	OnDead, //방금 사망
	Dead, //죽어있었던 상태
};



class GameObject : public std::enable_shared_from_this<GameObject>
{
public:
	
	
	
	GameObject(int32 objectId ) : _objectId(objectId), _roomId(0), _type(GameObjectType::None){ _pos.set_object_id(static_cast<uint64_t>(objectId)); }
	GameObject(int32 objectId,  GameObjectType type) : _objectId(objectId), _roomId(0), _type(type) { _pos.set_object_id(static_cast<uint64_t>(objectId)); }
	

	
	int32 GetObjectId() { return _objectId; }



	void Setpos(Protocol::PosInfo pos) 
	{
		_pos.set_x(pos.x());
		_pos.set_y(pos.y());
		_pos.set_z(pos.z());
		_pos.set_yaw(pos.yaw());
		_pos.set_state(pos.state());
		
	}

      void Setpos(Vector3 vecpos) 
    {
        Vector3 dir(vecpos.x - _pos.x(), vecpos.y - _pos.y(), vecpos.z - _pos.z());
        _pos.set_x(vecpos.x);
        _pos.set_y(vecpos.y);
        _pos.set_z(vecpos.z);
		_pos.set_yaw(CalculateYaw(dir));
    }
	void Set_x(float x) { _pos.set_x(x); }; void Set_y(float y) { _pos.set_y(y); }; void Set_z(float z) { _pos.set_z(z); };
	const Protocol::PosInfo* Getpos() { return &_pos; }
	Vector3 Getpos_As_Vector3() { return Vector3::PosInfoToVector3(&_pos); }

	CreatureState GetState() { return static_cast<CreatureState>(_pos.state()); }
	void SetState(CreatureState state) { _pos.set_state(static_cast<int32>(state)); }
	void SetroomId(int32 roomid) { _roomId = roomid; }
	int32 GetroomId() { return _roomId; }

	RoomPtr Getroomptr() { return GRoomManager.FindRoom(_roomId); }

	GameObjectType GetType() { return _type; }
public:
	virtual void Init(int32 templateId)
	{
		InitStatData(templateId);
	}
	
protected:
	void InitStatData(int32 templateId)
	{
		const StatData* statData = DataManager::GetInstance().GetStat(templateId);
		if (statData) {
			_speed = statData->speed;
			_maxHp = statData->baseHp;
			_maxMp = statData->baseMp;
			_attack = statData->baseAttack;
			_name = statData->name;
			_basestatData = statData;

			CurrentHp = _maxHp;
			CurrentMp = _maxMp;
			// 나중에 DB가 붙으면 여기서 _level = db.GetLevel() 등을 호출하면 됩니다.
			// + something 을 여기서 해주면 됨
			//즉 스텟 초기화

		}
	}
public:
	void OnAttacked(int32 damage)
	{
		CurrentHp = CurrentHp - damage;

		if (CurrentHp <= 0)
		{
			CurrentHp = 0;
			std::cout << _name << " is dead" << std::endl; 
			OnDead();
		}
		
	}

	

	float GetSpeed() { return _speed; }
	const StatData* GetBaseStatData() { return _basestatData; }
	int32 GetTempleteId() {
		return _basestatData->templateId;
	}
	int32 GetAttack() { return _attack; }
	const std::string& GetName() { return  _name; }
	int32 GetCurrentHp() { return CurrentHp; }
	int32 GetCurrentMp() { return CurrentMp; }

	int32 GetMaxHP() { return _maxHp; }
	int32 GetMaxMP() { return _maxMp; }

	bool UseMp(int32 requiredMp) 
	{
		if (CurrentMp - requiredMp < 0) { return false; }
		CurrentMp = CurrentMp - requiredMp;
		return true;
	}

protected:
	//GameObject 사망 판정시 호출될 함수
	virtual void OnDead() 
	{
		//사망 상태 방송 패킷 전송은 Room의 Execute 에서 담당

		//Dead 로 Creature state 수정
		SetState(CreatureState::OnDead);

	}
protected:

	int32 _objectId = 0;
	int32 _roomId = 0; //소속된 Room의 roomId, initialized to 0

	Protocol::PosInfo _pos; //여기에 objectId도 있음
	GameObjectType _type;


	//stat Data , 게임 진행에 따라 변화 할수 있음. UI는 이 정보를 바탕으로 그려져야 하고 판정역시 마찬가지임
	int32 _maxHp = 0; //최대 체력 = 기본 최대체력 + something
	int32 _maxMp = 0; //최대 마나 = 기본 최대마나 + something
	int32 _attack = 0; // 공격력 = 기본 공격력(baseAttack) + something
	float _speed = 1; // 이동 속도 (고정값 혹은 기본값)
	std::string _name ="NOPE";
	int32 CurrentHp = 0; //현재 체력
	int32 CurrentMp = 0; // 현재 마나

	//기본 스탯 정보. 참고만해야함. 변화하는 정보가 아님
	const StatData* _basestatData = nullptr; //templeteId 는 여기에 포함되어 있음

public:
	uint64 lastMoveTick = 0; // 마지막 이동 검증 시간 (ms)
	std::map<int32, int64> _skillCooltimes; // <SkillID, LastUsedTick>

public:
	float CalculateYaw(Vector3 dir)
	{
		// 이동 거리가 거의 없으면 각도를 변경하지 않음 (0으로 나누기 방지)
		if (std::abs(dir.x) < EPSILON && std::abs(dir.z) < EPSILON)
			return 0.0f; // 혹은 기존 yaw 유지

		// atan2는 라디안 값을 반환 (-PI ~ PI)
		float radian = std::atan2(dir.x, dir.z);

		// 라디안 -> 도 변환
		float degree = radian * (180.0f / 3.1415926535f);

		// 결과를 0~360 범위로 정규화 (선택 사항)
		if (degree < 0) degree += 360.0f;

		if (!std::isfinite(degree))
			return 0.0f;

		return degree;
	}
};
