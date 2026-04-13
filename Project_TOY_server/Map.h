#pragma once
#include <string>
#include "Vector3.h"
#include <vector>
#include "Types.h"

struct MapData;
class Map
{
public:
    bool Load(const MapData* mapdata);
    bool CanGo(Vector3 pos); // 핵심 로직
    float GetHeight(Vector3 pos); // [추가] 특정 좌표의 지형 높이 반환

private:
    int _width;
    int _height;
    float _minX, _minZ;
    float _cellSize;

    const MapData* _mapdata;
    std::vector<std::vector<bool>> _collisionData;
    std::vector<std::vector<float>> _heightData; // [추가] 높이 데이터 저장용
};

