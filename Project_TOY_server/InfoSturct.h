#pragma once
#include "Types.h"

enum class CreatureState
{
    Idle,
    Moving,
    Skill,
    OnDead, //방금 사망
    Dead, //죽어있었던 상태
};



namespace Core {
    struct PosInfo {
        // 기본 생성자: 모든 멤버를 0으로 초기화하여 쓰레기 값 문제를 원천 방지합니다.
        PosInfo() : object_id(0), x(0), y(0), z(0), yaw(0), state(CreatureState::Idle) {}

        int32 object_id = 0; // 이동하는 대상의 ID
        float x = 0;
        float y = 0;
        float z = 0;
        float yaw = 0;       // 바라보는 방향 (회전)
        CreatureState state = CreatureState::Idle;
    };


    struct StatInfo
    {
        int32 hp = 0;
        int32 maxHp = 0;
        int32 mp = 0;
        int32 maxMp = 0;
        int32 attack = 0;
        float speed = 1.0f;
        std::string name;

    };

    struct SpawnInfo
    {
        PosInfo spawnposinfo;
        int32 templateId;
    };

    struct TargetPosInfo {
        float x;
        float y;
        float z;
    };


    struct ItemInfo
    {
        int32 itemDbId;
        int32 itemTemplateId;
        int32 count;
        int32 slot;
        std::string itemMemo = "";
    };


    // CoreRoom에서 계산된 피격 결과를 Room으로 전달하기 위한 구조체
    struct DamageResult {
        GameObjectPtr target;
        int32 damage;
    };

}

