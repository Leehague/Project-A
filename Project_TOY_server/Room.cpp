#include "Room.h"
#include "Player.h" 
#include "Session.h" 
#include "GameObject.h"
#include "Protocol/Protocol.pb.h"
#include <string>
#include "Vector3.h"
#include "MapManager.h"
#include "Map.h"
#include "Monster.h"
#include "Projectile.h"
#include "DataContents.h"
#include "ObjectManager.h"
#include "JobSerializer.h"


Room::Room(int32 roomId, int32 mapId) :JobQueue(&GJobSerializer) , _Selfroomid(roomId)
{
    // 방이 생성될 때 맵 매니저를 통해 맵을 할당받습니다.
    // GMapManager는 전역 혹은 싱글톤으로 선언되어 있어야 합니다.
    _map = GMapManager.LoadMap(mapId);

    if (_map == nullptr)
    {
        std::cout << "Room " << roomId << ": Map Load Failed! (ID: " << mapId << ")" << std::endl;
        return;
    }
    InitGridData(_map->GetMapData());
}

void Room::Enter(GameObjectPtr go)
{
    //서버 내부 메모리에 정보 저장(방 입장 처리)
    {
        std::lock_guard<std::mutex> lock(_lock);
        _objects[go->GetObjectId()] = go;
        go->SetroomId(_Selfroomid);

        auto [cellX, cellZ] = GetSectorPos(Vector3::PosInfoToVector3(go->Getpos())); // 좌표로 인덱스 추출
        _sectors[cellZ][cellX].insert(go);

    }
    //본인에게 입장 성공 및 좌표 알림 (SC_ENTER_GAME)
    if (go->GetType() == GameObjectType::Player )
    {       
        auto player = std::static_pointer_cast<Player>(go);
        if (player==nullptr) { return; }
        if (auto session = player->session.lock())
        {
            session->SetPlayerId(player->GetObjectId());
            session->SetPlayerPtr(player);
        }
        Protocol::SC_ENTER_GAME enterPkt;     
        *enterPkt.mutable_pos_info() = *(player->Getpos()); // 서버가 결정한 좌표

        enterPkt.set_templeteid(go->GetTempleteId()); //핸들러에서 결정된 템플릿 아이디
        
        enterPkt.set_mapid(_map->GetMapId()); //클라에 보내줄 맵 Id
        auto sendBuffer = ServerUtils::MakeSendBuffer(enterPkt, Protocol::PKT_SC_ENTER_GAME);
        
        if (!sendBuffer) return;
        
        player->session.lock()->Send(sendBuffer);
    }
    else if (go->GetType() == GameObjectType::Monster)
    {
        MonsterPtr monster= std::static_pointer_cast<Monster>(go);
        SpawnBroadcast(monster);
    }
    // 수정 : 스폰 패킷은 Room::Enter 에서 전송하지 않고 나중에 레디 패킷을 수신해서 전송함
    
}

void Room::EnterMonsters(const std::vector<MonsterPtr>& monsters)
{
    //서버 내부 메모리에 정보 저장(방 입장 처리)
    {
        for (MonsterPtr monster : monsters) 
        {
            std::lock_guard<std::mutex> lock(_lock);
            _objects[monster->GetObjectId()] = monster;
            monster->SetroomId(_Selfroomid);
        }
    }
    SpawnBroadcast(monsters);
}

void Room::Leave(PlayerPtr player)
{
    if (player == nullptr) return;

    uint64 playerId = player->GetObjectId();

    {
        std::lock_guard<std::mutex> lock(_lock);
        
        auto it = _objects.find(playerId);
        if (it == _objects.end())
            return; // 이미 나갔거나 없는 객체면 무시

        GameObjectPtr go = it->second;

        // 그리드에서 제거
        auto [cellX, cellZ] = GetSectorPos(Vector3::PosInfoToVector3(go->Getpos()));
        _sectors[cellZ][cellX].erase(go);

        _objects.erase(playerId); // 1. 룸의 관리 목록에서 제거
    }

    // 2. 타인들에게 이 유저가 나갔음을 알림 (SC_DESPAWN)
    Protocol::SC_PLAYER_DESPAWN despawnPkt;
    despawnPkt.add_player_id(playerId);
    

    auto sendBuffer = ServerUtils::MakeSendBuffer(despawnPkt, Protocol::PKT_SC_PLAYER_DESPAWN);
    if (!sendBuffer) return;
    
    Broadcast(sendBuffer, playerId); // 본인은 이미 나갔으므로 제외


}

