#include "Map.h"
#include <fstream>
#include <iostream>
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

bool Map::CanGo(Vector3 pos)
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
