#include "Room.h"
#include "Player.h" 
#include "Session.h" 
#include "GameObject.h"
#include "Protocol/Protocol.pb.h"
#include <string>
#include "Vector3.h"
#include "MapManager.h"
#include "Map.h"

Room::Room(int32 roomId, int32 mapId) : _Selfroomid(roomId)
{
    // 방이 생성될 때 맵 매니저를 통해 맵을 할당받습니다.
    // GMapManager는 전역 혹은 싱글톤으로 선언되어 있어야 합니다.
    _map = GMapManager.LoadMap(mapId);

    if (_map == nullptr)
    {
        std::cout << "Room " << roomId << ": Map Load Failed! (ID: " << mapId << ")" << std::endl;
    }
}

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

        enterPkt.set_templeteid(go->GetTempleteId()); //핸들러에서 결정된 템플릿 아이디
        
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
              
        Protocol::SpawnInfo* spawnInfo = spawnPkt.add_players_spawn_info();

        spawnInfo->mutable_spawnposinfo()->CopyFrom(player->Getpos());
        spawnInfo->set_templeteid(player->GetTempleteId());


        if (spawnPkt.players_spawn_info_size() > 0) // 데이터가 있을 때만 전송
        {
            
            SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(spawnPkt, Protocol::PKT_SC_PLAYER_SPAWN);

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
            
            Protocol::SpawnInfo* spawnInfo = spawnPkt.add_players_spawn_info();
                       
            // 2. 해당 슬롯에 기존 오브젝트의 위치 정보를 복사 
            spawnInfo->mutable_spawnposinfo()->CopyFrom(pair.second->Getpos());
            spawnInfo->set_templeteid(pair.second->GetTempleteId());
            


        }

        
        if (spawnPkt.players_spawn_info_size() > 0) // 데이터가 있을 때만 전송
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
    Vector3 currentPos = Vector3::PosInfoToVector3(player->Getpos());
    Vector3 newPos = Vector3(pkt.pos_info().x(), pkt.pos_info().y(), pkt.pos_info().z());

    uint64 currentTick = ::GetTickCount64(); // 현재 서버 시간 (Windows 기준)

    // 처음에만 0일 수 있으므로 예외 처리
    if (player->lastMoveTick == 0) player->lastMoveTick = currentTick - 100;

    // deltaTime 계산 (초 단위로 변환)
    float deltaTime = (currentTick - player->lastMoveTick) / 1000.0f;
    player->lastMoveTick = currentTick; // 현재 시간을 다음 검증을 위해 저장


    float dist = Vector3::Distance(currentPos, newPos);
    float maxAllowedDist = player->GetSpeed() * deltaTime * 1.2f; // 오차범위 20%

    //속도 검증
    if (dist > maxAllowedDist) {
        // 너무 멀리 이동함 (패킷 무시 혹은 강제 위치 복구)
        std::cout << "비정상 이동 요청 인식" << std::endl;

        SendMoveResync(player);
        return;
    }
    
    // 지형 검증 
    if (_map != nullptr)
    {
        if (_map->CanGo(newPos) == false)
        {
            // 충돌 발생! 클라이언트에게 강제 위치 복구 패킷 전송
            SendMoveResync(player);
            return;
        }
    }

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

