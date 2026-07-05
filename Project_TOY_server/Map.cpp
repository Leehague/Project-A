#include "Map.h"
#include "DataContents.h"
#include <fstream>
#include <iostream>
#include <queue>
#include "InfoSturct.h"
#include "Vector3.h"


bool Map::Load(const MapData* mapdata)
{
    
    std::string path = mapdata->MapPath;
    std::ifstream ifs(path);
    if (!ifs.is_open()) 
    { 
        std::cout << "맵 파일 열기 실패" << std::endl;
        return false;
    }
       

    // 1. 첫 번째 줄: 타일 개수 읽기
    if (!(ifs >> _width >> _height)) 
    {
        std::cout << "타일 개수 읽기 실패" << std::endl;
        return false;
    }

    // 2. [수정] 두 번째 줄: 기준 좌표 및 셀 크기 읽기
    if (!(ifs >> _minX >> _minZ >> _cellSize)) 
    {
        std::cout << "_minX  _minZ  _cellSize 읽기 실패" << std::endl;
        return false;
    }

    //[추가] DataManager에서 json파일을 통해 가져온 mapdata와 Minx등이 일치하는지 확인
    if (mapdata->MinX != _minX || mapdata->MinZ != _minZ || mapdata->CellSize != _cellSize)
    {
        std::cout << "MapData mismatch " << std::endl;

        std::cout << "mapdata->MinX :"<< mapdata->MinX<<"_minX: "<< _minX << std::endl;

        std::cout << "mapdata->MinZ: "<< mapdata->MinZ<<"_minZ: "<< _minZ << std::endl;

        std::cout << "mapdata->CellSize: "<< mapdata->CellSize<<"_cellSize: "<< _cellSize << std::endl;
        return false;
    }
    //[추가] mapdata 포인터를 Map에서 기억
    _mapdata = mapdata;

    // 3. 데이터 공간 확보
    _collisionData.assign(_height, std::vector<bool>(_width, false));
    _heightData.assign(_height, std::vector<float>(_width, 0.0f)); // 공간 확보

    // 4. 맵 데이터 파싱
    for (int z = 0; z < _height; z++)
    {
        for (int x = 0; x < _width; x++)
        {
            std::string cellInfo;
            if (ifs >> cellInfo)
            {
                // "0|1.5" 형태의 문자열 파싱
                size_t pos = cellInfo.find('|');
                if (pos != std::string::npos)
                {
                    int collision = std::stoi(cellInfo.substr(0, pos));
                    float height = std::stof(cellInfo.substr(pos + 1));

                    _collisionData[z][x] = (collision == 1);
                    _heightData[z][x] = height;
                }
            }
        }
    }

    std::cout << "Map Loaded: " << path << " | Min(" << _minX << "," << _minZ << ") Cell: " << _cellSize << std::endl;
    return true;
}


float Map::GetHeight(Vector3 pos)
{
    int x = static_cast<int>((pos.x - _minX) / _cellSize);
    int z = static_cast<int>((pos.z - _minZ) / _cellSize);

    if (x < 0 || x >= _width || z < 0 || z >= _height)
        return -100.0f;

    return _heightData[z][x];
}
uint64 Map::GetMapId()
{
    return _mapdata->MapId;
}
std::pair<int, int> Map::GetGridPos(Vector3 pos)
{
    // 1. 기준 좌표(_minX, _minZ)로부터의 거리를 셀 크기로 나눕니다.
    int x = static_cast<int>((pos.x - _minX) / _cellSize);
    int z = static_cast<int>((pos.z - _minZ) / _cellSize);

    // 2. 맵의 범위를 벗어나지 않도록 보정(Clamp)해줍니다.
    // _width와 _height는 맵 로드 시 결정된 타일의 개수입니다.
    x = std::max(0, std::min(x, _width - 1));
    z = std::max(0, std::min(z, _height - 1));

    return { x, z };
}

