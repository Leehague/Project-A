#pragma once
#include "GameObject.h"
#include "Types.h"

class Projectile : public GameObject
{
public:
    Projectile(int32 objectId);
   
    virtual ~Projectile();

    void Init(GameObjectPtr attacker, const SkillData* skillData, Vector3 targetPos);
    void TickMove(); // 시간 경과에 따른 위치 갱신

    GameObjectPtr GetAttacker() { return _attacker.lock(); }
    const SkillData* GetSkillData() { return _skillData; }
    float GetTraveledDistance() const { return _traveledDistance; }

private:
    std::weak_ptr<GameObject> _attacker; // 강한 참조를 약한 참조로 변경
    const SkillData* _skillData;

    Vector3 _startPos;
    Vector3 _direction;
    float _speed = 20.0f; // 임시 속도 (추후 SkillData json에 추가 권장)
    float _traveledDistance = 0.0f;
    uint64 _lastTick = 0;
};
