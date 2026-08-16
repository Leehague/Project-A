using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

[Serializable]
public class QuestTemplate
{
    //Common
    public int id; //templateId 
    public string name;
    public int questTypeId;
    public int rewardTypeid;


    //questType: Kill type
    public int trargetMonsterId;
    public int targetCount;

    //rewardType: Exp
    public int rewardExp;

    //rewardType: Item
    public int rewardItemTemplateId;
    public int rewardItemCount;

    //rewardType : Gold
    public int rewardGold;
}


[Serializable]
public class QuestTemplateData : ILoader<int, QuestTemplate>
{
    public List<QuestTemplate> quests = new List<QuestTemplate>();

    public Dictionary<int, QuestTemplate> MakeDict()
    {
        Dictionary<int, QuestTemplate> dict = new Dictionary<int, QuestTemplate>();
        foreach (QuestTemplate quest in quests)
            dict.Add(quest.id, quest);
        return dict;
    }
}
