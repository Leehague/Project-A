#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "GameObject.h"

class Item : public GameObject
{
public:

    Item(int32 objectId)
        : GameObject(objectId, GameObjectType::Item)
    {

    }


    void InitItem(int32 itemDbId, int32 itemTemplteId, int32 count, int32 slot);

private:
    
    int32 _itemDbId;
    int32 _itemTemplteId;
    int32 _count;
    int32 _slot;
    //아이템에서 HP는 일종의 내구도 개념으로 이해 하면 될것
};
