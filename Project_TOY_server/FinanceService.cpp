#include "FinanceService.h"
#include "Creature.h"
#include "ObjectManager.h"
#include "Inventory.h"
#include "Item.h"
#include "RoomManager.h"
#include "Room.h"
#include "ServerUtils.h"
#include "Player.h"
#include "Session.h"

FinanceResult FinanceService::AcquireGold(CreaturePtr creature, int64 amount, ItemChangeReason reason)
{
    int currentgold= creature->GetOwnedGold();
    currentgold += amount;
    creature->SetOwnedGold(currentgold);

    return FinanceResult::Success;
}

FinanceResult FinanceService::UseGold(CreaturePtr creature, int64 amount, ItemChangeReason reason)
{
    if (creature == nullptr)
    {
        return FinanceResult::CreatureNotFound;
    }

    int currentgold = creature->GetOwnedGold();
    if (currentgold < amount)
    {
        return FinanceResult::NotEnoughGold;
    }
    currentgold -= amount;

    creature->SetOwnedGold(currentgold);
    return FinanceResult::Success;
}

FinanceResult FinanceService::AcquireItem(CreaturePtr creature, int32 item_obejctid, int32 count, ItemChangeReason reason)
{
    if (creature == nullptr)
    {
        return FinanceResult::CreatureNotFound;
    }

    GameObjectPtr go = GObjcetManager.Find(item_obejctid);
    if (go == nullptr || go->GetType() != GameObjectType::Item)
    {
        return FinanceResult::ItemNotFound;
    }
    ItemPtr item = std::dynamic_pointer_cast<Item>(go);

    if (item == nullptr)
    {
        return FinanceResult::ItemPointerErr;
    }

    //알맞은 reason 마다 처리
    if (reason == ItemChangeReason::QuestReward)
    {

        InventoryPtr inven = creature->GetInventory();

        if (inven == nullptr)
        {
            return FinanceResult::InventoryNotFound;
        }

        if (inven->IsInventoryNotFull() == false)
        {
            return FinanceResult::InventoryFull;
        }
        inven->InsertItem(item->GetItemDBid(), item);

        SendInventoryUpdatePacket(creature);

        return FinanceResult::Success;
    }

    return FinanceResult::UnkonwnCase;
}



void FinanceService::WriteAuditLog(CreaturePtr creature, ItemChangeReason reason, int64 goldChange, int32 itemId, int32 count)
{
    //TODO Logger 클래스 필요
}

void FinanceService::SendInventoryUpdatePacket(CreaturePtr creature)
{
    if (creature->GetType() != GameObjectType::Player)
    {
        return; //플레이어가 아니면 리턴
    }

    PlayerPtr player = std::dynamic_pointer_cast<Player>(creature);

    if (player == nullptr)
    {
        return;
    }

    
    int32 roomid = creature->GetroomId();

    RoomPtr room = GRoomManager.FindRoom(roomid);

    room->Push([player]() {
        Protocol::SC_ITEM_RESPONSE resPkt;
        //인벤토리에서 전체 아이템 정보 가져오기
        InventoryPtr inventory = player->GetInventory();
        const auto& allItems = inventory->GetAllItems();

        //여기서 아아템의 갯수와 구조가 복잡해지고 많아지면 그 구조에 따라 최적화가 필요할 수도 있음

        for (const auto& pair : allItems)
        {
            ItemPtr item = pair.second;
            if (item == nullptr) continue;

            Protocol::ItemInfo* itemInfo = resPkt.add_items();

            itemInfo->set_dbid(item->GetItemDBid());
            itemInfo->set_templateid(item->GetTemplateId());
            itemInfo->set_count(item->GetCount());
            itemInfo->set_slot(item->GetSlot());
            itemInfo->set_item_memo(item->GetMemo());


            //temp log code
            std::cout << "itemInfo >> DB id :" << item->GetItemDBid() << std::endl;
        }

        // 패킷 직렬화 및 전송
        SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_ITEM_RESPONSE);
        if (auto session = player->session.lock())
        {
            if (sendBuffer)
            {
                session->Send(sendBuffer);
            }
        }

        });

    
}
