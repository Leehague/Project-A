#include "Item.h"


void Item::InitItem(ItemInfo iteminfo)
{
    _iteminfo.itemDbId = iteminfo.itemDbId;
    _iteminfo.itemTemplateId = iteminfo.itemTemplateId;
    
    _iteminfo.count = iteminfo.count;
    _iteminfo.slot = iteminfo.slot;
    _iteminfo.itemMemo = iteminfo.itemMemo; //에러가 여기서 나고 있음... 왜지?
}
