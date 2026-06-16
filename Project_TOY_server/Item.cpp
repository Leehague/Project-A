#include "Item.h"


void Item::InitItem(Core::ItemInfo iteminfo)
{
    _iteminfo.itemDbId = iteminfo.itemDbId;
    _iteminfo.itemTemplateId = iteminfo.itemTemplateId;
    
    _iteminfo.count = iteminfo.count;
    _iteminfo.slot = iteminfo.slot;
    _iteminfo.itemMemo = iteminfo.itemMemo; 
}
