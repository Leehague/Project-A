#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "Room.h"
#include "GameObject.h"
#include "Session.h"
#include "DataContents.h"
#include "DataManager.h"

class Player : public GameObject
{
public:

    Player(int32 objectId, std::shared_ptr<Session> sessionPtr)
        : GameObject(objectId, GameObjectType::Player), session(sessionPtr)
    {
        
    }

    void Init(int32 templateId)
    {
        auto sessionPtr = session.lock();
        if (sessionPtr)
        {
            // 이제 shared_ptr로 관리되는 상태이므로 안전하게 호출 가능합니다.
            sessionPtr->SetPlayerPtr(std::static_pointer_cast<Player>(shared_from_this()));
            
        }
        InitStatData(templateId);
    }

    std::weak_ptr<Session> session; // 순환 참조 방지

public:
    float GetSpeed() { return _speed; }
private:
    //stat Data 

    int32 _templateId = 0; // 캐릭터 종류 고유 번호 
    int32 _maxHp = 0; // 기본 체력
    int32 _attack = 0; // 기본 공격력
    float _speed = 0; // 이동 속도 (고정값 혹은 기본값)
    
public:
    uint64 lastMoveTick = 0; // 마지막 이동 검증 시간 (ms)
private:
    void InitStatData(int32 templateId)
    {
        const StatData* statData = DataManager::GetInstance().GetStat(templateId);
        if (statData) {
            _speed = statData->speed;
            _maxHp = statData->baseHp;
            _attack = statData->baseAttack;
            _templateId = templateId;
            // 나중에 DB가 붙으면 여기서 _level = db.GetLevel() 등을 호출하면 됩니다.
        }
    }
};