void Room::Broadcast(SendBufferPtr sendBuffer)
{
    Broadcast(sendBuffer, -1);
}
void Room::Broadcast(SendBufferPtr sendBuffer, int32 passing_object_id)
{
    // Snapshot target sessions under lock to avoid data-race / iterator invalidation
    std::vector<std::shared_ptr<Session>> targets;
    {
        std::lock_guard<std::mutex> lock(_lock);
        for (auto& pair : _objects) {
            if (pair.second->GetType() != GameObjectType::Player) continue;
            auto player = std::static_pointer_cast<Player>(pair.second);
            if (player->GetObjectId() == passing_object_id) continue;
            if (auto session = player->session.lock()) {
                targets.push_back(session);
            }
        }
    }
    Broadcast(sendBuffer,targets);//target이 있는 버전의 Braodcast를 호출함
    
}
void Room::Broadcast(SendBufferPtr sendBuffer, std::vector<std::shared_ptr<Session>> targets)
{
    // Buffer sanity check
    if (!sendBuffer || sendBuffer->Size() == 0) {
        std::cerr << "[ERROR] Broadcast: Invalid SendBuffer" << std::endl;
        return;
    }

    if (targets.empty()) {
        //std::cout << "[DEBUG] Broadcast: No targets" << std::endl;
        return;
    }
    uint32 bufferSize = sendBuffer->Size();

    // self 캡처 제거 (내부에서 쓰지 않음)
    this->Push([sendBuffer, targets, bufferSize]() {
        //std::cout << "[Broadcast Job] Sending to " << targets.size()
        //    << " targets, buffer size: " << bufferSize<< " Now, sendBufferSize:  "<< sendBuffer->Size() << std::endl;

        for (auto& session : targets) {
            if (!session) continue;
            try {
                session->Send(sendBuffer);
            }
            catch (const std::exception& e) {
                std::cerr << "[ERROR] Broadcast send failed: " << e.what() << std::endl;
            }
        }
        });

}
void Room::SpawnBroadcast(PlayerPtr player)
{
    //(player 기준)타인들에게 나를 알림 (SC_PLAYER_SPAWN 브로드캐스트)
    {
        Protocol::SC_PLAYER_SPAWN spawnPkt;
              
        Protocol::SpawnInfo* spawnInfo = spawnPkt.add_players_spawn_info();

        spawnInfo->mutable_spawnposinfo()->CopyFrom(*player->Getpos());
        spawnInfo->set_templeteid(player->GetTempleteId());


        if (spawnPkt.players_spawn_info_size() > 0) // 데이터가 있을 때만 전송
        {
            
            SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(spawnPkt, Protocol::PKT_SC_PLAYER_SPAWN);

            if (!sendBuffer) return;

            // 나를 제외한 모두에게 전송 (기존에 만든 Broadcast 함수 활용)
            Broadcast(sendBuffer, player->GetObjectId());
        }

    }

    //(player 기준)나에게 기존 오브젝트들을 알림 (SC_PLAYER_SPAWN 목록 전송)
    {
        Protocol::SC_PLAYER_SPAWN playerspawnPkt;
        Protocol::SC_MONSTER_SPAWN monsterspawnPkt;

        // Snapshot _objects under lock
        std::vector<GameObjectPtr> snapshot;
        {
            std::lock_guard<std::mutex> lock(_lock);
            snapshot.reserve(_objects.size());
            for (auto& pair : _objects)
                snapshot.push_back(pair.second);
        }

        for (auto& obj : snapshot)
        {
            if (!obj) continue;
            if (obj->GetType() == GameObjectType::Player)
            {
                Protocol::SpawnInfo* spawnInfo = playerspawnPkt.add_players_spawn_info();
                spawnInfo->mutable_spawnposinfo()->CopyFrom(*(obj->Getpos()));
                spawnInfo->set_templeteid(obj->GetTempleteId());
            }
            else if (obj->GetType() == GameObjectType::Monster)
            {
                Protocol::SpawnInfo* spawnInfo = monsterspawnPkt.add_monsters_spawn_info();
                spawnInfo->mutable_spawnposinfo()->CopyFrom(*(obj->Getpos()));
                spawnInfo->set_templeteid(obj->GetTempleteId());
            }
        }

        if (playerspawnPkt.players_spawn_info_size() > 0)
        {
            auto sendBuffer = ServerUtils::MakeSendBuffer(playerspawnPkt, Protocol::PKT_SC_PLAYER_SPAWN);
            if (!sendBuffer) return;
            
            
            if (auto s = player->session.lock()) s->Send(sendBuffer);
        }
        if (monsterspawnPkt.monsters_spawn_info_size() > 0)
        {
            auto sendBuffer = ServerUtils::MakeSendBuffer(monsterspawnPkt, Protocol::PKT_SC_MONSTER_SPAWN);
            if (!sendBuffer) return;
            
            if (auto s = player->session.lock()) s->Send(sendBuffer);
        }
    }

}
void Room::SpawnBroadcast(const std::vector<MonsterPtr>& monsters)
{

    Protocol::SC_MONSTER_SPAWN monsterspawn_pkt;

    for(MonsterPtr monster : monsters) 
    {
        Protocol::SpawnInfo* spawnInfo = monsterspawn_pkt.add_monsters_spawn_info();

        spawnInfo->mutable_spawnposinfo()->CopyFrom(*(monster->Getpos()));
        spawnInfo->set_templeteid(monster->GetTempleteId());
    }

    if (monsterspawn_pkt.monsters_spawn_info_size() > 0) // 데이터가 있을 때만 전송
    {

        auto sendBuffer = ServerUtils::MakeSendBuffer(monsterspawn_pkt, Protocol::PKT_SC_MONSTER_SPAWN);

        if (sendBuffer) {
            //전체에게 전송
            Broadcast(sendBuffer);
        }

        
    }

}
void Room::SpawnBroadcast(MonsterPtr monster)
{
    SpawnBroadcast(std::vector<MonsterPtr>{monster});
}

