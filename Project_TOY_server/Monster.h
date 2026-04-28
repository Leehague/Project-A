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

    // 추가: 행동 결정 주기 관리
    uint64 _nextDecisionTick = 0;
    const uint32 DECISION_INTERVAL_MS = 500; // 0.5초마다 판단
public:
    std::vector<float> GatherContext();
    void ExecuteHighLevelAction(int actionId, Vector3 targetPos);
    void MoveTo(Vector3 targetPos);

public:
    void UpdateAction();
private:
    float _targetrange = 100.f; //TODO: DB 연동
    std::vector<Vector3> _path;

    void LogicUpdate();  // 결정 주기마다 호출 (판단 로직)
    void ProcessMove();  // 이동 중일 때 좌표 갱신 처리
};