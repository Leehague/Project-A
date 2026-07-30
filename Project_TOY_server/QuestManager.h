#pragma once
#include "Types.h"

namespace Protocol
{
    class QuestInfo;
}

class QuestManager
{
public:

    static QuestManager& GetInstance() {
        static QuestManager instance;
        return instance;
    }

    //동적 생성 퀘스트
    QuestPtr Create(Protocol::QuestInfo questinfo, GameObjectPtr client);

    //정적 생성 퀘스트
    QuestPtr Create(int32 templateId, GameObjectPtr client);


    //전역 조회 및 삭제는 성능을 위해(lock을 최소화 하기 위해) 구현하지 않음 , 필요시 구현
    //QuestPtr Find(int32 quest_server_id);
    //void RemoveQuest(int32 quest_server_id);
    



private:
    
    
    std::atomic<int64> _instanceIdGenerator{ 0 };
};