void Room::BroadcastMove(const std::vector<GameObjectPtr>& gameobjects)
{
    Protocol::SC_MOVING movePkt;
    for (GameObjectPtr go : gameobjects) 
    {
        if (go == nullptr) return;

        Protocol::PosInfo* newPos = movePkt.add_pos_info();
        newPos->CopyFrom(*go->Getpos());

        // Sanitize all components (use isfinite to catch inf/NaN)
        if (!std::isfinite(newPos->x()) || !std::isfinite(newPos->y()) || !std::isfinite(newPos->z()))
        {
            std::cout << "BroadcastMove: invalid coordinates detected. Dropping broadcast." << std::endl;
            return;
        }
        
        //log
        //std::cout << "\n newPos: \t" << newPos->x() << "\t" << newPos->y() << "\t" << newPos->z() << std::endl;

       
    }
    
    if (movePkt.pos_info_size() > 0) 
    {
        auto sendBuffer = ServerUtils::MakeSendBuffer(movePkt, Protocol::PKT_SC_MOVING);


        if (sendBuffer)
        {
            if (gameobjects.size() == 1) 
            {
                //주변에 방송 (본인 제외 로직은 BroadcastAround에 구현된 passing_object_id 활용)
                BroadcastAround(sendBuffer, Vector3::PosInfoToVector3(gameobjects[0]->Getpos()), gameobjects[0]->GetObjectId());
            }
            else 
            { 
                //TODO BroadcastAround 를 하나의 위치 기준으로 하는게 아니라 여러개의 위치를 기반으로 적당한 범위에 방송하도록 오버로딩필요
                Broadcast(sendBuffer);
            }
            //Broadcast(sendBuffer);
        }
           
    }

}


void Room::BroadcastMove(GameObjectPtr go)
{
    BroadcastMove(std::vector< GameObjectPtr>{ go });
}


void Room::SendTo(PlayerPtr player, SendBufferPtr sendBuffer)
{
    // Snapshot session under lock, then send outside lock
        std::shared_ptr<Session> session;
    {
        std::lock_guard<std::mutex> lock(_lock);
        if (!player) return;
        session = player->session.lock();
    }

    if (session) {
        try {
            session->Send(sendBuffer);
        }
        catch (const std::exception& e) {
            std::cerr << "SendTo: session->Send threw: " << e.what() << " Guid:" << session->GetGuid() << std::endl;
        }
        catch (...) {
            std::cerr << "SendTo: unknown exception when sending to Guid:" << (session ? session->GetGuid() : 0) << std::endl;
        }
    }
}

