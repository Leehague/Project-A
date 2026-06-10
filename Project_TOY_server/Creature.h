#pragma once
#include "GameObject.h"
#include "DataContents.h"
#include "InfoSturct.h"
#include <map>
#include <memory>


class Creature : public GameObject
{
public:
    Creature(int32 objectId, GameObjectType type) : GameObject(objectId, type) {}
    virtual ~Creature() {}

    // 상태 관련
    CreatureState GetState() { return _state; }
    void SetState(CreatureState state) { _state = state; }

    virtual void Init(int32 templateId)
    {
        GameObject::Init(templateId); //이걸 해야 공용 필드인 _templateId 가 초기화 됨.
        InitStatData(templateId);
    }
   
public:

    virtual void OnAttacked(int32 damage);


    float GetSpeed() { return _statInfo.speed; }
    const StatData* GetBaseStatData() { return _basestatData; }
    
    int32 GetAttack() { return _statInfo.attack; }

    virtual const std::string& GetName() { return  _statInfo.name; }

    int32 GetCurrentHp() { return _statInfo.hp; }
    int32 GetCurrentMp() { return _statInfo.mp; }

    int32 GetMaxHP() { return _statInfo.maxHp; }
    int32 GetMaxMP() { return _statInfo.maxMp; }

    bool UseMp(int32 requiredMp);

    uint64 GetlastMoveTick() {
        return _lastMoveTick;
    }

    void SetlastMoveTick(uint64 lastMoveTick)
    {
        _lastMoveTick = lastMoveTick;
    }

    int64 GetSkillCoolTime(int32 SkillID)
    {
        return _skillCooltimes[SkillID];
    }

    void SetSkillCoolTime(int32 SkillID,int64 LastUsedTick)
    {
        _skillCooltimes[SkillID] = LastUsedTick;
    }

protected:
    //Creature 사망 판정시 호출될 함수
    virtual void OnDead();

   
protected:

    Core::StatInfo _statInfo; //동적으로 변화할 정보

    //기본 스탯 정보. 참고만해야함. 변화하는 정보가 아님
    const StatData* _basestatData = nullptr; //templeteId 는 여기에 포함되어 있음

    uint64 _lastMoveTick = 0; // 마지막 이동 검증 시간 (ms)
    std::map<int32, int64> _skillCooltimes; // <SkillID, LastUsedTick>

protected:
    
    CreatureState _state = CreatureState::Idle;


protected:
    void InitStatData(int32 templateId);
};
