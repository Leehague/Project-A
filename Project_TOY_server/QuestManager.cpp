#include "QuestManager.h"
#include "Protocol/Protocol.pb.h"
#include "Kill_Quest.h"
#include "DataManager.h"


QuestPtr QuestManager::Create(Protocol::QuestInfo questinfo, GameObjectPtr client)
{
    // 전역 고유 Instance ID 생성
    int64 uniqueInstanceId = ++_instanceIdGenerator;
    int32 questId = static_cast<int32>(uniqueInstanceId); // 현재 Quest 생성자가 int32를 받으므로 캐스팅
    // 1. 프로토콜 정보에 있는 quest_type_id를 보고 분기 생성
    int32 questTypeId = questinfo.quest_type_id();
    switch (questTypeId)
    {
    case 1: // 처치형 퀘스트 (Kill Quest)
        return std::make_shared<Kill_Quest>(questId, questinfo, client);

        // 추후 수집형이나 탐험형 퀘스트 추가 시 분기 확장 가능
        // case 2: 
        //     return std::make_shared<Collect_Quest>(questId, questinfo, client);

    default:
        std::cout << "Undefined Quest Type: " << questTypeId << std::endl;
        return nullptr;
    }
}

QuestPtr QuestManager::Create(int32 templateId, GameObjectPtr client)
{
    // 전역 고유 Instance ID 생성
    int64 uniqueInstanceId = ++_instanceIdGenerator;
    int32 questId = static_cast<int32>(uniqueInstanceId); // 현재 Quest 생성자가 int32를 받으므로 캐스팅
    // 2. DataManager에서 템플릿 정보를 조회하여 어떤 타입의 퀘스트인지 판별
    const QuestData* questData = DataManager::GetInstance().GetQuest(templateId);
    if (questData == nullptr)
    {
        std::cout << "Quest template not found: " << templateId << std::endl;
        return nullptr;
    }
    switch (questData->questTypeId)
    {
    case 1: // 처치형 퀘스트 (Kill Quest)
        return std::make_shared<Kill_Quest>(questId, templateId, client);

    default:
        std::cout << "Undefined Quest Type in Data: " << questData->questTypeId << std::endl;
        return nullptr;
    }
}


