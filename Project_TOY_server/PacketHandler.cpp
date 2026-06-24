#include "PacketHandler.h"
#include "Session.h"
#include "Player.h"
#include "ObjectManager.h"
#include "RoomManager.h"
#include "DataContents.h"
#include "DBManager.h"
#include "Inventory.h"
#include "Item.h"
#include "InfoSturct.h"

// 헤더에 있는 extern 선언과 타입이 정확히 일치해야 합니다.
PacketHandlerFunc GPacketHandler[65535];

bool Handle_INVALID(SessionPtr& session, BYTE* buffer, int32 len)
{
    std::cout << "Handle_INVALID is called" << std::endl;
    return false;
}

bool Handle_CS_LOGIN(SessionPtr& session, Protocol::CS_LOGIN& pkt)
{
    std::cout << "Login request ID: " << pkt.user_id() << std::endl;

    //TODO 로그인 유효성 검증 로직 추가 (로그인 서버 추가후 로그인 서버와의 통신 필요)
    //이때 accountId를 받아 와야함

    int32 accountId = 1; int32 characterId; int32 templateId;
    Core::PosInfo posInfo; // DB에서 위치 정보를 받아올 변수

    PlayerPtr player;
    // TODO: 향후 DBManager::GetCharacterInfo가 위치 정보(posInfo)까지 가져오도록 수정 필요합니다.
    // 현재는 GetCharacterInfo가 false를 반환해도, 위에서 선언한 posInfo가 생성자 덕분에 (0,0,0)으로 안전하게 초기화됩니다.
    if (DBManager::GetInstance().GetCharacterInfo(accountId, characterId, templateId/*, posInfo*/))
    {
        
        GameObjectPtr go = GObjcetManager.Create(GameObjectType::Player, session, templateId,nullptr);
        player = std::static_pointer_cast<Player>(go);
        
        // [수정] DB에서 위치를 가져오지 않더라도, 생성자로 (0,0,0)이 된 posInfo로 위치를 명시적으로 설정합니다.
        // 이렇게 하면 Room::Enter에서 올바른 위치를 사용하게 됩니다.
        player->Setpos(posInfo);

    }
    //참고 : characterId 는 playerDbId와 동일함 둘다 DB에서의 Id임
    // player 객체가 성공적으로 생성되었는지 확인 후 인벤토리 로드
    if (player && DBManager::GetInstance().LoadPlayerInventory(characterId, player))
    {
        // 응답 전송 
        Protocol::SC_LOGIN_OK resPkt;

        resPkt.set_success(true);

        resPkt.set_player_id(player->GetObjectId()); //반드시 로그인 요청을 한 유저의 playerId(objectId)로 답을 해주어야함

        //참고 SC_LOGIN_OK 에서의 player Id는 ObjectManager에서 관리하는 objectId와 동일함

        if (resPkt.success()) { std::cout << "success is true" << std::endl; }
        else { std::cout << "success is not true" << std::endl; }

        auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_LOGIN_OK);

        if (!sendBuffer) { return true; }
        session->Send(sendBuffer);


        return true;
    }

    //여기서 false를 리턴하거나 혹은 true를 리턴하는 대신 에러 대처를 별도로 해야함 일단은 false 리턴
    return false;
}


bool Handle_CS_CHAT(SessionPtr& session, Protocol::CS_CHAT& pkt)
{
    // 1. 받은 패킷에서 데이터 추출 (pkt->chatMsg 대신 pkt.msg() 등 사용)
    // .proto 파일에 정의한 필드명에 따라 함수명이 결정됩니다. (예: string msg -> msg())
    std::string receivedMsg = pkt.msg();

    // 2. 보낼 응답 패킷 생성 및 데이터 채우기
    Protocol::SC_CHAT_BROADCAST res;

    // 플레이어 ID 설정 
    //res.set_player_id(static_cast<uint64>(session->Session::GetSocket()));
    res.set_player_id(session->GetPlayerId());
    // 채팅 내용 설정 (Protobuf가 내부적으로 메모리 할당을 관리하므로 strcpy_s가 필요 없습니다)
    res.set_msg(receivedMsg);

    // 3. ServerUtils를 사용하여 가변 크기용 SendBuffer 생성
    // 내부에서 ByteSizeLong() 호출, 헤더 기입, 직렬화가 모두 처리됩니다.
    SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(res, Protocol::PKT_SC_CHAT_BROADCAST);

    // 4. 브로드캐스팅
    if (sendBuffer)
    {
        RoomPtr room = GRoomManager.FindRoom(session->GetPlayerPtr()->GetroomId());
            
        room->Broadcast(sendBuffer);
    }
    return true;
}


