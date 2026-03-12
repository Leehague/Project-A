#include "Room.h"
#include "Player.h" 
#include "Session.h" 

void Room::Enter(PlayerPtr player)
{
    std::lock_guard<std::mutex> lock(_lock);
    _players[player->playerId] = player;
    player->room = shared_from_this();
}
void Room::Leave(PlayerPtr player)
{

}

void Room::Broadcast(SendBufferPtr sendBuffer)
{
    std::lock_guard<std::mutex> lock(_lock);
    for (auto& pair : _players) {
        PlayerPtr player = pair.second;
        auto session = player->session.lock(); // 세션이 살아있는지 확인
        if (session) {
            session->Send(sendBuffer);
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
    // float distance = CalculateDistance(player->posInfo, pkt.pos_info());
    // if (distance > MAX_MOVE_SPEED) return;

    // 2. [갱신] 서버 메모리에 플레이어 위치 정보 업데이트
    player->posInfo.CopyFrom(pkt.pos_info());

    // 3. [전달] 방 안의 다른 유저들에게 이동 사실 브로드캐스트
    Protocol::SC_MOVING resPkt;
    auto* resPos = resPkt.mutable_pos_info();
    resPos->CopyFrom(player->posInfo);

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_MOVING);
    Broadcast(sendBuffer); // <-- This must be inside the function body
}
