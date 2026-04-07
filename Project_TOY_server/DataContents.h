#pragma once
#include <map>
#include <vector>
#include "Types.h" // float, int32 등 정의된 헤더
#include <string>

struct StatData {
    int32 templateId; // 캐릭터 종류 고유 번호 
    int32 baseHp;     // 기본 체력
    int32 baseAttack; // 기본 공격력
    float speed;      // 이동 속도 (고정값 혹은 기본값)
    
};

struct MapData 
{
    uint64 MapId;
    std::string mapName;
    std::string MapPath;
    int64 MinX;
    int64 MinZ;
    float CellSize;
    uint64 width;
    uint64 height;
};

// 나중에 ItemData, SkillData 등 추가 가능