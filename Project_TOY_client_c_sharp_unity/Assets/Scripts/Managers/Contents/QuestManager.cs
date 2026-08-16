using Protocol;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


public class QuestManager
{

    public Dictionary<int, Quest> Quests {  get; private set; } = new Dictionary<int, Quest>();

    public void AddorUpdateQuest(QuestInfo questinfo)
    {

        if (Quests.ContainsKey(questinfo.QuestId))
        {

            //key가 이미 존재함으로 업데이트 상황
            Quests[questinfo.QuestId].Update(questinfo);
        }
        else
        {
            //key가 존재하지 않으므로 새로운 퀘스트 생성
            Quest quest = new Quest(questinfo);
            Quests[questinfo.QuestId] = quest;
        }

    }

    public Quest GetQuest(int questid)
    {
        if (!Quests.ContainsKey(questid))
        {
            //예외 처리

            return null;
        }

        return Quests[questid];
    }

    public Quest GetFirstActiveQuest()
    {
        return Quests.Values.FirstOrDefault();
    }

    
}