void Room::HandleMove(PlayerPtr player ,Protocol::CS_MOVING& pkt)
{
    if (player == nullptr) 
    { 
        std::cout << "HandleMove : player ptr is null" << std::endl; 
        return;
    }

    Protocol::PosInfo posInfo = pkt.pos_info();
    std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([weakSelf, player, posInfo]() {
        if (auto self = weakSelf.lock()) { // 실행 시점에 룸이 살아있는지 확인
            // 1. [검증] 이전 위치와 새 위치의 거리 차이가 너무 크면 무시하거나 보정 (핵 방지)
            Vector3 currentPos = Vector3::PosInfoToVector3(player->Getpos());
            Vector3 newPos = Vector3(posInfo.x(), posInfo.y(), posInfo.z());

            // 처음에만 0일 수 있으므로 예외 처리

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

                self->SendMoveResync(player);
                return;
            }

            // 지형 검증
            MapPtr map = self->GetMapptr();
            if (map != nullptr)
            {
                if (map->CanGo(newPos) == false)
                {
                    // 충돌 발생! 클라이언트에게 강제 위치 복구 패킷 전송
                    self->SendMoveResync(player);
                    return;
                }
            }

            //[갱신] 그리드 업데이트
            {
                self->UpdateObjectGrid(player, currentPos, newPos);
            }


            // 2. [갱신] 서버 메모리에 플레이어 위치 정보 업데이트
            player->Setpos(posInfo);

            // 3. [전달] 방 안의 다른 유저들에게 이동 사실 브로드캐스트

            self->BroadcastMove(player);
            //Loging
            /*std::cout << "RoomId: " << _Selfroomid << std::endl
                << "object Id : " << resPos->object_id() << "HandleMove : (" << resPos->x() << resPos->y() << resPos->z() << ")" << std::endl;*/
        }
        });
}

void Room::HandleSkillForPlayer(PlayerPtr player, Protocol::CS_SKILL& pkt)
{
    int32 skillid = pkt.skill_id();
    int32 targetObjectId = pkt.has_target() ? pkt.target().target_object_id() : -1;
    bool hasDestPos = pkt.has_dest_pos();
    Vector3 targetPos = hasDestPos ? Vector3(pkt.dest_pos().x(), pkt.dest_pos().y(), pkt.dest_pos().z()) : Vector3(0, 0, 0);

    std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());
    
    // 상태 변경 로직을 안전하게 단일 스레드에서 순차적으로 처리하기 위해 JobQueue에 Push
    this->Push([weakSelf, player, targetObjectId, targetPos, skillid]() {
        if (auto self = weakSelf.lock()) {
            GameObjectPtr targetobj = nullptr;
            if (targetObjectId != -1)
            {
                targetobj = GObjcetManager.Find(targetObjectId);
            }
            
            self->HandleSkill(player, targetobj, targetPos, skillid);
        }
    });
}

void Room::HandleSkillForMonster(MonsterPtr monster, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid)
{
    std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([weakSelf, monster, targetobj, targetPos, skillid]() {
        if (auto self = weakSelf.lock()) {
            self->HandleSkill(monster, targetobj, targetPos, skillid);
        }
    });
}

