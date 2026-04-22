#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "Room.h"
#include "GameObject.h"
#include "Session.h"
#include "DataContents.h"
#include "DataManager.h"


class Monster : public GameObject
{
public:

    Monster(int32 objectId)
        : GameObject(objectId, GameObjectType::Monster) 
    {
        
    }

    virtual void Init(int32 templateId)
    {
        
        InitStatData(templateId);
        
    }


public:
    uint64 lastMoveTick = 0; // 마지막 이동 검증 시간 (ms)
    std::map<int32, int64> _skillCooltimes; // <SkillID, LastUsedTick>

};