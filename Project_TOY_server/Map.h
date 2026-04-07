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
    bool CanGo(Vector3 pos); // ÇÙ½É ·ÎÁ÷


private:
    int _width;
    int _height;
    float _minX, _minZ;
    float _cellSize;

    const MapData* _mapdata;
    std::vector<std::vector<bool>> _collisionData;
};

