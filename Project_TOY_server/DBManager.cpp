// DBManager.cpp
#include "DBManager.h"
#include <iostream>
#include "DataContents.h"
#include "DataManager.h"
#include "Player.h"
#include "Inventory.h"
#include "Item.h"
#include "ObjectManager.h"


bool DBManager::Init(int poolSize, const std::wstring& connStr) {
    // 1. ODBC 환경 핸들 생성
    if (SQL_SUCCESS != ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv))
        return false;

    if (SQL_SUCCESS != ::SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0))
        return false;

    // 2. 연결 풀 생성
    for (int i = 0; i < poolSize; i++) {
        DBConnection* conn = new DBConnection();
        if (conn->Connect(_hEnv, connStr)) {
            _pool.push_back(conn);
            _freePool.push(conn);
        }
        else {
            delete conn;
            return false;
        }
    }

    // 3. DB 전용 쓰레드 실행 (보통 1~2개면 충분합니다)
    for (int i = 0; i < 1; i++) {
        _threads.push_back(std::thread(&DBManager::DBThreadMain, this));
    }

    return true;
}

void DBManager::DBThreadMain() {
    while (true) {
        JobFn job = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _condVar.wait(lock, [this] { return !_dbJobs.empty(); });

            job = std::move(_dbJobs.front());
            _dbJobs.pop();
        }

        if (job) job(); // Push된 람다/함수 실행
    }
}

void DBManager::Push(JobFn&& job) {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _dbJobs.push(std::move(job));
    }
    _condVar.notify_one();
}

DBConnection* DBManager::PopConnection() {

    std::lock_guard<std::mutex> lock(_mutex);
    if (_freePool.empty()) return nullptr;
    DBConnection* conn = _freePool.front();
    _freePool.pop();
    return conn;
}

void DBManager::PushConnection(DBConnection* conn) {
    if (conn) conn->FreeStmt(); // 재사용 시 충돌을 막기 위해 구문 핸들 초기화
    std::lock_guard<std::mutex> lock(_mutex);
    _freePool.push(conn);
}

bool DBManager::LoadPlayerInventory(int32 playerDBId, PlayerPtr player)
{
    
    // 1. DB 커넥션 풀에서 남는 커넥션을 하나 가져옵니다.
    DBConnection* conn = PopConnection();
    if (conn == nullptr)
    {
        std::cerr << "Failed to get DB connection from pool!" << std::endl;
        return false;
    }

    // 2. 파라미터 바인딩: 쿼리의 '?' 자리에 playerId를 넣습니다.
    // (SQL Injection 해킹을 방지하기 위해 Prepared Statement 방식을 사용해야 합니다)
    conn->BindParam(1, playerDBId);

    // 3. 쿼리 실행 (미리 만들어둔 비클러스터형 인덱스 IX_Inventory_PlayerId를 타게 됨)
    const WCHAR* query = L"SELECT ItemDbId, TemplateId, Count, Slot FROM [dbo].[Inventory] WHERE PlayerId = ?";

    if (conn->Execute(query))
    {
        // 4. Fetch 루프: 결과가 안 나올 때까지 한 줄(Row)씩 읽어옵니다.
        while (conn->Fetch())
        {
            Core::ItemInfo curuentiteminfo;
            // 컬럼(Column) 인덱스나 이름으로 데이터를 추출
            curuentiteminfo.itemDbId = conn->GetInt(L"ItemDbId");
            curuentiteminfo.itemTemplateId = conn->GetInt(L"TemplateId");
            curuentiteminfo.count = conn->GetInt(L"Count");
            curuentiteminfo.slot = conn->GetInt(L"Slot");
            curuentiteminfo.itemMemo = conn->GetString(L"ItemMemo");

            
            // 5. DataManager를 이용해 해당 아이템이 실제로 존재하는 유효한 아이템인지 메타데이터 검증
            const ItemData* itemData = DataManager::GetInstance().GetItem(curuentiteminfo.itemTemplateId);
            if (itemData != nullptr)
            {
                // 6. 플레이어 객체 내부의 인벤토리(메모리)에 아이템 등록
                CoreRoomPtr playercoreroom = player->GetCoreroomptr();
                ItemPtr item = std::static_pointer_cast<Item>(GObjcetManager.Create(GameObjectType::Item, player->session.lock(), curuentiteminfo.itemTemplateId, playercoreroom));
                item->InitItem(curuentiteminfo);
                player->GetInventory()->InsertItem(curuentiteminfo.itemDbId, item);

                std::cout << "[Inventory Loaded] Item: " << itemData->name
                    << " (Count: " << curuentiteminfo.count << " / Slot: " << curuentiteminfo.slot << ")" << std::endl;
            }
            else
            {
                // DB에는 있는데 json 데이터에는 없는 이상한 아이템인 경우 (로그 남기기)
                std::cerr << "[Warning] Unknown TemplateId in DB: " << curuentiteminfo.itemTemplateId << std::endl;
            }
        }
    }
    else
    {
        std::cerr << "Failed to execute inventory select query!" << std::endl;
    }

    // 7. 사용이 끝난 커넥션을 다시 풀에 반납
    PushConnection(conn);
    return true;
    
}

bool DBManager::GetCharacterInfo(int32 accountId, int32& outCharacterId, int32& outTemplateId)
{
    DBConnection* conn = PopConnection();
    if (conn == nullptr) return false;

    // 1. 파라미터 바인딩: '?' 자리에 accountId 삽입
    conn->BindParam(1, accountId);

    // 2. 쿼리 실행 
    const WCHAR* query = L"SELECT CharacterId, TemplateId FROM [dbo].[Characters] WHERE AccountId = ?";
    
    bool success = false;
    if (conn->Execute(query))
    {
        // 3. 결과 패치: 해당 계정의 캐릭터가 존재하면 데이터 추출
        if (conn->Fetch()) 
        {
            outCharacterId = conn->GetInt(L"CharacterId");
            outTemplateId = conn->GetInt(L"TemplateId");
            success = true;
        }
    }

    PushConnection(conn);
    return success;
}