void Room::HandleSkill(PlayerPtr player, Protocol::CS_SKILL& pkt)
{
    //TODO: [핵 방지]_skillCooltimes 를 이용하던 아니면 다른 메모리영역을 추가하던 해서 스킬이 진짜 그 캐릭터가 쓸수 있는 스킬인지 체크하는 로직필요

    const SkillData* skilldata = DataManager::GetInstance().GetSkill(pkt.skill_id());
    int64 now = GetTickCount64();
    int64 lastUsed = player->_skillCooltimes[skilldata->id];
    int64 coolTime = skilldata->coolTime * 1000; // 초 단위를 ms로 변환

    if (now - lastUsed < coolTime) {
        // 아직 쿨타임 중! 요청 무시 혹은 에러 패킷 전송
        return;
    }

    // 검증 통과 후 사용 시점 갱신
    player->_skillCooltimes[skilldata->id] = now;

    // 3. 코스트(마나 등) 체크 및 차감
    // (Player 클래스에 GetStat(), SetStat() 혹은 직접 접근 가능한 멤버가 있다고 가정)
    if (skilldata->costType == CostType::Mana) {
        int32 currentMp = player->GetCurrentMp(); // 플레이어 현재 MP 가져오기
        int32 requiredMp = skilldata->cost; // 예시: 스킬 데이터에 코스트 수치를 추가하면 더 좋습니다.

        
        if (player->UseMp(currentMp - requiredMp) == false) 
        {
            //마나 부족, 본인에게 메시지등을 보낼수 있을것임
            return;
        }

        //본인에게 MP 변경 패킷 전송 
        Protocol::SC_CHANGE_MP mp_Change_pkt;
        mp_Change_pkt.set_object_id ( player->GetObjectId());
        mp_Change_pkt.set_current_mp(player->GetCurrentMp());
        auto sendBuffer = ServerUtils::MakeSendBuffer(mp_Change_pkt, Protocol::PKT_SC_CHANGE_MP);

        if (auto s = player->session.lock())
            s->Send(sendBuffer);
    }

    // 4. 검증 통과 후 사용 시점 갱신
    player->_skillCooltimes[skilldata->id] = now;

    // 5. 스킬 타입별 피격 판정
    bool isHit = false;

    switch (skilldata->skillType)
    {
    case SkillType::Melee:
    {
        //distance 체크 , 그리드 방식으로 수정예정
        for (auto obj : _objects) 
        {
            GameObjectPtr target = obj.second;
            if (target && target->GetObjectId() != player->GetObjectId()) {
                // 거리 계산 (Vector3::Distance)
                float dist = Vector3::Distance(Vector3::PosInfoToVector3(player->Getpos()), Vector3::PosInfoToVector3(target->Getpos()));



                // 사거리 검증 (약간의 마진 부여: 0.5f)
                if (dist <= skilldata->range + 0.5f) {
                    isHit = true;

                    // 데미지 계산 및 적용
                    int32 damage = skilldata->damage + player->GetAttack(); // 스킬데미지 + 캐릭터공격력 수정 가능
                    target->OnAttacked(player, damage); // 대상의 HP를 깎는 함수 호출

                    //Hp 변화 방송
                    Protocol::SC_CHANGE_HP hp_changed_pkt;
                    hp_changed_pkt.set_object_id(target->GetObjectId());
                    hp_changed_pkt.set_current_hp(target->GetCurrentHp());
                    hp_changed_pkt.set_damage(damage);
                    hp_changed_pkt.set_attacker_id(player->GetObjectId());
                    auto sendBuffer = ServerUtils::MakeSendBuffer(hp_changed_pkt, Protocol::PKT_SC_CHANGE_HP);
                    Broadcast(sendBuffer);

                    std::cout << "[Melee Hit] " << player->GetName() << " -> " << target->GetName() << " (Damage: " << damage << ")" << std::endl;
                }
            }
        }

        // 클라이언트가 보낸 target_id로 대상 오브젝트 찾기( 타게팅용, 기획 변경으로 삭제)
        //GameObjectPtr target = (_objects.find(pkt.target().target_object_id()) != _objects.end()) ? _objects[pkt.target().target_object_id()] : nullptr;

    }
    break;

    case SkillType::Projectile:
        // 투사체는 즉시 피격이 아니라 Projectile 객체를 생성하여 Update에서 처리
        // SpawnProjectile(player, skilldata, pkt.target_pos());
        break;

    case SkillType::Dash:
        // 이동 가능 지역인지 확인 후 좌표 강제 갱신
        break;
    }

    // 6. 결과 브로드캐스트 (주변 모두에게 애니메이션 알림)
    Protocol::SC_SKILL resPkt;
    resPkt.set_object_id(player->GetObjectId());
    resPkt.set_skill_id(skilldata->id);
    
    if (pkt.has_target())
    {
        // mutable_target()을 통해 TargetobjectInfo 객체의 포인터를 얻어 값 설정
        resPkt.mutable_target()->set_target_object_id(pkt.target().target_object_id());
    }
    else if (pkt.has_dest_pos())
    {
        resPkt.mutable_dest_pos()->CopyFrom(pkt.dest_pos());
    }

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_SKILL);
    Broadcast(sendBuffer);
}

//위치 되감기
void Room::SendMoveResync(PlayerPtr player)
{
    // 1. 서버에 저장된 '이전' 좌표를 담은 패킷 생성
    Protocol::SC_MOVING resPkt;
    auto* resPos = resPkt.mutable_pos_info();
    resPos->CopyFrom(player->Getpos()); // 업데이트 전의 서버 좌표

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_MOVING);

    // 2. 해당 유저에게만 강제로 전송 (위치 되감기)
    if (auto s = player->session.lock())
        s->Send(sendBuffer);

}


