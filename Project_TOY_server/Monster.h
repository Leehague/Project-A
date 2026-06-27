#pragma once
#include "Types.h"
#include "GameObject.h"
#include "Creature.h"
#include <functional>

using RLPredictCallback = std::function<int(const std::vector<float>&)>;

class Monster : public Creature
{
public:

    Monster(int32 objectId, CoreRoomPtr coreroomptr)
        : Creature(objectId, GameObjectType::Monster, coreroomptr), _isRLControlled(false), _rlPredictCallback(nullptr)
    {
        
    }

    void SetRLControlled(bool val) { _isRLControlled = val; }
    bool IsRLControlled() const { return _isRLControlled; }

    void SetRLPredictCallback(RLPredictCallback callback) { _rlPredictCallback = callback; }
    bool HasRLPredictCallback() const { return _rlPredictCallback != nullptr; }


    using SkillCallback = std::function<void(MonsterPtr, GameObjectPtr, Vector3, int32)>;
    SkillCallback _onUseSkillCallback = nullptr;

    void SetSkillCallback(SkillCallback callback) { _onUseSkillCallback = callback; }
private:
    bool _isRLControlled;
    RLPredictCallback _rlPredictCallback;
    // 추가: 행동 결정 주기 관리
    uint64 _nextDecisionTick = 0;
    const uint32 DECISION_INTERVAL_MS = 100; // 0.1초 = 100ms 마다 판단

    

public:
    std::vector<float> GatherContext();
    void ExecuteHighLevelAction(int actionId, Vector3 targetPos);
    void MoveTo(const Vector3& targetPos);
    void FleeFrom(const Vector3& targetPos);

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
