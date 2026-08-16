using Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


public enum QuestState
{
    NotAccepted = 0, //이상태에서는 client 만 있고 acquirer는 없음
    Accepted =1, // 진행중인 퀘스트 ,client acquirer 둘다 존재
    Completed =2, // 조건을 만족한 퀘스트 그러나 보상 수령 전까지는 Accepted로 돌아 갈 수 있음
    Rewardreceived =3
}



public class Quest
{
    private QuestTemplate questdata;


 
    public Quest(QuestInfo questinfo)
    {
        this.quest_template_id = questinfo.QuestTemplateId;
        this.questdata = QuestinfoToQuestData(questinfo);
        this.quest_id = questinfo.QuestId;
        this.quest_state =  (QuestState)questinfo.State; 
        this.client_object_id = questinfo.ClientObjectId;

        target_count = questdata.targetCount;
        current_count = 0;
    }



    public int quest_id { get; private set; }

    public int quest_template_id { get; private set; }

    public QuestState quest_state { get; private set; }

    public int client_object_id { get; private set; }

    public int acquirer_object_id { get; private set; }


    public int target_count { get; private set; }

    public int current_count { get; private set; }

    public string Name { get { return questdata.name; } }

    public void Update(QuestInfo questinfo)
    {
        target_count = questinfo.TargetCount;
        current_count = questinfo.CurrentCount;
        quest_state = (QuestState)questinfo.State;

    }

    

    public QuestTemplate QuestinfoToQuestData(QuestInfo questinfo)
    {
        QuestTemplate data = new QuestTemplate();

        // 1. 공통 정보 매핑
        data.id = questinfo.QuestTemplateId;
        data.name = questinfo.Name;
        data.questTypeId = questinfo.QuestTemplateId;
        data.rewardTypeid = questinfo.RewardTypeId;
        // 2. 퀘스트 타입 조건 매핑 (예: Kill 퀘스트인 경우)
        data.trargetMonsterId = questinfo.TargetTemplateId;
        data.targetCount = questinfo.TargetCount;
        // 3. 보상 정보 초기화 (안전을 위해 0으로 먼저 세팅)
        data.rewardExp = 0;
        data.rewardItemTemplateId = 0;
        data.rewardItemCount = 0;
        data.rewardGold = 0;
        // 4. 보상 타입 ID에 따라 필드 분류 매핑 
        // (1: Exp, 2: Item, 3: Gold)
        switch (questinfo.RewardTypeId)
        {
            case 1: // Exp
                data.rewardExp = questinfo.RewardValue;
                break;
            case 2: // Item
                data.rewardItemTemplateId = questinfo.RewardItemId;
                data.rewardItemCount = questinfo.RewardValue;
                break;
            case 3: // Gold
                data.rewardGold = questinfo.RewardValue;
                break;
            default:
                break;
        }

        return data;
    }
}

