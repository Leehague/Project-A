#pragma once
#include <string>
#include <vector>
#include "Types.h"
#include "Vector3.h"


struct PQNode {
    bool operator<(const PQNode& other) const { return f > other.f; }
    int32 f; // g + h
    int32 g; // 시작점부터의 비용
    Vector3 pos;
};


struct MapData;
class Map
{
public:
    bool Load(const MapData* mapdata);
    bool LoadNavMesh(const std::string& path);
    
    bool CanGo(Vector3 pos , float Ypadding = 0.5f); // 핵심 로직
    bool CheckProjectileCollision(Vector3 pos); // 투사체 지형 충돌 검사
    float GetHeight(Vector3 pos); // [추가] 특정 좌표의 지형 높이 반환
    uint64 GetMapId();

    const MapData* GetMapData() {return _mapdata;}
    std::pair<int, int> GetGridPos(Vector3 pos);
private:

    int _width;
    int _height;
    float _minX, _minZ;
    float _cellSize;

    const MapData* _mapdata =nullptr;
    std::vector<std::vector<bool>> _collisionData;
    std::vector<std::vector<float>> _heightData; // 높이 데이터 저장용


    std::vector<Triangle> _navTriangles;
    // 각 그리드 좌표(z, x)에 속한 삼각형들의 인덱스 리스트
    std::vector<std::vector<std::vector<int>>> _gridIndices;


    

public:
    // 시작점에서 목적지까지의 경로를 Vector3 리스트로 반환
    std::vector<Vector3> FindPath(Vector3 startPos, Vector3 endPos);

 
};
