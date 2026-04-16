#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "Room.h"
#include "GameObject.h"
#include "Session.h"
#include "DataContents.h"
#include "DataManager.h"

struct SkillRecord {
    int64 lastUseTime = 0; // 밀리초(ms) 단위
};

class Player : public GameObject
{
public:

    Player(int32 objectId, std::shared_ptr<Session> sessionPtr)
        : GameObject(objectId, GameObjectType::Player), session(sessionPtr)
    {
        
    }

    

    std::weak_ptr<Session> session; // 순환 참조 방지

    virtual void Init(int32 templateId)
    {

        auto sessionPtr = session.lock();
        if (sessionPtr)
        {
            // 이제 shared_ptr로 관리되는 상태이므로 안전하게 호출 가능합니다.
            sessionPtr->SetPlayerPtr(std::static_pointer_cast<Player>(shared_from_this()));

        }
        InitStatData(templateId);
    }


public:
    uint64 lastMoveTick = 0; // 마지막 이동 검증 시간 (ms)
    std::map<int32, int64> _skillCooltimes; // <SkillID, LastUsedTick>

};