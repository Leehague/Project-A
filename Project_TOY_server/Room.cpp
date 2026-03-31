#include "Room.h"
#include "Player.h" 
#include "Session.h" 
#include "GameObject.h"
#include "Protocol/Protocol.pb.h"
#include <string>

void Room::Enter(GameObjectPtr go)
{
    //서버 내부 메모리에 정보 저장(방 입장 처리)
    {
        std::lock_guard<std::mutex> lock(_lock);
        _objects[go->GetObjectId()] = go;
        go->SetroomId(_Selfroomid);

        
    }
    //본인에게 입장 성공 및 좌표 알림 (SC_ENTER_GAME)
    if (auto player = std::static_pointer_cast<Player>(go))
    {       
        
        if (auto session = player->session.lock())
        {
            session->SetPlayerId(player->GetObjectId());
        }
        Protocol::SC_ENTER_GAME enterPkt;
        *enterPkt.mutable_pos_info() = player->Getpos(); // 서버가 결정한 좌표
        auto sendBuffer = ServerUtils::MakeSendBuffer(enterPkt, Protocol::PKT_SC_ENTER_GAME);
        player->session.lock()->Send(sendBuffer);
    }

    // 수정 : 스폰 패킷은 Room::Enter 에서 전송하지 않고 나중에 레디 패킷을 수신해서 전송함
    
}
void Room::Leave(PlayerPtr player)
{
    uint64 playerId = player->GetObjectId();

    {
        std::lock_guard<std::mutex> lock(_lock);
        _objects.erase(playerId); // 1. 룸의 관리 목록에서 제거
    }

    // 2. 타인들에게 이 유저가 나갔음을 알림 (SC_DESPAWN)
    Protocol::SC_DESPAWN despawnPkt;
    despawnPkt.add_player_id(playerId);
    

    auto sendBuffer = ServerUtils::MakeSendBuffer(despawnPkt, Protocol::PKT_SC_DESPAWN);
    Broadcast(sendBuffer, playerId); // 본인은 이미 나갔으므로 제외
}

void Room::Broadcast(SendBufferPtr sendBuffer)
{
    Broadcast(sendBuffer, -1);
}
void Room::Broadcast(SendBufferPtr sendBuffer, int32 passing_object_id)
{
    //std::cout << "Room::Broadcast works" << std::endl;

    std::lock_guard<std::mutex> lock(_lock);
    for (auto& pair : _objects) {
        if (pair.second->GetType() != GameObjectType::Player) { continue; }

        PlayerPtr player = std::static_pointer_cast<Player>(pair.second);

        if (player->GetObjectId() == passing_object_id) { continue; }
        auto session = player->session.lock(); // 세션이 살아있는지 확인
        if (session) {
            session->Send(sendBuffer);
            std::cout << "Broadcast to session " << session->GetGuid() << std::endl;
        }
    }
}
void Room::SpawnBroadcast(PlayerPtr player)
{
    //(player 기준)타인들에게 나를 알림 (SC_PLAYER_SPAWN 브로드캐스트)
    {
        Protocol::SC_PLAYER_SPAWN spawnPkt;

        // With this code:
        auto* obj = spawnPkt.add_players_pos_info();
        obj->CopyFrom(player->Getpos()); // 내 정보 추가


        if (spawnPkt.players_pos_info_size() > 0) // 데이터가 있을 때만 전송
        {
            auto sendBuffer = ServerUtils::MakeSendBuffer(spawnPkt, Protocol::PKT_SC_PLAYER_SPAWN);

            // 나를 제외한 모두에게 전송 (기존에 만든 Broadcast 함수 활용)
            Broadcast(sendBuffer, player->GetObjectId());
        }

    }

    //(player 기준)나에게 기존 오브젝트들을 알림 (SC_PLAYER_SPAWN 목록 전송)
    {
        Protocol::SC_PLAYER_SPAWN spawnPkt;

        // 방의 모든 오브젝트를 순회하며 나를 제외한 정보를 패킷에 추가
        for (auto& pair : _objects)
        {
            
            // 1. 새로운 PosInfo 슬롯을 리스트에 추가하고 그 주소를 가져옴
            Protocol::PosInfo* info = spawnPkt.add_players_pos_info();

            // 2. 해당 슬롯에 기존 오브젝트의 위치 정보를 복사 
            info->CopyFrom(pair.second->Getpos());

            std::cout << "other objectId: " << pair.second->Getpos().object_id()
                <<"templeteId: "<< pair.second->Getpos().templeteid() << std::endl;


        }

        

        if (spawnPkt.players_pos_info_size() > 0) // 데이터가 있을 때만 전송
        {
            // 패킷 시리얼라이즈 및 전송
            auto sendBuffer = ServerUtils::MakeSendBuffer(spawnPkt, Protocol::PKT_SC_PLAYER_SPAWN);

            if (auto s = player->session.lock())
            {
                /*std::cout << "[Packet Log] ID: " << Protocol::PKT_SC_PLAYER_SPAWN
                    << " | Total Size: " << sendBuffer->WriteSize() << " Bytes" << std::endl;*/
                s->Send(sendBuffer);
            }
        }
    }

}
void Room::SendTo(PlayerPtr player, SendBufferPtr sendBuffer)
{
    std::lock_guard<std::mutex> lock(_lock);

    // Get the session from the player (std::weak_ptr<Session>)
    auto session = player->session.lock(); // Convert weak_ptr to shared_ptr
    if (session)
    {
        session->Send(sendBuffer);
    }
    // else: session is expired, do nothing or handle error as needed
}
void Room::HandleMove(PlayerPtr player ,Protocol::CS_MOVING& pkt)
{
    if (player == nullptr) 
    { 
        std::cout << "HandleMove : player ptr is null" << std::endl; 
        return;
    }

    // 1. [검증] 이전 위치와 새 위치의 거리 차이가 너무 크면 무시하거나 보정 (핵 방지)
    //TODO: 검증 로직 추가 필요 , 핵 방지 , 최적화 등
    
    // float distance = CalculateDistance(player->posInfo, pkt.pos_info());
    // if (distance > MAX_MOVE_SPEED) return;

    // 2. [갱신] 서버 메모리에 플레이어 위치 정보 업데이트
    player->Setpos(pkt.pos_info());

    // 3. [전달] 방 안의 다른 유저들에게 이동 사실 브로드캐스트
    Protocol::SC_MOVING resPkt;
    auto* resPos = resPkt.mutable_pos_info();
    resPos->CopyFrom(player->Getpos());

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_MOVING);
    Broadcast(sendBuffer, player->GetObjectId()); // 자기자신은 제외


    //Loging
    std::cout << "RoomId: " << _Selfroomid << std::endl
        << "object Id : " << resPos->object_id() << "HandleMove : (" << resPos->x() << resPos->y() << resPos->z() << ")" << std::endl;

}
