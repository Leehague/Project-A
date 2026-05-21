#pragma once
#include "Types.h"
#include <map>

class Inventory
{
public:
    Inventory();

    ItemPtr FindItem(int32 itemDbId)
    {
        return Items[itemDbId];
    }

    void InsertItem(int32 itemDbId,ItemPtr item)
    {
        if (Items[itemDbId] == nullptr)
        {
            Items[itemDbId] = item;
        }
        
    }
private:

    std::map<int32, ItemPtr> Items; // <ItemDbId, ItemPtr>
};
