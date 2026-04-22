#include "Map.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include "DataContents.h"

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
        std::cout << "MapData 불일치" << std::endl;

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

bool Map::CanGo_Old(Vector3 pos)
{
    // 월드 좌표 -> 인덱스 변환 공식
    // (현재좌표 - 시작좌표) / 한 칸의 크기
    int x = static_cast<int>((pos.x - _minX) / _cellSize);
    int z = static_cast<int>((pos.z - _minZ) / _cellSize);

    if (x < 0 || x >= _width || z < 0 || z >= _height)
        return false;

    return !_collisionData[z][x];
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
//bool Map::LoadNavMesh(const std::string& path) {
//    std::ifstream ifs(path, std::ios::binary);
//    if (!ifs.is_open()) return false;
//
//    // 1. 정점 로드
//    int vertCount;
//    ifs.read((char*)&vertCount, sizeof(int));
//    std::vector<Vector3> vertices(vertCount);
//    
//    ifs.read((char*)vertices.data(), sizeof(Vector3) * vertCount);
//
//    //for (int i = 0; i < vertCount; i++) {
//    //    // x, y, z를 각각 4바이트씩 읽어서 대입
//    //    ifs.read((char*)&vertices[i].x, sizeof(float));
//    //    ifs.read((char*)&vertices[i].y, sizeof(float));
//    //    ifs.read((char*)&vertices[i].z, sizeof(float));
//    //}
//
//    // 2. 인덱스 로드 후 삼각형 구성
//    int indexCount;
//    ifs.read((char*)&indexCount, sizeof(int));
//    for (int i = 0; i < indexCount; i += 3) {
//        int i1, i2, i3;
//        ifs.read((char*)&i1, sizeof(int));
//        ifs.read((char*)&i2, sizeof(int));
//        ifs.read((char*)&i3, sizeof(int));
//
//        _navTriangles.push_back({ vertices[i1], vertices[i2], vertices[i3] });
//    }
//
//    // 2. 그리드 인덱스 공간 확보
//    _gridIndices.assign(_height, std::vector<std::vector<int>>(_width));
//
//    // 3. 각 삼각형을 그리드에 등록
//    for (int i = 0; i < _navTriangles.size(); i++) {
//        const auto& tri = _navTriangles[i];
//
//        // 삼각형의 AABB를 그리드 좌표로 변환
//        int startX = static_cast<int>((tri.minX - _minX) / _cellSize);
//        int endX = static_cast<int>((tri.maxX - _minX) / _cellSize);
//        int startZ = static_cast<int>((tri.minZ - _minZ) / _cellSize);
//        int endZ = static_cast<int>((tri.maxZ - _minZ) / _cellSize);
//
//        // 유효 범위 클램핑
//        startX = std::max(0, std::min(startX, _width - 1));
//        endX = std::max(0, std::min(endX, _width - 1));
//        startZ = std::max(0, std::min(startZ, _height - 1));
//        endZ = std::max(0, std::min(endZ, _height - 1));
//
//        // 해당 범위의 모든 그리드 칸에 내 인덱스를 등록
//        for (int z = startZ; z <= endZ; z++) {
//            for (int x = startX; x <= endX; x++) {
//                _gridIndices[z][x].push_back(i);
//            }
//        }
//    }
//
//    std::cout << "Total Triangles: " << _navTriangles.size() << std::endl;
//    if (!_navTriangles.empty()) {
//        std::cout << "Sample Triangle 0: V1(" << _navTriangles[0].v1.x << ", " << _navTriangles[0].v1.z << ")" << std::endl;
//    }
//    return true;
//}

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

// 점이 삼각형 내부에 있는지 판단하는 함수 (Barycentric coordinate 방식)
bool IsPointInTriangle(Vector3 p, Triangle tri) {
    // 1. 높이(Y) 검증: 점 P가 삼각형의 평면 높이와 너무 멀리 떨어져 있으면 false
    // NavMesh 데이터이므로 삼각형 정점들의 평균 Y값이나 
    // 최소/최대 Y 범위를 기준으로 허용 오차를 둡니다.
    float minY = std::min({ tri.v1.y, tri.v2.y, tri.v3.y }) - 0.5f;
    float maxY = std::max({ tri.v1.y, tri.v2.y, tri.v3.y }) + 0.5f;
    if (p.y < minY || p.y > maxY) return false;

    // 2. XZ 평면 판정 (Barycentric Coordinate)
    // 벡터 정의
    float v0x = tri.v3.x - tri.v1.x; float v0z = tri.v3.z - tri.v1.z; // AC
    float v1x = tri.v2.x - tri.v1.x; float v1z = tri.v2.z - tri.v1.z; // AB
    float v2x = p.x - tri.v1.x;      float v2z = p.z - tri.v1.z;      // AP

    // 도트 프로덕트(내적) 계산 (XZ 평면용)
    float dot00 = v0x * v0x + v0z * v0z;
    float dot01 = v0x * v1x + v0z * v1z;
    float dot02 = v0x * v2x + v0z * v2z;
    float dot11 = v1x * v1x + v1z * v1z;
    float dot12 = v1x * v2x + v1z * v2z;

    // Barycentric 좌표(u, v) 계산
    float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
    float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
    float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

    // 최종 판정: u >= 0, v >= 0, u + v <= 1 이면 내부
    // 부동 소수점 오차를 고려하여 아주 작은 값(EPSILON)을 적용합니다.
    const float EPSILON = 0.001f;
    return (u >= -EPSILON) && (v >= -EPSILON) && (u + v <= 1.0f + EPSILON);
}

bool Map::CanGo(Vector3 pos) {
    int x = static_cast<int>((pos.x - _minX) / _cellSize);
    int z = static_cast<int>((pos.z - _minZ) / _cellSize);

    if (x < 0 || x >= _width || z < 0 || z >= _height) return false;

    // 현재 위치한 그리드 셀에 등록된 삼각형들만 루프
    const auto& candidateIndices = _gridIndices[z][x];
    for (int index : candidateIndices) {
        if (IsPointInTriangle(pos, _navTriangles[index])) {
            return true;
        }
    }

    return false;
}