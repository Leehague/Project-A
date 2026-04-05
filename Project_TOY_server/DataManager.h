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
        // LoadItemData(...); // 확장 가능
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
            stat.baseAttack = item["attack"];
            stat.speed = item["speed"];
            _statTable[stat.templateId] = stat;
        }
    }

public:
    const StatData* GetStat(int32 templateId) {
        if (_statTable.find(templateId) == _statTable.end()) return nullptr;
        return &_statTable[templateId];
    }

private:
    std::map<int32, StatData> _statTable; //key : tempateId , value : StatData
};