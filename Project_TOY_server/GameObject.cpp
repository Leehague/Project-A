#include "GameObject.h"
#include "Room.h"
#include "DataContents.h"
#include "DataManager.h"
#include "RoomManager.h"

Vector3  GameObject::Getpos_As_Vector3() { return Vector3::PosInfoToVector3(&_pos); }

RoomPtr GameObject::Getroomptr() { return GRoomManager.FindRoom(_roomId); }

void GameObject::Setpos(Protocol::PosInfo pos)
{
    _pos.set_x(pos.x());
    _pos.set_y(pos.y());
    _pos.set_z(pos.z());
    _pos.set_yaw(pos.yaw());
    _pos.set_state(pos.state());

}

void GameObject::Setpos(Vector3 vecpos)
{
    Vector3 dir(vecpos.x - _pos.x(), vecpos.y - _pos.y(), vecpos.z - _pos.z());
    _pos.set_x(vecpos.x);
    _pos.set_y(vecpos.y);
    _pos.set_z(vecpos.z);
    _pos.set_yaw(CalculateYaw(dir));
}

void GameObject::InitStatData(int32 templateId)
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

void GameObject::OnAttacked(int32 damage)
{
    CurrentHp = CurrentHp - damage;

    if (CurrentHp <= 0)
    {
        CurrentHp = 0;
        std::cout << _name << " is dead" << std::endl;
        OnDead();
    }

}

bool GameObject::UseMp(int32 requiredMp)
{
    if (CurrentMp - requiredMp < 0) { return false; }
    CurrentMp = CurrentMp - requiredMp;
    return true;
}
void GameObject::OnDead()
{
    //사망 상태 방송 패킷 전송은 Room의 Execute 에서 담당

    //Dead 로 Creature state 수정
    SetState(CreatureState::OnDead);

}


float GameObject::CalculateYaw(Vector3 dir)
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