void Room::HandleSkill(GameObjectPtr SKillUser, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid)
{
    // [수정] 플레이어만 사용 -> 모든 스킬을 쓸수 있는 게임 오브젝트가 사용하는 메소드
    // TODO: [DB컨텐츠 추가 필요][핵 방지]_skillCooltimes 를 이용하던 아니면 다른 메모리영역을 추가하던 해서 스킬이 진짜 그 캐릭터가 쓸수 있는 스킬인지 체크하는 로직필요

    const SkillData* skilldata = DataManager::GetInstance().GetSkill(skillid);
    int64 now = GetTickCount64();
    int64 lastUsed = SKillUser->_skillCooltimes[skilldata->id];
    int64 coolTime = skilldata->coolTime * 1000; // 초 단위를 ms로 변환

    if (now - lastUsed < coolTime) {
        // 아직 쿨타임 중! 요청 무시 혹은 에러 패킷 전송
        std::cout << "it's cooltime" << std::endl;
        return;
    }

    // 검증 통과 후 사용 시점 갱신
    SKillUser->_skillCooltimes[skilldata->id] = now;

    // 3. 코스트(마나 등) 체크 및 차감
    // (Player 클래스에 GetStat(), SetStat() 혹은 직접 접근 가능한 멤버가 있다고 가정)
    if (skilldata->costType == CostType::Mana) {
        
        int32 requiredMp = skilldata->cost; // 예시: 스킬 데이터에 코스트 수치를 추가하면 더 좋습니다.

        if (SKillUser->UseMp(requiredMp) == false)
        {
            //마나 부족, 본인에게 메시지등을 보낼수 있을것임
            std::cout << "Mana is not enough " << std::endl;
            return;
        }

    }

    // 4. 검증 통과 후 사용 시점 갱신
    SKillUser->_skillCooltimes[skilldata->id] = now;

    // 5. 스킬 타입별 피격 판정
    bool isHit = false;

    switch (skilldata->skillType)
    {
    case SkillType::Melee:
    {
        for (auto obj : _objects)
        {
            GameObjectPtr target = obj.second;
            if (target && target->GetObjectId() != SKillUser->GetObjectId()) {
                // 거리 계산 (Vector3::Distance)
                float dist = Vector3::Distance(Vector3::PosInfoToVector3(SKillUser->Getpos()), Vector3::PosInfoToVector3(target->Getpos()));

                // 사거리 검증 (약간의 마진 부여: 0.5f)
                if (dist <= skilldata->range + 0.5f) {
                    isHit = true;

                    // 데미지 계산 및 적용(서버 메모리 업데이트)
                    int32 damage = skilldata->damage + SKillUser->GetAttack(); // 스킬데미지 + 캐릭터공격력 수정 가능
                    target->OnAttacked(damage); // 대상의 HP를 깎는 함수 호출

                    UpdateHPToOthers(target, SKillUser, damage, SKillUser->Getpos_As_Vector3());

                    std::cout << "[Melee Hit] " << SKillUser->GetName() << " -> " << target->GetName() << " (Damage: " << damage << ")" << std::endl;
                }
            }
        }

    }
    break;

    case SkillType::Projectile:
        std::cout << "Spawn Projectile" << std::endl;
        SpawnProjectile(SKillUser, skilldata, targetPos);
        
        break;

    case SkillType::Dash:
        // 이동 가능 지역인지 확인 후 좌표 강제 갱신
        break;
    }

    // 6. 결과 브로드캐스트 (주변 모두에게 애니메이션 알림)
    if (SKillUser->GetType() == GameObjectType::Player)
    {
        PlayerPtr player = std::static_pointer_cast<Player>(SKillUser);
        //본인에게 MP 변화 패킷 전송
        UpdateMPToSelf(player);
    }
    

    Protocol::SC_SKILL resPkt;
    resPkt.set_object_id(SKillUser->GetObjectId());
    resPkt.set_skill_id(skilldata->id);

    bool Istargetpos = (skilldata->targetType == SkillTargetType::positionTarget);

    if (targetobj)
    {
        // mutable_target()을 통해 TargetobjectInfo 객체의 포인터를 얻어 값 설정
        resPkt.mutable_target()->set_target_object_id(targetobj->GetObjectId());
    }
    else if (Istargetpos)
    {

        resPkt.mutable_dest_pos()->set_x(targetPos.x);
        resPkt.mutable_dest_pos()->set_y(targetPos.y);
        resPkt.mutable_dest_pos()->set_z(targetPos.z);
    }

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_SKILL);
    if (!sendBuffer) return;
    BroadcastAround(sendBuffer, SKillUser->Getpos_As_Vector3());
    
}

//투사체 관련 함수
void Room::SpawnProjectile(GameObjectPtr attacker, const SkillData *skillData, Vector3 targetPos)
{
    if (attacker == nullptr || skillData == nullptr)
        return;

    // 1. 투사체 객체 생성
    GameObjectPtr go = GObjcetManager.Create(GameObjectType::Projectile, nullptr, skillData->projectileId);
    std::shared_ptr<Projectile> projectile = std::static_pointer_cast<Projectile>(go);
    
    if (projectile == nullptr)
        return;

    // 2. 공격자의 위치로 초기 좌표 설정
    Protocol::PosInfo posInfo = *(attacker->Getpos());
    projectile->Setpos(posInfo);

    // 3. 투사체 방향 및 속도 등 초기화
    projectile->Init(attacker, skillData, targetPos);

    // 4. 룸에 입장 (내부 관리 목록 _objects 추가 및 그리드 배치)
    // TODO: 클라이언트 측에 투사체 스폰을 알리는 패킷 전송 로직이 필요하다면 여기에 추가
    Enter(projectile);
}

