#include "Creature.h"
#include "Room.h"
#include "DataContents.h"
#include "DataManager.h"
#include "RoomManager.h"

void Creature::InitStatData(int32 templateId)
{
    const StatData* statData = DataManager::GetInstance().GetStat(templateId);
    if (statData) {
        _statInfo.speed = statData->speed;
        _statInfo.maxHp = statData->baseHp;
        _statInfo.maxMp = statData->baseMp;
        _statInfo.attack = statData->baseAttack;
        _statInfo.name = statData->name;
        _basestatData = statData;

        _statInfo.hp = _statInfo.maxHp;
        _statInfo.mp = _statInfo.maxMp;
        // 나중에 DB가 붙으면 여기서 _level = db.GetLevel() 등을 호출하면 됩니다.
        // + something 을 여기서 해주면 됨
        //즉 스텟 초기화

    }
}

void Creature::OnAttacked(int32 damage)
{
    _statInfo.hp = _statInfo.hp - damage;

    if (_statInfo.hp <= 0)
    {
        _statInfo.hp = 0;
        std::cout << _statInfo.name << " is dead" << std::endl;
        OnDead();
    }

}

bool Creature::UseMp(int32 requiredMp)
{
    if (_statInfo.mp - requiredMp < 0) { return false; }
    _statInfo.mp = _statInfo.mp - requiredMp;
    return true;
}

void Creature::OnDead()
{
    //사망 상태 방송 패킷 전송은 Room의 Execute 에서 담당

    //Dead 로 Creature state 수정
    SetState(CreatureState::OnDead);

}
