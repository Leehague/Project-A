#pragma once
#include "Types.h"



class MapManager
{
public:
    void Init();
    MapPtr LoadMap(int mapId); // DataManager를 통해 경로를 얻어와 Map 객체 생성
    MapPtr GetMap(int mapId);

private:
    std::unordered_map<int, MapPtr> _maps;
};

extern MapManager GMapManager;