void Room::UpdateProjectile(std::shared_ptr<Projectile> projectile)
{
    if (projectile == nullptr || projectile->GetState() == CreatureState::Dead)
        return;

    Vector3 oldPos = Vector3::PosInfoToVector3(projectile->Getpos());
    
    // 1. 투사체 이동 진행
    projectile->TickMove();
    
    Vector3 newPos = Vector3::PosInfoToVector3(projectile->Getpos());

    // 2. 그리드 좌표 업데이트
    UpdateObjectGrid(projectile, oldPos, newPos);

    // TODO: 타겟 충돌(피격) 판정 및 사거리 도달 시 삭제 로직 구현
    // 충돌 시나 사거리 초과 시 -> projectile->SetState(CreatureState::Dead);

    // 3. 이동 패킷 브로드캐스트
    BroadcastMove(projectile);
}

//본인에게 MP 변경 패킷 전송 
void Room::UpdateMPToSelf(PlayerPtr player)
{
    
    Protocol::SC_CHANGE_MP mp_Change_pkt;
    mp_Change_pkt.set_object_id(player->GetObjectId());
    mp_Change_pkt.set_current_mp(player->GetCurrentMp());
    auto sendBuffer = ServerUtils::MakeSendBuffer(mp_Change_pkt, Protocol::PKT_SC_CHANGE_MP);

    if (!sendBuffer) return;

    SendTo(player, sendBuffer);

}

//본인에게 HP 변경 패킷 전송
void Room::UpdateHPToSelf(PlayerPtr player)
{
    Protocol::SC_CHANGE_HP hp_Change_pkt;
    hp_Change_pkt.set_object_id(player->GetObjectId());
    hp_Change_pkt.set_current_hp(player->GetCurrentHp());
    auto sendBuffer = ServerUtils::MakeSendBuffer(hp_Change_pkt, Protocol::PKT_SC_CHANGE_HP);

    if (!sendBuffer) return;

    SendTo(player, sendBuffer);
}

void Room::UpdateMPToOthers(GameObjectPtr target, Vector3 broadcastcenter)
{
    //Mp 변화 방송
    Protocol::SC_CHANGE_MP mp_changed_pkt;
    mp_changed_pkt.set_object_id(target->GetObjectId());
    mp_changed_pkt.set_current_mp(target->GetCurrentMp());
    
    auto sendBuffer = ServerUtils::MakeSendBuffer(mp_changed_pkt, Protocol::PKT_SC_CHANGE_MP);
    if (!sendBuffer) return;

    BroadcastAround(sendBuffer, broadcastcenter);

}


void Room::UpdateHPToOthers(GameObjectPtr target, GameObjectPtr attacker, int damage, Vector3 broadcastcenter)
{
    //Hp 변화 방송
    Protocol::SC_CHANGE_HP hp_changed_pkt;
    hp_changed_pkt.set_object_id(target->GetObjectId());
    hp_changed_pkt.set_current_hp(target->GetCurrentHp());
    hp_changed_pkt.set_damage(damage);
    hp_changed_pkt.set_attacker_id(attacker->GetObjectId());
    auto sendBuffer = ServerUtils::MakeSendBuffer(hp_changed_pkt, Protocol::PKT_SC_CHANGE_HP);
    if (!sendBuffer) return;

    BroadcastAround(sendBuffer, broadcastcenter);

}

void Room::MonsterSpawn(int32 NumOfMonster, int templatedId)
{
    // [수정] 외부 쓰레드(ConsoleThread 등)에서 호출될 것을 대비해 
    // 실제 로직을 람다로 묶어 JobQueue에 넣습니다.
    RoomPtr self = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([self, NumOfMonster, templatedId]() {
        std::vector<MonsterPtr> monsters;
        for (int i = 0; i < NumOfMonster; i++)
        {
            MonsterPtr monster = std::static_pointer_cast<Monster>(
                GObjcetManager.Create(GameObjectType::Monster, nullptr, templatedId)
            );

            // 좌표 설정 등 로직 수행
            monster->Set_x(10.0f + i * 2.0f);
            monster->Set_z(10.0f);

            monsters.push_back(monster);
        }

        // EnterMonsters 내부에서도 락을 잡고 데이터를 수정하므로 
        // Job 내부에서 실행되는 것이 안전합니다.
        self->EnterMonsters(monsters);

        std::cout << "[Job] MonsterSpawn completed: " << NumOfMonster << " monsters." << std::endl;
        });
}

