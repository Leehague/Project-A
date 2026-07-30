#pragma once
#include "Types.h"
#include "GameObject.h"

//여기서 QuestType과 QuestEventType은 다른 것임
enum class QuestEventType
{
    Basic =0,
    KillObejct =1,
    UpdateQuestByclient =2,
    Rewardreceive =3,
};


struct QuestEvent
{
    QuestEventType questeventtype;
    QuestType questtype;
    
    int32 target_Obejct_templatedId;

    GameObjectType target_ObjectType;
    int32 Count;
};

