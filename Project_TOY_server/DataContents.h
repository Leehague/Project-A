#pragma once
#include <map>
#include <vector>
#include "Types.h" // float, int32 등 정의된 헤더
#include <string>

struct StatData {
    int32 templateId; // 캐릭터 종류 고유 번호 , 클라에서 어떤 프리팹(특히 아트리소스)를 가져와야하는지 판별하는데 쓰임
    int32 baseHp;     // 기본 체력
    int32 baseMp;     // 기본 마나  
    int32 baseAttack; // 기본 공격력
    float speed;      // 이동 속도 (고정값 혹은 기본값)
    std::string name;
    
};

struct MapData 
{
    uint64 MapId;
    std::string mapName;
    std::string MapPath;
    std::string NavMeshPath;
    int64 MinX;
    int64 MinZ;
    float CellSize;
    uint64 width;
    uint64 height;
};

enum class SkillType {
    Common = 0,     // 공통타입, 가장 기본타입
    Melee = 1,      // 근접
    Projectile = 2, // 투사체
    Dash = 3        // 이동기
};

enum class SkillTargetType 
{
    objectTarget =0, // 타겟팅, 오브젝트 타겟
    positionTarget =1 // 지역 목표 , 특정 위치가 타겟
};

enum class CostType 
{
    None = 0,
    Mana = 1,
    Hp = 2
};

struct SkillData {
    int32 id;
    std::string name;
    SkillType skillType;
    CostType costType;
    int32 damage;
    float range;
    float coolTime;
    std::string animName;
    int32 cost;

    //Projectile
    float projectileSpeed;
    int32 projectileId;
    
    //Dash
    float dashDistance;
    float dashSpeed;

    //targetType
    SkillTargetType targetType;
};