PlayerPtr Room::GetNearestPlayer(Vector3 pos, float maxRange)
{
    PlayerPtr nearestPlayer = nullptr;
    float bestDistSq = maxRange * maxRange; // 제곱근 연산을 피하기 위해 거리의 제곱 사용

    // 1. 주변 9개 그리드에 있는 플레이어 리스트를 가져옴 (이미 구현된 함수 활용)
    std::vector<PlayerPtr> adjacentPlayers = GetAdjacentPlayers(pos);

    // 2. 리스트를 순회하며 가장 가까운 플레이어 탐색
    for (const PlayerPtr& player : adjacentPlayers)
    {
        // 사망 상태인 플레이어는 제외 (필요 시)
        if (player->GetState() == CreatureState::Dead)
            continue;

        Vector3 playerPos = Vector3::PosInfoToVector3(player->Getpos());

        // 두 지점 사이의 거리 제곱 계산 (sqrt를 안 써서 성능 이득)
        float distSq = Vector3::DistanceSquared(pos, playerPos);

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            nearestPlayer = player;
        }
    }

    return nearestPlayer;
}

//위치 되감기
void Room::SendMoveResync(PlayerPtr player)
{
    // 1. 서버에 저장된 '이전' 좌표를 담은 패킷 생성
    Protocol::SC_MOVING resPkt;

    Protocol::PosInfo* resPos = resPkt.add_pos_info();
    resPos->CopyFrom(*(player->Getpos())); // 업데이트 전의 서버 좌표
    resPos->set_state((int)CreatureState::Idle);

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_MOVING);

    if (!sendBuffer) return;

    // 2. 해당 유저에게만 강제로 전송 (위치 되감기)
    if (auto s = player->session.lock())
        s->Send(sendBuffer);

}

std::pair<int, int> Room::GetSectorPos(Vector3 pos)
{
    // 1. 먼저 물리 타일 좌표로 변환
    int cellX = static_cast<int>(std::floor((pos.x - _minX) / _cellSize));
    int cellZ = static_cast<int>(std::floor((pos.z - _minZ) / _cellSize));

    // 2. 물리 좌표를 섹터 크기로 나누어 섹터 인덱스 산출
    int sectorX = cellX / _sectorSize;
    int sectorZ = cellZ / _sectorSize;

    // 범위 제한
    sectorX = std::clamp(sectorX, 0, _sectorCountX - 1);
    sectorZ = std::clamp(sectorZ, 0, _sectorCountZ - 1);

    return { sectorX, sectorZ };
}

//인접 플레이어 추출 (Interest Management)
std::vector<std::shared_ptr<Session>> Room::GetAdjacentPlayersSessions(Vector3 pos, int32 passing_object_id)
{
    std::vector<std::shared_ptr<Session>> SessionsOfadjacentPlayers;

    auto [cellX, cellZ] = GetSectorPos(pos);

    // Snapshot relevant GameObjectPtr under lock to avoid concurrent modification
    std::vector<GameObjectPtr> snapshot;
    {
        std::lock_guard<std::mutex> lock(_lock);
        for (int dz = -1; dz <= 1; ++dz)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                int nx = cellX + dx;
                int nz = cellZ + dz;

                if (nx >= 0 && nx < _sectorCountX && nz >= 0 && nz < _sectorCountZ)
                {
                    for (auto& go : _sectors[nz][nx])
                    {
                        snapshot.push_back(go);
                    }
                }
            }
        }
    }

    // Process snapshot without holding the room lock
    for (auto& go : snapshot)
    {
        if (!go) continue;

        // CHECK ID FIRST before any weak_ptr operations
        if (go->GetObjectId() == passing_object_id) continue;

        if (go->GetType() == GameObjectType::Player)
        {
            auto player = std::static_pointer_cast<Player>(go);

            // Only now try to lock the session weak_ptr
            if (auto session = player->session.lock())
            {

                SessionsOfadjacentPlayers.push_back(session);
            }
        }
    }

    return SessionsOfadjacentPlayers;
}

std::vector <PlayerPtr> Room::GetAdjacentPlayers(Vector3 pos, int32 passing_object_id) 
{
    std::vector <PlayerPtr>adjacentPlayers;

    auto [cellX, cellZ] = GetSectorPos(pos);

    // Snapshot relevant GameObjectPtr under lock to avoid concurrent modification
    std::vector<GameObjectPtr> snapshot;
    {
        std::lock_guard<std::mutex> lock(_lock);
        for (int dz = -1; dz <= 1; ++dz)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                int nx = cellX + dx;
                int nz = cellZ + dz;

                if (nx >= 0 && nx < _sectorCountX && nz >= 0 && nz < _sectorCountZ)
                {
                    for (auto& go : _sectors[nz][nx])
                    {
                        snapshot.push_back(go);
                    }
                }
            }
        }
    }

    // Process snapshot without holding the room lock
    for (auto& go : snapshot)
    {
        if (!go) continue;

        // CHECK ID FIRST before any weak_ptr operations
        if (go->GetObjectId() == passing_object_id) continue;

        if (go->GetType() == GameObjectType::Player)
        {
            auto player = std::static_pointer_cast<Player>(go);

            // Only now try to lock the session weak_ptr
            if (auto session = player->session.lock())
            {

                adjacentPlayers.push_back(player);
            }
        }
    }

    return adjacentPlayers;
}





