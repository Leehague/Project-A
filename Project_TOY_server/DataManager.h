#pragma once
#include "json.hpp" // nlohmann/json
#include <fstream>
#include "DataContents.h"
using json = nlohmann::json;

//데이터를 초기화 시 가져와서 메모리에 올려 놓는 역할 및 필요시 해당 데이터를 건네주는 클래스
class DataManager {
public:
    static DataManager& GetInstance() {
        static DataManager instance;
        return instance;
    }

    void Init() {
        LoadStatData("Data/StatData.json");
        LoadMapData("Data/MapData.json");
        LoadSkillData("Data/SkillData.json");
    }

private:
    void LoadStatData(const std::string& path) {
        std::ifstream f(path);

        if (!f.is_open()) {
            // 이 메시지가 뜨면 경로가 잘못된 것입니다.
            std::cout << "Cannot find file at: " << path << std::endl;
            return;
        }

        // 파일이 비어있는지도 확인하면 좋습니다.
        if (f.peek() == std::ifstream::traits_type::eof()) {
            std::cout << "File is empty!" << std::endl;
            return;
        }

        json data = json::parse(f);
        
        for (auto& item : data["stats"]) {
            StatData stat;
            stat.templateId = item["id"]; //json 파일에서의 id는 templateId 를 말하는 것임
            stat.baseHp = item["hp"];
            stat.baseMp = item["mp"];
            stat.baseAttack = item["attack"];
            stat.speed = item["speed"];
            stat.name = item["name"];
            _statTable[stat.templateId] = stat;
        }
    }

    void LoadMapData(const std::string& path)
    {
        std::ifstream f(path);

        if (!f.is_open()) {
            // 이 메시지가 뜨면 경로가 잘못된 것입니다.
            std::cout << "Cannot find file at: " << path << std::endl;
            return;
        }

        // 파일이 비어있는지도 확인하면 좋습니다.
        if (f.peek() == std::ifstream::traits_type::eof()) {
            std::cout << "File is empty!" << std::endl;
            return;
        }

        json data = json::parse(f);

        for (auto& item : data["Maps"]) {
            MapData mapdata;
            mapdata.MapId = item["id"];
            mapdata.mapName = item["name"];
            mapdata.MapPath = item["MapPath"];
            mapdata.MinX = item["MinX"];
            mapdata.MinZ = item["MinZ"];
            mapdata.width = item["Width"];
            mapdata.height = item["Height"];
            mapdata.CellSize = item["CellSize"];
            _mapTable[mapdata.MapId] = mapdata;
        }
    }
    void LoadSkillData(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return;

        json data = json::parse(f);
        for (auto& item : data["skills"]) {
            SkillData skill;
            skill.id = item["id"];
            skill.name = item["name"];
            skill.skillType = static_cast<SkillType>(item["skillType"].get<int>());
            skill.costType = static_cast<CostType>(item["CostType"].get<int>());
            skill.damage = item["damage"];
            skill.range = item["range"];
            skill.coolTime = item["coolTime"];
            skill.animName = item["animName"];
            skill.cost = item["cost"];
            _skillTable[skill.id] = skill;
        }
    }

    
public:
    const StatData* GetStat(int32 templateId) {
        if (_statTable.find(templateId) == _statTable.end()) return nullptr;
        return &_statTable[templateId];
    }
    const MapData* GetMapData(int32 MapId) 
    {
        if (_mapTable.find(MapId) == _mapTable.end()) return nullptr;
        return &_mapTable[MapId];
    }
    const SkillData* GetSkill(int32 id) {
        if (_skillTable.find(id) == _skillTable.end()) return nullptr;
        return &_skillTable[id];
    }
private:
    std::map<int32, StatData> _statTable; //key : tempateId , value : StatData
    std::map<int32, MapData> _mapTable; //key : MapId, value : MapData
    std::unordered_map<int32, SkillData> _skillTable; //key : Id , value : SkillData
};