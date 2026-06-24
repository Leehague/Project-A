#pragma once
#include "Types.h"
#include "DataContents.h"
#include "Creature.h"

class Player : public Creature
{
public:

    Player(int32 objectId, std::shared_ptr<Session> sessionPtr, CoreRoomPtr coreroomptr)
        : Creature(objectId, GameObjectType::Player, coreroomptr), session(sessionPtr)
    {
        
    }

    std::weak_ptr<Session> session; // 순환 참조 방지

    virtual void Init(int32 templateId);
   
    InventoryPtr GetInventory()
    {
        return OwnInventory;
    }


private:
    InventoryPtr OwnInventory;

};
