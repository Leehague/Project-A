#pragma once
#include "DataContents.h"
#include "QuestEvent.h"
#include "Types.h"
#include <iostream>

namespace Protocol
{
    class QuestInfo;
}

enum class QuestState
{
    NotAccepted = 0, //이상태에서는 client 만 있고 acquirer는 없음
    Accepted =1, // 진행중인 퀘스트 ,client acquirer 둘다 존재
    Completed =2, // 조건을 만족한 퀘스트 그러나 보상 수령 전까지는 Accepted로 돌아 갈 수 있음
    Rewardreceived =3
};

//NotAccepted <-> Accepted <-> Completed ->Rewardreceived

class Quest
{

public:

    //정적인 퀘스트 생성 인경우
    Quest(int32 questId,int32 quest_templateid, GameObjectPtr client);

    //동적인 퀘스트 생성 인 경우
    Quest(int32 questId, Protocol::QuestInfo questinfo,GameObjectPtr client);

public:
    //QuestType enum class 정의 와 id가 일치 해야함
    QuestType GetQuestType() {
        return (QuestType)questdata.questTypeId;
    }
    // 퀘스트 상태 업데이트 핵심 추상 함수
    virtual void OnEvent(const QuestEvent& event) = 0;

    // 모든 퀘스트는 CurrentCount 에 해당하는 값을 반환해야함
    virtual int32 GetCurrentCount() = 0;

    int32 GetQuestId()
    {
        return _quest_id;
    }
    

    QuestState GetQuestState() {
        return queststate;
    }

    GameObjectPtr GetQuestAcquirer()
    {
        return _acquirer;
    }

    GameObjectPtr GetClient()
    {
        return _client;
    }

    int32 GetQuestTypeId()
    {
        return questdata.questTypeId;
    }

   
    void SetQuestState(QuestState state)
    {
        //Rewardreceived 상태인 경우 퀘스트 상태 변경을 해서는 안됨, 로직 오류임 이에 대한 방어코드
        if (queststate == QuestState::Rewardreceived)
        {
            std::cout << "detated chaning try for Quest that is QuestState::Rewardreceived " << std::endl;
            return;
        }
        queststate = state;
    }
    
    void Accept(GameObjectPtr acquirer)
    {
        _acquirer = acquirer;
        SetQuestState(QuestState::Accepted);
    }

    bool IsStaticQuest()
    {
        return _IsStaticQuest;
    }

    QuestData GetQuestData()
    {
        return questdata;
    }

    //해당 Quest의 내부 인자를 바탕으로 QuestInfo를 채워주는 함수 (proto의 QuestInfo가 수정될시 수정되어야 하는 함수)
    void FillQuestInfo(Protocol::QuestInfo* outinfo);
   
private:

    GameObjectPtr _client; //퀘스트 발행자(요청자)
    GameObjectPtr _acquirer; //퀘스트를 받은 대상
    QuestState queststate;

    bool _IsStaticQuest;
    

    QuestData QuestInfoToQuestData(Protocol::QuestInfo questinfo);
    


protected:
    QuestData questdata;
    int32 _quest_id;

};
