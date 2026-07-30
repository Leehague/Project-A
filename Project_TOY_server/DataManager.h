#pragma once
#include "json.hpp" // nlohmann/json
#include <fstream>
#include "DataContents.h"
#include <iostream>
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
        LoadItemData("Data/ItemData.json");
        LoadNpcData("Data/NPCData.json");
        LoadQuestData("Data/QuestData.json");
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
            stat.baseHp = item["MaxHp"];
            stat.baseMp = item["MaxMp"];
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
            mapdata.NavMeshPath = item["NavMeshPath"];
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
        //1. 스킬 타입별로 정리 , 현재로써는 '가이드 라인' 의 역할
        std::map<int32, std::vector<std::string>> infoMap;
        std::vector<std::string> commonInfo;
        for (auto& st : data["skillType"]) {
            int32 typeId = st["TypeId"];
            std::vector<std::string> info = st["Infolist"].get<std::vector<std::string>>();

            if (typeId == 0) commonInfo = info; // 0번은 공통
            else infoMap[typeId] = info;
        }

        // 2. 실제 스킬 데이터 로드
        for (auto& item : data["skills"]) {
            SkillData skill;

            // JSON 객체 형태이므로 필드명을 직접 참조하여 안전하게 로드
            // (Infolist에 있는 필드들을 순회하며 로드하는 방식도 가능하지만, 
            //  구조체 멤버가 고정되어 있으므로 아래 방식이 더 직관적입니다.)

            // [공통 필드]
            skill.id = item["id"];
            skill.name = item["name"].get<std::string>();
            skill.skillType = static_cast<SkillType>(item["skillTypeId"].get<int>());
            skill.coolTime = item["coolTime"];
            skill.costType = static_cast<CostType>(item["CostTypeId"].get<int>());
            skill.animName = item["animName"].get<std::string>();
            if (item.contains("cost")) skill.cost = item["cost"];

            // [타입별 선택적 필드] - item.contains() 또는 item.value() 사용
            if (item.contains("damage")) skill.damage = item["damage"];
            if (item.contains("range")) skill.range = item["range"];

            // Projectile 전용
            if (skill.skillType == SkillType::Projectile) {
                if (item.contains("projectileSpeed")) skill.projectileSpeed = item["projectileSpeed"];
                if (item.contains("projectileId")) skill.projectileId = item["projectileId"];
            }

            // Dash 전용
            if (skill.skillType == SkillType::Dash) {
                if (item.contains("dashDistance")) skill.dashDistance = item["dashDistance"];
                if (item.contains("dashSpeed")) skill.dashSpeed = item["dashSpeed"];
            }

            skill.targetType = static_cast<SkillTargetType>(item["targetTypeId"]);

            _skillTable[skill.id] = skill;
        }
    }

    void LoadItemData(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cout << "Cannot find file at: " << path << std::endl;
            return;
        }
        if (f.peek() == std::ifstream::traits_type::eof()) {
            std::cout << "File is empty!" << std::endl;
            return;
        }

        json data = json::parse(f);

        for (auto& item : data["items"]) {
            ItemData itemData;
            itemData.templateId = item["id"];
            itemData.name = item["name"];
            itemData.itemType = static_cast<ItemType>(item["itemTypeId"].get<int>());
            itemData.description = item["description"];
            itemData.iconPath = item["iconPath"];

            // Equipment specific
            if (item.contains("damage")) itemData.damage = item["damage"];
            if (item.contains("modelPath")) itemData.modelPath = item["modelPath"];

            // Consumable specific
            if (item.contains("value")) itemData.value = item["value"];
            if (item.contains("effect")) itemData.effect = item["effect"];
            if (item.contains("coolTime")) itemData.coolTime = item["coolTime"];
            if (item.contains("stackable")) itemData.stackable = item["stackable"];
            if (item.contains("maxStack")) itemData.maxStack = item["maxStack"];

            _itemTable[itemData.templateId] = itemData;
        }
    }

    void LoadNpcData(const std::string& path)
    {
        std::ifstream f(path);

        if (!f.is_open()) {
            std::cout << "Cannot find file at: " << path << std::endl;
            return;
        }
        if (f.peek() == std::ifstream::traits_type::eof()) {
            std::cout << "File is empty!" << std::endl;
            return;
        }

        json data = json::parse(f);


        for (auto& npc : data["npcs"])
        {
            NPCData npcdata;
            npcdata.id = npc["id"];
            npcdata.name = npc["name"];
            npcdata.statid = npc["statid"];

            _npcTable[npcdata.id] = npcdata;
        }
    }

    void LoadQuestData(const std::string& path)
    {
        std::ifstream f(path);

        if (!f.is_open()) {
            std::cout << "Cannot find file at: " << path << std::endl;
            return;
        }
        if (f.peek() == std::ifstream::traits_type::eof()) {
            std::cout << "File is empty!" << std::endl;
            return;
        }

        json data = json::parse(f);



        for (auto& quest : data["quests"])
        {
            QuestData questdata;

            questdata.id = quest["id"];
            questdata.name = quest["name"];
            questdata.questTypeId = quest["questTypeId"];
            questdata.rewardTypeid = quest["rewardTypeid"];

            if (quest.contains("trargetMonsterId")) questdata.trargetMonsterId = quest["trargetMonsterId"];
            if (quest.contains("targetCount")) questdata.targetCount = quest["targetCount"];
            if (quest.contains("rewardExp")) questdata.rewardExp = quest["rewardExp"];
            if (quest.contains("rewardItemTemplateId")) questdata.rewardItemTemplateId = quest["rewardItemTemplateId"];
            if (quest.contains("rewardItemCount")) questdata.rewardItemCount = quest["rewardItemCount"];
            if (quest.contains("rewardGold")) questdata.rewardGold = quest["rewardGold"];

            _questTable[questdata.id] = questdata;
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
    const ItemData* GetItem(int32 templateId) {
        if (_itemTable.find(templateId) == _itemTable.end()) return nullptr;
        return &_itemTable[templateId];
    }

    const NPCData* GetNPC(int32 NPCtemplateId)
    {
        if (_npcTable.find(NPCtemplateId) == _npcTable.end()) return nullptr;
        return &_npcTable[NPCtemplateId];
    }

    const QuestData* GetQuest(int32 questId)
    {
        if (_questTable.find(questId) == _questTable.end()) return nullptr;
        return &_questTable[questId];
    }
private:
    std::map<int32, StatData> _statTable; //key : tempateId , value : StatData
    std::map<int32, MapData> _mapTable; //key : MapId, value : MapData
    std::unordered_map<int32, SkillData> _skillTable; //key : Id , value : SkillData
    std::unordered_map<int32, ItemData> _itemTable; //key : Id, value : ItemData
    std::unordered_map<int32, NPCData> _npcTable;
    std::unordered_map<int32, QuestData> _questTable;


};