bool Handle_CS_WHISPER(SessionPtr& session, Protocol::CS_WHISPER& pkt)
{
    uint64 targetId = pkt.targetplayer_id(); // Protobuf getter
    std::string msg = pkt.msg();

    // 1. 타겟에게 보낼 패킷 생성 (SC_WHISPER 권장)
    Protocol::SC_WHISPER res;
    res.set_fromplayer_id(session->GetPlayerId()); // 보낸 사람 ID
    res.set_msg(msg);

    // 2. 버퍼 생성
    SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(res, Protocol::PKT_SC_WHISPER);

    if (sendBuffer)
    {
        // 3. 타겟에게 전송
        GSessionManager.SendTo(targetId, sendBuffer);

        // 4. (옵션) 보낸 사람 본인에게도 "전송 성공" 피드백 패킷을 보내면 좋습니다.
        // 또는 그냥 클라가 보낸 메시지를 본인 화면에 바로 띄우도록 설계할 수도 있습니다.
    }
    return true;
}

bool Handle_CS_MOVING(SessionPtr& session, Protocol::CS_MOVING& pkt) 
{
    //세션에서 플레이어 객체 가져오기 (세션에 Player 멤버가 있다고 가정)
    PlayerPtr player = session->GetPlayerPtr();
    if (player == nullptr) 
    { 
        std::cout << "Handle_CS_MOVING: player has nullptr" << std::endl;
        return true; 
    }

    RoomPtr room = GRoomManager.FindRoom(player->GetroomId());
    if (room == nullptr)
    {
        std::cout << "Handle_CS_MOVING: room has nullptr" << std::endl;
        return true;
    }
    room->HandleMove(player,pkt);
    
    return true;
}

bool Handle_CS_ENTER_GAME(SessionPtr& session, Protocol::CS_ENTER_GAME& pkt)
{
    auto player = session->GetPlayerPtr();
    if (player == nullptr) 
    { 
        std::cout << "not found playerPTr in Handle_CS_ENTER_GAME" << std::endl;
        return true;
    }

    // Temp , TODO: FindLastRoom 은 서버 입장에서 마지막으로 만들어진 Room 을 찾아서 입장 시키는 것, 
    // 따라서 해당 클라에 알맞는 룸을 찾아서 입장시키는 로직이 필요함 
    // 이는 기획에 따라 달라질 요소가 있음

    RoomPtr currentRoom = GRoomManager.FindLastRoom(); //즉 여기서 어떤 룸을 선택할지 다른 방식을 도입해야할 수도 있음

    currentRoom->Enter(player); //Enter 로직 속에 전송로직이 포함되어 있으므로 이것으로 충분함

    //수정 : Room ::Enter 에서는 스폰 패킷을 더이상 전송하지 않음.

    return true;
}

bool Handle_CS_GAME_READY(SessionPtr& session, Protocol::CS_GAME_READY& pkt)
{
    //session 에서 들고 있는 playerId(objectId) 와 패킷의 playerId(obejctId)가 일치하는지 확인
    if (session->GetPlayerId() != pkt.player_id()) 
    {
        std::cout << "Handle_CS_GAME_READY: session playerId and packet playerId is not same" << std::endl;
        return true;
    }
    int32 roomid = session->GetPlayerPtr()->GetroomId();
    GRoomManager.FindRoom(roomid)->SpawnBroadcast(session->GetPlayerPtr());
    return true;
}

//클라에서온 스킬사용 요청 핸들러 함수
bool Handle_CS_SKILL(SessionPtr& session, Protocol::CS_SKILL& pkt)
{
    PlayerPtr player = session->GetPlayerPtr();
    if (player == nullptr)
    {
        std::cout << "Handle_CS_SKILL: player has nullptr" << std::endl;
        return true;
    }

    RoomPtr room = GRoomManager.FindRoom(player->GetroomId());
    if (room == nullptr)
    {
        std::cout << "Handle_CS_SKILL: room has nullptr" << std::endl;
        return true;
    }
    room->HandleSkillForPlayer(player, pkt);
    
    return true;
}

bool Handle_CS_OWNED_ITEM_REQUEST(SessionPtr& session, Protocol::CS_OWNED_ITEM_REQUEST& pkt)
{   
    PlayerPtr player = session->GetPlayerPtr();
    if (player == nullptr) return true;

    //요청한 유저가 본인이 맞는지 검증
    if (player->GetObjectId() != pkt.player_id())
    {
        //만약 발급한 각 플레이어(세션)이 가진 obejctID(playerId)를 리프레쉬하는 코드등이 추가되면 여기서 연결을 끊는 것이 아니라
        //클라가 잘못 알고 있음을 확인하고 서버가 알고있는 playerId를 통보하거나 혹은 클라를 확인하는 코드등 추가 작업 필요가능성 있음
        std::cout << "player Id missmatch" << std::endl;
        return true; // 일단은 true 리턴하여 패킷 처리는 했다고 응답, 하지만 실제로는 클라가 잘못된 playerId를 가지고 있음을 알려주는 패킷을 보내거나 하는 추가 작업 필요할 수 있음
    }

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
        std::cout << "itemInfo >> DB id :"<< item->GetItemDBid() << std::endl;
    }

    // 패킷 직렬화 및 전송
    SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_ITEM_RESPONSE);
    if (sendBuffer)
    {
        session->Send(sendBuffer);
    }

    return true;
}
