#include "Kill_Quest.h"
#include "FinanceService.h"
#include "ObjectManager.h"
#include "Creature.h"
#include "Item.h"
#include "Protocol/Protocol.pb.h"

Kill_Quest::Kill_Quest(int32 questid, int32 quest_templateid, GameObjectPtr client) : Quest(questid , quest_templateid,client)
{
    
    _target_Obejct_templatedId = questdata.trargetMonsterId;
    GoalKillCount = questdata.targetCount;
    CurrentKillCount = 0;
}

Kill_Quest::Kill_Quest(int32 questid, Protocol::QuestInfo questinfo, GameObjectPtr client) :Quest(questid,questinfo,client)
{
    _target_Obejct_templatedId = questinfo.target_template_id();
    GoalKillCount = questinfo.target_count();
    CurrentKillCount = 0;
}

void Kill_Quest::OnEvent(const QuestEvent& event)
{
    
    /*if (event.questtype != GetQuestType())
    {
        return;
    }*/

    if (event.questeventtype == QuestEventType::KillObejct  && event.target_Obejct_templatedId == _target_Obejct_templatedId)
    {
        CurrentKillCount += event.Count;
    }

    if (event.questeventtype == QuestEventType::UpdateQuestByclient && event.target_Obejct_templatedId == _target_Obejct_templatedId)
    {
        GoalKillCount += event.Count;
    }

    if (CurrentKillCount >= GoalKillCount)
    {
        SetQuestState(QuestState::Completed);
    }

    if (GoalKillCount > CurrentKillCount)
    {
        SetQuestState(QuestState::Accepted);
    }

    if (event.questeventtype == QuestEventType::Rewardreceive)
    {
        QuestState state = GetQuestState();

        if (state != QuestState::Completed)
        {
            return;
        }
        else
        {
            state = QuestState::Rewardreceived;

            //TODO 실질적인 reward 지급 로직
            GameObjectPtr go = GetQuestAcquirer();
            CreaturePtr acquirer = std::dynamic_pointer_cast<Creature>(go);
            if (acquirer == nullptr) { return; }

            //아이템 생성 및 발급
            
            GameObjectPtr item =GObjcetManager.Create(GameObjectType::Item, nullptr, questdata.rewardItemTemplateId, acquirer->GetCoreroomptr());

            ItemPtr itemptr = std::dynamic_pointer_cast<Item>(item);

            if (itemptr == nullptr) { return; }
            itemptr->SetCount(questdata.rewardItemCount);
            FinanceResult result =  FinanceService::AcquireItem(acquirer, item->GetObjectId(), itemptr->GetCount(), ItemChangeReason::QuestReward);


            if (result != FinanceResult::Success)
            {
                //예외처리
            }
        }
    }

}

int32 Kill_Quest::GetCurrentCount()
{
    return CurrentKillCount;
}

