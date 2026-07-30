#pragma once
#include <vector>
#include <memory>
#include "QuestEvent.h"
#include "Types.h"
#include "Quest.h"
#include "Kill_Quest.h"
#include <unordered_map>

namespace Protocol
{
    class QuestInfo;
}


class QuestComponent 
{
    //acquir , 퀘스트 수행자로써의 기능
public:
    QuestComponent(GameObjectPtr owner) : _owner(owner) {}
    // 이벤트를 수신하여 현재 진행 중인 모든 퀘스트에 전달(Notify)
    void HandleEvent(const QuestEvent& event);

    //퀘스트 수락을 '요청' 하는 함수
    bool REQAcceptQuest(GameObjectPtr client, int64 quest_id);

    void FindQuestInfoactiveQuest(int64 quest_id, Protocol::QuestInfo* outquestinfo);

private:
    //자신이 수행할 퀘스트를 추가함
    void AddactiveQuest(QuestPtr quest) { _activeQuests[quest->GetQuestId()] = quest; }

    GameObjectPtr _owner;
    std::unordered_map<int32, QuestPtr> _activeQuests; // 진행 중인 퀘스트 (Observers)
    std::vector<int32> _RewardreceivedQuestIds;        // 완료된(보상을 수령한) 퀘스트 이력


    //client, 퀘스트 생성자로써의 기능
public:
    QuestPtr CreateQuest(int32 quest_tempalateid); //정적 퀘스트 생성
    QuestPtr CreateQuest(Protocol::QuestInfo questinfo); //동적 퀘스트 생성

    //acquir 측의 questcomponet에서 client questcomponet의 이 함수를 호출해야함
    bool RESAcceptQuest(int64 quest_id, GameObjectPtr acquir, QuestPtr& outquest);

private:

    void AddQuestAsClient(int32 questid, QuestPtr quest)
    {
        _QuestsAsClient[questid] = quest;
    }
    //client 입장 (퀘스트 발행자)으로의 퀘스트
    std::unordered_map<int32, QuestPtr> _QuestsAsClient;
};
