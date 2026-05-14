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
private:
    // 추가: 행동 결정 주기 관리
    uint64 _nextDecisionTick = 0;
    const uint32 DECISION_INTERVAL_MS = 100; // 0.1초 = 100ms 마다 판단

    

public:
    std::vector<float> GatherContext();
    void ExecuteHighLevelAction(int actionId, Vector3 targetPos);
    void MoveTo(Vector3 targetPos);

public:
    void JobUpdate();
    void UpdateAction();
public:
    float _targetrange = 100.f; //TODO: DB 연동
    std::vector<Vector3> _path;

  
private:
    void ProcessMove();  // 이동 중일 때 좌표 갱신 처리
    void SyncPosAndBroadcast(Vector3 oldPos, Vector3 newPos); // 헬퍼 함수: 그리드 갱신과 브로드캐스트를 묶어서 처리
    void UseSkill(GameObjectPtr targetobj, Vector3 targetPos, int32 skillid);
};