#pragma once
#include "Types.h"
#include "GameObject.h"
#include "InfoSturct.h"



class Item : public GameObject
{
public:

    Item(int32 objectId, CoreRoomPtr coreroomptr)
        : GameObject(objectId, GameObjectType::Item, coreroomptr)
    {

    }


    void InitItem(Core::ItemInfo iteminfo);

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

    void SetCount(int32 count)
    {
        _iteminfo.count = count;
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
    Core::ItemInfo _iteminfo;
    
    //아이템에서 HP는 일종의 내구도 개념으로 이해 하면 될것
};
