#include "Room.h"
#include "Player.h" 
#include "Session.h" 
#include "GameObject.h"


void Room::Enter(GameObjectPtr go)
{
    std::lock_guard<std::mutex> lock(_lock);
    _objects[go->GetObjectId()] = go;
    //player->getroo = shared_from_this();
    //go->SetroomId(_roomid);
}
void Room::Leave(PlayerPtr player)
{

}

void Room::Broadcast(SendBufferPtr sendBuffer)
{
    std::cout << "Room::Broadcast works" << std::endl;

    std::lock_guard<std::mutex> lock(_lock);
    for (auto& pair : _objects) {
        if (pair.second->GetType() != GameObjectType::Player) { continue; }


        PlayerPtr player = std::static_pointer_cast<Player>(pair.second);
        auto session = player->session.lock(); // 세션이 살아있는지 확인
        if (session) {
            session->Send(sendBuffer);
            std::cout << "Broadcast to session " << session->GetGuid() << std::endl;
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
    if (player == nullptr) return;

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
    Broadcast(sendBuffer); // <-- This must be inside the function body
}