std::vector<Vector3> Map::FindPath(Vector3 startPos, Vector3 endPos)
{
    std::vector<Vector3> path;

    // 1. 갈 수 없는 목적지라면 바로 리턴
    if (!CanGo(endPos)) return path;

    // 2. 우선순위 큐 및 방문 기록용 데이터 구조
    std::priority_queue<PQNode> pq;
    // key: 그리드 좌표(pair<int, int>), value: 해당 지점까지의 최소 비용 g
    std::map<std::pair<int, int>, int32> bestG;
    // 부모 노드 기록 (경로 역추적용)
    std::map<std::pair<int, int>, std::pair<int, int>> parent;

    auto startIdx = GetGridPos(startPos);
    auto endIdx = GetGridPos(endPos);

    if (startIdx == endIdx) return { endPos };

    pq.push({ 0, 0, startPos });
    bestG[startIdx] = 0;

    while (!pq.empty())
    {
        PQNode node = pq.top();
        pq.pop();

        std::pair<int, int> nowIdx = GetGridPos(node.pos);

        // 목적지 도착 확인
        if (nowIdx == endIdx)
        {
            // 경로 역추적하여 path 벡터 채우기
            std::pair<int, int> curr = endIdx;
            while (curr != startIdx)
            {
                path.push_back(Vector3(
                    curr.first * _cellSize + _minX,
                    GetHeight(Vector3(curr.first * _cellSize + _minX, 0, curr.second * _cellSize + _minZ)),
                    curr.second * _cellSize + _minZ
                ));
                curr = parent[curr];
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        // 8방향 탐색
        static int dx[] = { 1, -1, 0, 0, 1, 1, -1, -1 };
        static int dz[] = { 0, 0, 1, -1, 1, -1, 1, -1 };

        for (int i = 0; i < 8; i++)
        {
            int nextX = nowIdx.first + dx[i];
            int nextZ = nowIdx.second + dz[i];
            std::pair<int, int> nextIdx = { nextX, nextZ };

            Vector3 nextPos(nextX * _cellSize + _minX, 0, nextZ * _cellSize + _minZ);
            nextPos.y = GetHeight(nextPos);

            if (!CanGo(nextPos)) continue;

            // 가로/세로는 10, 대각선은 14 (정수 연산 최적화)
            int32 moveCost = (i < 4) ? 10 : 14;
            int32 nextG = node.g + moveCost;

            if (bestG.find(nextIdx) == bestG.end() || nextG < bestG[nextIdx])
            {
                bestG[nextIdx] = nextG;
                // Heuristic: 맨해튼 거리 
                int32 h = static_cast<int32>(std::abs(nextX - endIdx.first) + std::abs(nextZ - endIdx.second)) * 10;
                pq.push({ nextG + h, nextG, nextPos });
                parent[nextIdx] = nowIdx;
            }
        }
    }

    return path;
}

bool Map::LoadNavMesh(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        std::cout << "파일 열기 실패: " << path << std::endl;
        return false;
    }

    // 1. 정점 개수 읽기
    int32 vertCount = 0;
    ifs.read(reinterpret_cast<char*>(&vertCount), sizeof(int32));



    if (vertCount <= 0) {
        std::cout << "Vertex Count가 0입니다." << std::endl;
        return false;
    }



    // 2. 정점 목록 읽기 (하나씩 읽어서 Vector3에 넣기)
    std::vector<Vector3> vertices(vertCount);
    for (int i = 0; i < vertCount; i++) {
        // 유니티 float 3개를 순서대로 읽음
        ifs.read((char*)&vertices[i].x, sizeof(float));
        ifs.read((char*)&vertices[i].y, sizeof(float));
        ifs.read((char*)&vertices[i].z, sizeof(float));
    }

    // 3. 인덱스 개수 읽기
    int32 indexCount = 0;
    ifs.read((char*)&indexCount, sizeof(int32));

    // 4. 그리드 공간 확보 (중요: Load()에서 읽은 _width, _height 기준)
    _gridIndices.assign(_height, std::vector<std::vector<int>>(_width));

    // 5. 삼각형 구성 및 그리드 등록
    _navTriangles.clear();
    for (int i = 0; i < indexCount; i += 3) {
        int32 i1, i2, i3;
        ifs.read((char*)&i1, sizeof(int32));
        ifs.read((char*)&i2, sizeof(int32));
        ifs.read((char*)&i3, sizeof(int32));

        // 안전한 인덱스 체크
        if (i1 < 0 || i1 >= vertCount || i2 < 0 || i2 >= vertCount || i3 < 0 || i3 >= vertCount)
            continue;

        Triangle tri;
        tri.v1 = vertices[i1]; tri.v2 = vertices[i2]; tri.v3 = vertices[i3];

        // AABB 계산
        tri.minX = std::min({ tri.v1.x, tri.v2.x, tri.v3.x });
        tri.maxX = std::max({ tri.v1.x, tri.v2.x, tri.v3.x });
        tri.minZ = std::min({ tri.v1.z, tri.v2.z, tri.v3.z });
        tri.maxZ = std::max({ tri.v1.z, tri.v2.z, tri.v3.z });

        int triIndex = static_cast<int>(_navTriangles.size());
        _navTriangles.push_back(tri);

        // --- [누락되었던 핵심 로직: 그리드 등록] ---
        int startX = static_cast<int>((tri.minX - _minX) / _cellSize);
        int endX = static_cast<int>((tri.maxX - _minX) / _cellSize);
        int startZ = static_cast<int>((tri.minZ - _minZ) / _cellSize);
        int endZ = static_cast<int>((tri.maxZ - _minZ) / _cellSize);

        // 그리드 범위 클램핑 (런타임 에러 방지)
        startX = std::max(0, std::min(startX, _width - 1));
        endX = std::max(0, std::min(endX, _width - 1));
        startZ = std::max(0, std::min(startZ, _height - 1));
        endZ = std::max(0, std::min(endZ, _height - 1));

        for (int z = startZ; z <= endZ; z++) {
            for (int x = startX; x <= endX; x++) {
                _gridIndices[z][x].push_back(triIndex);
            }
        }
    }

    std::cout << "NavMesh Loaded. Triangles: " << _navTriangles.size() << std::endl;
    return true;
}



bool Map::CanGo(Vector3 pos, float Ypadding) {
    int x = static_cast<int>((pos.x - _minX) / _cellSize);
    int z = static_cast<int>((pos.z - _minZ) / _cellSize);

    if (x < 0 || x >= _width || z < 0 || z >= _height) return false;

    // 현재 위치한 그리드 셀에 등록된 삼각형들만 루프
    const auto& candidateIndices = _gridIndices[z][x];
    for (int index : candidateIndices) {
        if (pos.IsPointInTriangle( _navTriangles[index] , Ypadding)) {
            return true;
        }
    }

    return false;
}

bool Map::CheckProjectileCollision(Vector3 pos) 
{
    // 투사체 지형 충돌 검사
    // 현재는 이동 가능 여부(CanGo)의 결과를 반전시켜 반환합니다.
    // 갈 수 없는 곳(!CanGo)이면 투사체가 충돌(true)했다고 판정.
    return !CanGo(pos);
}


