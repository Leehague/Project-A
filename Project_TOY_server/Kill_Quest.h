#pragma once
#include "Quest.h"
#include <utility> // pair 사용
#include "QuestEvent.h"

//특정한 templatedId를 가지는 오브젝트를 Kill 하는 퀘스트
class Kill_Quest : public Quest
{
private:
    int CurrentKillCount;
    
    int GoalKillCount;

    int32 _target_Obejct_templatedId;

public:
    Kill_Quest(int32 questid, int32 quest_templateid , GameObjectPtr client);
    Kill_Quest(int32 questid ,Protocol::QuestInfo questinfo, GameObjectPtr client);

    void OnEvent(const QuestEvent& event);

    int32 GetCurrentCount();
};
