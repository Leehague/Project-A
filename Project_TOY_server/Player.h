#pragma once
#include "Types.h"
#include "DataContents.h"
#include "GameObject.h"

class Player : public GameObject
{
public:

    Player(int32 objectId, std::shared_ptr<Session> sessionPtr)
        : GameObject(objectId, GameObjectType::Player), session(sessionPtr)
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
