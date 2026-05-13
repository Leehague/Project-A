#include "Projectile.h"
#include "Vector3.h"
#include "Protocol/Protocol.pb.h"
#include "Room.h"
#include "Session.h"

Projectile::~Projectile() {}

void Projectile::Init(GameObjectPtr attacker, const SkillData* skillData, Vector3 targetPos)
{
    _attacker = attacker;
    _skillData = skillData;
    _startPos = Vector3::PosInfoToVector3(Getpos());

    // 타겟 방향 벡터 계산
    _direction = targetPos - _startPos;
    _direction.y = 0; // 지면과 평행하게 날아가도록 고정
    _direction.Normalize();

    _lastTick = ::GetTickCount64();
    _traveledDistance = 0.0f;
}

void Projectile::TickMove()
{
    uint64 currentTick = ::GetTickCount64();
    if (_lastTick == 0) _lastTick = currentTick;

    float deltaTime = (currentTick - _lastTick) / 1000.0f;
    _lastTick = currentTick;

    float moveDist = _speed * deltaTime;
    _traveledDistance += moveDist;

    // 현재 위치에서 방향으로 속도만큼 이동
    Vector3 currentPos = Vector3::PosInfoToVector3(Getpos());
    Vector3 newPos = currentPos + (_direction * moveDist);

    Protocol::PosInfo posInfo;
    posInfo.set_x(newPos.x);
    posInfo.set_y(newPos.y);
    posInfo.set_z(newPos.z);
    Setpos(posInfo);
}
