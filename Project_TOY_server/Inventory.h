#pragma once
#include "Types.h"
#include <map>
#include <mutex>

class Inventory
{
public:
    Inventory()
    {
        
    }

    ItemPtr FindItem(int32 itemDbId)
    {
        std::lock_guard<std::mutex> lock(_lock);

        auto it = Items.find(itemDbId);
        if (it != Items.end())
            return it->second;
            
        return nullptr; // 아이템을 못 찾으면 nullptr 반환
    }

    void InsertItem(int32 itemDbId,ItemPtr item)
    {
        std::lock_guard<std::mutex> lock(_lock);
        Items.insert({ itemDbId ,item }); //같은 DBid를 가진 item이 오면 무시하도록
    }

    // 전체 아이템 목록 반환 (멀티스레드 안전성을 위해 복사본 반환)
    std::map<int32, ItemPtr> GetAllItems() const
    {
        std::lock_guard<std::mutex> lock(_lock);
        return Items;
    }

private:
    // const 함수(GetAllItems)에서도 락을 걸 수 있도록 mutable 사용
    mutable std::mutex _lock;

    std::map<int32, ItemPtr> Items; // <ItemDbId, ItemPtr>
};
