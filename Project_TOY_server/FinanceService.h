#pragma once
#include "Types.h"
#include "JobQueue.h"


// 재화 획득/소모 사유 (로그 추적용)
enum class ItemChangeReason
{
    QuestReward,   // 퀘스트 보상
    MonsterLoot,   // 루팅
    ShopBuy,       // 상점 구매
    ShopSell,      // 상점 판매
    PlayerTrade,   // 유저 간 거래
    AdminCommand   // 운영자 툴
};

// 트랜잭션 결과
enum class FinanceResult
{
    Success,
    NotEnoughGold,
    InventoryFull,
    ItemNotFound,
    CreatureNotFound,
    DbError,
    UnkonwnCase,
    InventoryNotFound,
    ItemPointerErr
};


class FinanceService 
{
public:
    //단일 골드 변경 (획득)
    static FinanceResult AcquireGold(CreaturePtr creature, int64 amount, ItemChangeReason reason);

    //단일 골드 변경 (소모)
    static FinanceResult UseGold(CreaturePtr creature, int64 amount, ItemChangeReason reason);

    //단일 아이템 획득
    static FinanceResult AcquireItem(CreaturePtr creature,int32 item_obejctid, int32 count, ItemChangeReason reason);

   

    static void SendInventoryUpdatePacket(CreaturePtr creature);
private:
    static void WriteAuditLog(CreaturePtr creature, ItemChangeReason reason, int64 goldChange, int32 itemId, int32 count);
    
};
