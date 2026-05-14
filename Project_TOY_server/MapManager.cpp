#include "MapManager.h"
#include "DataManager.h"
#include "Map.h"

MapManager GMapManager;

void MapManager::Init()
{
    //
}

MapPtr MapManager::LoadMap(int mapId)
{
    // 이미 로드된 맵인지 확인
    if (_maps.find(mapId) != _maps.end())
        return _maps[mapId];

    // DataManager에서 경로 가져오기
    const MapData* data = DataManager::GetInstance().GetMapData(mapId);
    if (data == nullptr) return nullptr;

    // 새로운 Map 객체 생성 및 로드
    MapPtr map = std::make_shared<Map>();
    if (map->Load(data) && map->LoadNavMesh(data->NavMeshPath))
    {
        _maps[mapId] = map;
        return map;
    }
    
    
    return nullptr;
}

MapPtr MapManager::GetMap(int mapId)
{
    auto it = _maps.find(mapId);
    if (it != _maps.end())
        return it->second;

    return nullptr;
}
