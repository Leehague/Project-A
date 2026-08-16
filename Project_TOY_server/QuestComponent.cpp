#include "QuestComponent.h"
#include "DataManager.h"
#include "Protocol/Protocol.pb.h"
#include "QuestManager.h"
#include "Creature.h"


void QuestComponent::HandleEvent(const QuestEvent& event)
{
    for (const auto& pair : _activeQuests)
    {
        QuestPtr quest= pair.second;
        quest->OnEvent(event);
    }
}

bool QuestComponent::REQAcceptQuest(GameObjectPtr client,int64 quest_id)
{
    CreaturePtr creautre_client = std::dynamic_pointer_cast<Creature>(client);

    if (creautre_client == nullptr)
    {
        return false;
    }

    QuestComponentPtr clientquestcomponent = creautre_client->GetQuestComponent();

    QuestPtr outquest;
    if (clientquestcomponent->RESAcceptQuest(quest_id, _owner, outquest))
    {
        if (outquest)
        {
            AddactiveQuest(outquest);

            return true;
        }

        return false;
    }
    else
    {
        return false;
    }
}

void QuestComponent::FindQuestInfoactiveQuest(int64 quest_id, Protocol::QuestInfo* outquestinfo)
{
    _activeQuests[quest_id]->FillQuestInfo(outquestinfo);
}

std::vector<Protocol::QuestInfo> QuestComponent::GetQuestInfoListAsAcquirer()
{
    std::vector<Protocol::QuestInfo> result;


    for (const auto& [quest_id, quest] : _activeQuests) {
        Protocol::QuestInfo info;
        quest->FillQuestInfo(&info);
        result.push_back(info);
    }

    return result;
}

QuestPtr QuestComponent::CreateQuest(int32 quest_tempalateid)
{
    
    QuestPtr quest = QuestManager::GetInstance().Create(quest_tempalateid,_owner);

    AddQuestAsClient(quest->GetQuestId(), quest);
    
    return quest;
}

QuestPtr QuestComponent::CreateQuest(Protocol::QuestInfo questinfo)
{
    
    QuestPtr quest = QuestManager::GetInstance().Create(questinfo, _owner);
  
    AddQuestAsClient(quest->GetQuestId(), quest);
    
    return quest;
}

bool QuestComponent::RESAcceptQuest(int64 quest_id, GameObjectPtr acquir, QuestPtr& outquest)
{
    //TODO: 퀘스트 수락 가능 여부 확인
    // 1. 기획 에서 퀘스트 수락 조건 추가
    // 2. (비정상 요청 거부) ex) client 와 acquir의 현재위치가 매우 멂 등

    QuestPtr quest = _QuestsAsClient[quest_id];

    quest->Accept(acquir);

    outquest = quest;

    return true;
}

std::vector<Protocol::QuestInfo> QuestComponent::GetQuestInfoListAsClient()
{
    std::vector<Protocol::QuestInfo> result;


    for (const auto& [quest_id, quest] : _QuestsAsClient) {
        Protocol::QuestInfo info;
        quest->FillQuestInfo(&info);
        result.push_back(info);
    }

    return result;
}
