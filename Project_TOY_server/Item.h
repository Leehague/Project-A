#pragma once
#include "Types.h"
#include "Protocol/Protocol.pb.h"
#include "GameObject.h"

struct ItemInfo 
{
    int32 itemDbId;
    int32 itemTemplateId;
    int32 count;
    int32 slot;
    std::string itemMemo ="";
};


class Item : public GameObject
{
public:

    Item(int32 objectId)
        : GameObject(objectId, GameObjectType::Item)
    {

    }


    void InitItem(ItemInfo iteminfo);

    //getters
    int32 GetItemDBid() 
    {
        return _iteminfo.itemDbId;
    }
    /*virtual int32 GetTemplateId() 
    {
        return _iteminfo.itemTemplateId;
    }*/
    int32 GetCount() 
    {
        return _iteminfo.count;
    }
    int32 GetSlot() 
    {
        return _iteminfo.slot;
    }

    std::string GetMemo()
    {
        return _iteminfo.itemMemo;
    }

public:
    
private:
    ItemInfo _iteminfo;
    
    //아이템에서 HP는 일종의 내구도 개념으로 이해 하면 될것
};
