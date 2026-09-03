#pragma once
#include "Types.h"
#include "DataContents.h"
#include "Creature.h"
#include "QuestComponent.h"

class Player : public Creature
{
public:

    Player(int32 objectId, std::shared_ptr<Session> sessionPtr, CoreRoomPtr coreroomptr , int32 characterId)
        : Creature(objectId, GameObjectType::Player, coreroomptr), session(sessionPtr) , _characterId(characterId)
    {
        //여기서 shared_from_this() 사용 금지!!( 생성자 반환 전에는 사용 불가 Init 함수를 이용할 것!)
    }

    std::weak_ptr<Session> session; // 순환 참조 방지

    virtual void Init(int32 templateId);

    int32 GetcharacterId() {
        return _characterId;
    }
private:
    int32 _characterId; //DB의 characterId

    

    
};