void Room::UpdateObjectGrid(GameObjectPtr go, Vector3 oldPos, Vector3 newPos)
{
    auto oldGridPos = GetSectorPos(oldPos);
    auto newGridPos = GetSectorPos(newPos);

    if (oldGridPos == newGridPos)
        return;

    int oldX = oldGridPos.first;
    int oldZ = oldGridPos.second;
    int newX = newGridPos.first;
    int newZ = newGridPos.second;

    // Protect _sectors modification with lock
    {
        std::lock_guard<std::mutex> lock(_lock);
        // Defensive: verify indices valid
        if (oldZ >= 0 && oldZ < (int)_sectors.size() && oldX >= 0 && oldX < (int)_sectors[oldZ].size())
            _sectors[oldZ][oldX].erase(go);
        if (newZ >= 0 && newZ < (int)_sectors.size() && newX >= 0 && newX < (int)_sectors[newZ].size())
            _sectors[newZ][newX].insert(go);
    }
}

void Room::BroadcastAround(SendBufferPtr sendBuffer, Vector3 centerPos, int32 passing_object_id)
{
    // 모든 플레이어가 아니라 인접한 플레이어에게만 보냄
    std::vector<std::shared_ptr<Session>> targets = GetAdjacentPlayersSessions(centerPos, passing_object_id);

    
    Broadcast(sendBuffer, targets);
}

void Room::InitGridData(const MapData* mapdata)
{
    _cellSize = mapdata->CellSize;
    _minX = mapdata->MinX;
    _minZ = mapdata->MinZ;
    // 섹터 개수 계산 (전체 가로/세로 타일 수 / 섹터 크기)
    _sectorCountX = (mapdata->width / _sectorSize) + 1;
    _sectorCountZ = (mapdata->height / _sectorSize) + 1;

    _sectors.assign(_sectorCountZ, std::vector<std::set<GameObjectPtr>>(_sectorCountX));
}



void Room::Execute() 
{
    
    // 1. Execute queued jobs for this room
    JobQueue::Execute();

    // 2. Collect monsters under lock to avoid concurrent-modification while iterating
    
    //여기서 사망 상태 패킷 방송
    Protocol::SC_MONSTER_DEAD deadpkt; bool anyDead = false;

    std::vector<MonsterPtr> monstersToUpdate;
    std::vector<ProjectilePtr> projectilesToUpdate;
    {
        std::lock_guard<std::mutex> lock(_lock);
        for (auto& item : _objects)
        {
            if (item.second->GetType() == GameObjectType::Monster)
            {
                monstersToUpdate.push_back(std::static_pointer_cast<Monster>(item.second));
            }
            else if (item.second->GetType() == GameObjectType::Projectile)
            {
                projectilesToUpdate.push_back(std::static_pointer_cast<Projectile>(item.second));
            }
            if (item.second->GetState() == CreatureState::OnDead)
            {
                
                deadpkt.add_dead_object_id_list(item.second->GetObjectId());
                item.second->SetState(CreatureState::Dead);
                anyDead = true; // 죽은 몬스터가 있을 때만 플래그 활성화
            }

        }
    }

    // 3. Push monster update jobs (do not hold _lock while invoking Push)
    for (auto& monster : monstersToUpdate)
    {
        this->Push([monster]() {
            monster->JobUpdate();
            });
    }

        std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());
        for (auto& projectile : projectilesToUpdate)
        {
            this->Push([weakSelf, projectile]() {
                if (auto self = weakSelf.lock()) {
                    self->UpdateProjectile(projectile);
                }
            });
        }

    if (anyDead) 
    {
        SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(deadpkt, Protocol::PKT_SC_MONSTER_DEAD);

        if (!sendBuffer) return;

        Broadcast(sendBuffer);

        std::cout << "SomeOne is dead, boradcasting" << std::endl;
    }
    
}
