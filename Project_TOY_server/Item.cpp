#include "Item.h"


void Item::InitItem(int32 itemDbId, int32 itemTemplteId, int32 count, int32 slot)
{
    _itemDbId = itemDbId;
    _itemTemplteId = itemTemplteId;
    _count = count;
    _slot = slot;
}
