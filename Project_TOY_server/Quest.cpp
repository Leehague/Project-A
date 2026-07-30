#include "Quest.h"
#include "DataManager.h"
#include "Protocol/Protocol.pb.h"

Quest::Quest(int32 questId, int32 quest_templateid, GameObjectPtr client)
{
    questdata = *DataManager::GetInstance().GetQuest(quest_templateid);

    _quest_id = questId;
    _client = client;
    _acquirer = nullptr;
    queststate = QuestState::NotAccepted; //퀘스트 생성시에는 아직 인수자가 없음

    _IsStaticQuest = true;
}

Quest::Quest(int32 questId , Protocol::QuestInfo questinfo, GameObjectPtr client)
{
    questdata = QuestInfoToQuestData(questinfo);
    _quest_id = questId;
    _client = client;
    _acquirer = nullptr;
    queststate = QuestState::NotAccepted;

    _IsStaticQuest = false;
}


//해당 Quest의 내부 인자를 바탕으로 QuestInfo를 채워주는 함수 (proto의 QuestInfo가 수정될시 수정되어야 하는 함수)
void Quest::FillQuestInfo(Protocol::QuestInfo* outInfo)
{
    if (outInfo == nullptr) return;
    // QuestData(정적)와 런타임 상태값(동적)을 조합하여 일원화된 맵핑 수행
    outInfo->set_quest_id(_quest_id);
    outInfo->set_quest_templateid(questdata.id);
    outInfo->set_quest_type_id(questdata.questTypeId);
    outInfo->set_state(static_cast<int32>(queststate));

    outInfo->set_client_object_id(_client ? _client->GetObjectId() : 0);
    outInfo->set_acquirer_object_id(_acquirer ? _acquirer->GetObjectId() : 0);

    outInfo->set_target_template_id(questdata.trargetMonsterId);
    outInfo->set_current_count(GetCurrentCount()); // 파생 클래스(Kill_Quest 등)에서 카운트 반환
    outInfo->set_target_count(questdata.targetCount);

    outInfo->set_reward_type_id(questdata.rewardTypeid);

    // 보상 타입 ID에 따라 필드 분류 매핑 
    // (1: Exp, 2: Item, 3: Gold)
    int32 rewardval = -1;
    int32 rewardItemId = -1;
    switch (questdata.rewardTypeid) {
    case 1: //Exp
        rewardval = questdata.rewardExp;
        break; 
    case 2: //Item
        rewardval = questdata.rewardItemCount;
        rewardItemId = questdata.rewardItemTemplateId;
        break;
    case 3: //Gold
        rewardval = questdata.rewardGold;
        break;
    }
    outInfo->set_reward_value(rewardval);
    outInfo->set_reward_item_id(rewardItemId);
    outInfo->set_name(questdata.name);
}

QuestData Quest::QuestInfoToQuestData(Protocol::QuestInfo questinfo)
{
    QuestData data;

    // 1. 공통 정보 매핑
    data.id = questinfo.quest_templateid();
    data.name = questinfo.name(); 
    data.questTypeId = questinfo.quest_type_id();
    data.rewardTypeid = questinfo.reward_type_id();
    // 2. 퀘스트 타입 조건 매핑 (예: Kill 퀘스트인 경우)
    data.trargetMonsterId = questinfo.target_template_id();
    data.targetCount = questinfo.target_count();
    // 3. 보상 정보 초기화 (안전을 위해 0으로 먼저 세팅)
    data.rewardExp = 0;
    data.rewardItemTemplateId = 0;
    data.rewardItemCount = 0;
    data.rewardGold = 0;
    // 4. 보상 타입 ID에 따라 필드 분류 매핑 
    // (1: Exp, 2: Item, 3: Gold)
    switch (questinfo.reward_type_id())
    {
    case 1: // Exp
        data.rewardExp = questinfo.reward_value();
        break;
    case 2: // Item
        data.rewardItemTemplateId = questinfo.reward_item_id();
        data.rewardItemCount = questinfo.reward_value();
        break;
    case 3: // Gold
        data.rewardGold = questinfo.reward_value();
        break;
    default:
        break;
    }
    return data;
}
