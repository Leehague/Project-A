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
#include "DataManager.h"
#include "Creature.h"
#include "CoreRoom.h"
#include "InfoSturct.h"
#include "QuestEvent.h"
#include "LoginManager.h"
#include "DBManager.h"



// 내부 구조체(Core::PosInfo)를 Protobuf 패킷(Protocol::PosInfo)으로 복사하는 헬퍼 함수
static void CopyCorePosToProtocol(Protocol::PosInfo* dest, const Core::PosInfo* src)
{
    if (dest == nullptr || src == nullptr) return;
    dest->set_object_id(src->object_id);
    dest->set_x(src->x);
    dest->set_y(src->y);
    dest->set_z(src->z);
    dest->set_yaw(src->yaw);
    dest->set_state((int32)src->state);

}

static void CopyProtocolPosToCore(Core::PosInfo* dest, const Protocol::PosInfo* src)
{
    if (dest == nullptr || src == nullptr) return;
    dest->object_id = src->object_id();
    dest->x = src->x();
    dest->y = src->y();
    dest->z = src->z();
    dest->yaw = src->yaw();
    dest->state =(CreatureState) src->state();
}

Room::Room(int32 roomId, int32 mapId) :JobQueue(&GJobSerializer) , _Selfroomid(roomId)
{
    // 방이 생성될 때 맵 매니저를 통해 맵을 할당받습니다.
    // 맵할당 로직은 Coreroom의 생성자로 이동했음
    bool maploadsuccess;
    _coreroom = std::make_shared<CoreRoom>(mapId, maploadsuccess);

    if (maploadsuccess == false)
    {
        std::cout << "Room " << roomId << ": Map Load Failed! (ID: " << mapId << ")" << std::endl;
        return;
    }
}

void Room::Init()
{
    // CoreRoom 안에서 누군가 움직였다고 알리면, 기존 Room의 BroadcastMove를 실행하도록 콜백 연결
    std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());
    _coreroom->_onObjectMovedCallback = [weakSelf](GameObjectPtr go) {
        if (auto self = weakSelf.lock()) {
            self->BroadcastMove(go); // 기존에 만들었던 이동 패킷 전송 로직 실행
        }
    };

    // CoreRoom에서 오브젝트 생성 시 GObjcetManager를 호출하도록 팩토리 연결 , Create에서의 characterId 는 라이브서버에서만 유효한 인자이므로 -1로 처리
    _coreroom->_objectFactoryCallback = [](GameObjectType type, int32 templateId , CoreRoomPtr _coreroom) -> GameObjectPtr {
        return GObjcetManager.Create(type, nullptr, templateId, _coreroom,-1);
    };
    
}


void Room::Enter(GameObjectPtr go)
{
    std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([weakSelf, go]() {
        //서버 내부 메모리에 정보 저장(방 입장 처리)

        if (auto self = weakSelf.lock()) {
            self->_coreroom->_objects[go->GetObjectId()] = go;

            //기존에 Room에 들어 있던 객체들이 아니기 때문에 기본값(0 , nullptr )으로 설정되어 있을 Room, CoreRoom 정보 업데이트
            go->SetroomId(self->_Selfroomid);
            go->SetCoreroomptr(self->_coreroom);

            

            auto [cellX, cellZ] = self->_coreroom->GetSectorPos(Vector3::PosInfoToVector3(go->Getpos())); // 좌표로 인덱스 추출
            self->_coreroom->_sectors[cellZ][cellX].insert(go);
       
            //본인에게 입장 성공 및 좌표 알림 (SC_ENTER_GAME)
            if (go->GetType() == GameObjectType::Player)
            {
                auto player = std::static_pointer_cast<Player>(go);
                if (player == nullptr) { return; }

                // 1. 세션 락을 딱 한 번만 잡아서 안전하게 보관합니다.
                auto session = player->session.lock();
                if (session == nullptr) { return; } // 그 새 연결이 끊겼다면 조용히 무시

                session->SetPlayerId(player->GetObjectId());
                session->SetPlayerPtr(player);
                
                Protocol::SC_ENTER_GAME enterPkt;
                CopyCorePosToProtocol(enterPkt.mutable_pos_info(), player->Getpos()); // 서버가 결정한 좌표

                // 2. [핵심] Core::PosInfo 내부에 object_id가 0으로 비어있을 수 있으므로 
                // 여기서 확실하게 플레이어의 ObjectId로 덮어씌워 줍니다!
                enterPkt.mutable_pos_info()->set_object_id(player->GetObjectId());

                enterPkt.set_templateid(go->GetTemplateId()); //핸들러에서 결정된 템플릿 아이디

                enterPkt.set_mapid(self->GetMapptr()->GetMapId()); //클라에 보내줄 맵 Id
                auto sendBuffer = ServerUtils::MakeSendBuffer(enterPkt, Protocol::PKT_SC_ENTER_GAME);

                if (!sendBuffer) return;

               

                // 3. 락을 두 번 걸지 않고 처음에 확보해 둔 session 변수를 통해 Send 합니다.
                session->Send(sendBuffer);
            }
            else if (go->GetType() == GameObjectType::Monster)
            {
                MonsterPtr monster = std::static_pointer_cast<Monster>(go);
                self->SpawnBroadcast(monster);
            }
            // 수정 : 스폰 패킷은 Room::Enter 에서 전송하지 않고 나중에 레디 패킷을 수신해서 전송함
        }
    });
}

void Room::EnterMonsters(const std::vector<MonsterPtr>& monsters)
{
    std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());
    //서버 내부 메모리에 정보 저장(방 입장 처리)
    this->Push([weakSelf, monsters]() {
        if (auto self = weakSelf.lock()) {

            for (MonsterPtr monster : monsters)
            {
                //std::lock_guard<std::mutex> lock(_lock);
                self->_coreroom->_objects[monster->GetObjectId()] = monster;
                monster->SetroomId(self->_Selfroomid);
                monster->SetCoreroomptr(self->_coreroom);
                
                //몬스터에게 이 방(Room은 JobQueue를 상속받음)의 포인터를 주입합니다.
                monster->SetOwnerJobQueue(self); 
                
                monster->SetSkillCallback([self](MonsterPtr m, GameObjectPtr t, Vector3 pos, int32 id) {
                    // 실서버용 Room 스킬 핸들러 실행 (패킷 브로드캐스트 포함)
                    self->HandleSkillForMonster(m, t, pos, id);
                });

                auto [cellX, cellZ] = self->_coreroom->GetSectorPos(monster->Getpos_As_Vector3());

                if (cellZ >= 0 && cellZ < (int)self->_coreroom->_sectors.size() &&
                    cellX >= 0 && cellX < (int)self->_coreroom->_sectors[cellZ].size())
                {
                    self->_coreroom->_sectors[cellZ][cellX].insert(monster);
                }
            }

            self->SpawnBroadcast(monsters);
        }
        });
}

void Room::Leave(GameObjectPtr go)
{
    if (go == nullptr) return;

    {
        int32 objectID = go->GetObjectId();
        auto it = _coreroom->_objects.find(objectID);
        if (it == _coreroom->_objects.end())
            return; // 이미 나갔거나 없는 객체면 무시

        // 그리드에서 제거
        auto [cellX, cellZ] = _coreroom->GetSectorPos(Vector3::PosInfoToVector3(it->second->Getpos()));
        _coreroom->_sectors[cellZ][cellX].erase(it->second);

        _coreroom->_objects.erase(objectID); // 1. 룸의 관리 목록에서 제거
    }


    if (go->GetType() == GameObjectType::Player)
    {
        PlayerPtr player = std::dynamic_pointer_cast<Player>(go);

        if (!player) { return; } //하지만 아마 여기에 걸리면 버그가 있을것
        uint64 playerId = player->GetObjectId();

        //타인들에게 이 유저가 나갔음을 알림 (SC_DESPAWN)
        Protocol::SC_PLAYER_DESPAWN despawnPkt;
        despawnPkt.add_player_id(playerId);


        auto sendBuffer = ServerUtils::MakeSendBuffer(despawnPkt, Protocol::PKT_SC_PLAYER_DESPAWN);
        if (!sendBuffer) return;

        Broadcast(sendBuffer, playerId); // 본인은 이미 나갔으므로 제외
    }
    
    

}

void Room::Broadcast(SendBufferPtr sendBuffer)
{
    Broadcast(sendBuffer, -1);
}
void Room::Broadcast(SendBufferPtr sendBuffer, int32 passing_object_id)
{
    std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([weakSelf, passing_object_id, sendBuffer]() {
        
        
        if (auto self = weakSelf.lock()) {
            // Snapshot target sessions under lock to avoid data-race / iterator invalidation
            std::vector<std::shared_ptr<Session>> targets;
            {
                for (auto& pair : self->_coreroom->_objects) {
                    if (pair.second->GetType() != GameObjectType::Player) continue;
                    auto player = std::static_pointer_cast<Player>(pair.second);
                    if (player->GetObjectId() == passing_object_id) continue;
                    if (auto session = player->session.lock()) {
                        targets.push_back(session);
                    }
                }
            }
            self->Broadcast(sendBuffer, targets);//target이 있는 버전의 Braodcast를 호출함

        }
    });
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
    std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([weakSelf, player]() {

        if (auto self = weakSelf.lock())
        {
            //(player 기준)타인들에게 나를 알림 (SC_PLAYER_SPAWN 브로드캐스트)
            {
                Protocol::SC_PLAYER_SPAWN spawnPkt;

                Protocol::SpawnInfo* spawnInfo = spawnPkt.add_players_spawn_info();

                CopyCorePosToProtocol(spawnInfo->mutable_spawnposinfo(), player->Getpos());
                spawnInfo->set_templateid(player->GetTemplateId());


                if (spawnPkt.players_spawn_info_size() > 0) // 데이터가 있을 때만 전송
                {

                    SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(spawnPkt, Protocol::PKT_SC_PLAYER_SPAWN);

                    if (!sendBuffer) return;

                    // 나를 제외한 모두에게 전송 (기존에 만든 Broadcast 함수 활용)
                    self->Broadcast(sendBuffer, player->GetObjectId());
                }

            }

            //(player 기준)나에게 기존 오브젝트들을 알림 (SC_PLAYER_SPAWN 목록 전송)
            {
                Protocol::SC_PLAYER_SPAWN playerspawnPkt;
                Protocol::SC_MONSTER_SPAWN monsterspawnPkt;

                
               
                for (auto& pair : self->_coreroom->_objects)
                {
                    GameObjectPtr obj =pair.second;
                    if (!obj) continue;
                    if (obj->GetType() == GameObjectType::Player)
                    {
                        Protocol::SpawnInfo* spawnInfo = playerspawnPkt.add_players_spawn_info();
                        CopyCorePosToProtocol(spawnInfo->mutable_spawnposinfo(), obj->Getpos());
                        spawnInfo->set_templateid(obj->GetTemplateId());
                    }
                    else if (obj->GetType() == GameObjectType::Monster)
                    {
                        Protocol::SpawnInfo* spawnInfo = monsterspawnPkt.add_monsters_spawn_info();
                        CopyCorePosToProtocol(spawnInfo->mutable_spawnposinfo(), obj->Getpos());
                        spawnInfo->set_templateid(obj->GetTemplateId());
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
        
        });
    

}
void Room::SpawnBroadcast(const std::vector<MonsterPtr>& monsters)
{

    Protocol::SC_MONSTER_SPAWN monsterspawn_pkt;

    for(MonsterPtr monster : monsters) 
    {
        Protocol::SpawnInfo* spawnInfo = monsterspawn_pkt.add_monsters_spawn_info();

        //spawnInfo->mutable_spawnposinfo()->CopyFrom(*(monster->Getpos()));
        CopyCorePosToProtocol(spawnInfo->mutable_spawnposinfo(), monster->Getpos());
        spawnInfo->set_templateid(monster->GetTemplateId());
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
        CopyCorePosToProtocol(newPos, go->Getpos());

        // Sanitize all components (use isfinite to catch inf/NaN)
        if (!std::isfinite(newPos->x()) || !std::isfinite(newPos->y()) || !std::isfinite(newPos->z()))
        {
            std::cout << "BroadcastMove: invalid coordinates detected. Dropping broadcast." << std::endl;
            return;
        }
        
       
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
    if (!player) return;
    
    // weak_ptr의 lock()은 그 자체로 스레드 안전합니다.
    std::shared_ptr<Session> session = player->session.lock();

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

    const Protocol::PosInfo* posInfo = &(pkt.pos_info());
    Core::PosInfo pos;
    CopyProtocolPosToCore(&pos, posInfo);

    std::weak_ptr<Room> weakSelf = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([weakSelf, player, pos]() {
        if (auto self = weakSelf.lock()) { // 실행 시점에 룸이 살아있는지 확인

            if (player == nullptr)
            {
                std::cout << "HandleMove : player ptr is null" << std::endl;
                return;
            }

            if (self->_coreroom->HandleMove(player, pos))
            {
                // 3. [전달] 방 안의 다른 유저들에게 이동 사실 브로드캐스트

                self->BroadcastMove(player);
            }
            else
            {
                self->SendMoveResync(player);
            }
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


void Room::HandleSkill(CreaturePtr SKillUser, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid)
{
    // TODO: [DB컨텐츠 추가 필요][핵 방지]_skillCooltimes 를 이용하던 아니면 다른 메모리영역을 추가하던 해서 스킬이 진짜 그 캐릭터가 쓸수 있는 스킬인지 체크하는 로직필요

    const SkillData* skilldata = DataManager::GetInstance().GetSkill(skillid);
    if (skilldata == nullptr)
    {
        return;
    }
    std::vector<Core::DamageResult> damageresults;
    bool ishit = false; // 쓰레기값이 들어가지 않도록 초기화 해주는 것이 안전합니다.

    std::vector<GameObjectPtr> spawnedobejcts;
    // 각 지역 변수의 메모리 주소(&)를 포인터로 넘겨줍니다.
    if (_coreroom->HandleSkill(SKillUser, targetobj, targetPos, skillid, &damageresults, &ishit, &spawnedobejcts) == false)
    {
        return;
    }

    for (GameObjectPtr spawnedobejct : spawnedobejcts)
    {
        Enter(spawnedobejct);
    }



    // 6. 결과 브로드캐스트 (주변 모두에게 애니메이션 알림)

    //HP 손실 정보(피격정보)를 주변에 뿌림
    for (Core::DamageResult& result : damageresults)
    {
        CreaturePtr target = std::dynamic_pointer_cast<Creature>(result.target);

        if (target)
        {
            UpdateHPToOthers(target, SKillUser, result.damage, SKillUser->Getpos_As_Vector3());
        }

    }

    //스킬 사용자 , 본인에게 MP 변화 패킷 전송
    if (SKillUser->GetType() == GameObjectType::Player)
    {
        PlayerPtr player = std::static_pointer_cast<Player>(SKillUser);
        
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


void Room::UpdateProjectile(std::shared_ptr<Projectile> projectile)
{
    if (projectile == nullptr || projectile->GetState() == CreatureState::Dead)
        return;
    CreaturePtr attacker = std::dynamic_pointer_cast<Creature>(projectile->GetAttacker());

    bool ishit;
    //1.CoreRoom에서 이동 판정, 충돌 검사, 데미지 계산, 실제 HP 차감까지 모두 수행하고 결과만 반환받음
    std::vector<Core::DamageResult> results = _coreroom->UpdateProjectile(projectile, ishit);

    //2.피격 결과 네트워크 브로드캐스트
    for (const auto& result : results)
    {
        if (result.target == nullptr) continue;
        CreaturePtr target = std::dynamic_pointer_cast<Creature>(result.target);
        
        if (target == nullptr) continue; // 피격 대상이 Creature가 아니면 무시
        
        // TODO: 향후 광역 스킬을 위해 SC_CHANGE_HP_LIST 패킷이 추가되면, 
        // 이 루프 안에서 패킷 하나로 합치는(Batching) 로직으로 리팩토링하기 매우 좋습니다.
        UpdateHPToOthers(target, attacker, result.damage, result.target->Getpos_As_Vector3());
            
        if (target->GetType() == GameObjectType::Player) {
            UpdateHPToSelf(std::static_pointer_cast<Player>(target));
        }

        std::cout << "[Projectile Hit] " << attacker->GetName() << " -> " << target->GetName() << " (Damage: " << result.damage << ")" << std::endl;
    }

    if (!ishit) //충돌(피격) 하지 않았을 시에만 이동패킷 브로드 캐스팅
    {
        //3.이동 패킷 브로드캐스트
        BroadcastMove(projectile);
    }


    //[참고] projectile 의 state를 CreatureState::OnDead로 설정하는 로직은 coreroom에 있음


    if (projectile->GetState() == CreatureState::OnDead)
    {
        Leave(projectile);
        projectile->SetState(CreatureState::Dead); // 완전 소멸(Dead)로 변경하여 다음 프레임 루프 차단
    }
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

void Room::UpdateMPToOthers(CreaturePtr target, Vector3 broadcastcenter)
{
    //Mp 변화 방송
    Protocol::SC_CHANGE_MP mp_changed_pkt;
    mp_changed_pkt.set_object_id(target->GetObjectId());
    mp_changed_pkt.set_current_mp(target->GetCurrentMp());
    
    auto sendBuffer = ServerUtils::MakeSendBuffer(mp_changed_pkt, Protocol::PKT_SC_CHANGE_MP);
    if (!sendBuffer) return;

    BroadcastAround(sendBuffer, broadcastcenter);

}


void Room::UpdateHPToOthers(CreaturePtr target, CreaturePtr attacker, int damage, Vector3 broadcastcenter)
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
    MonsterSpawn(NumOfMonster, templatedId, false,false);
}

void Room::MonsterSpawn(int32 NumOfMonster, int templatedId, bool IsRLControll)
{
    MonsterSpawn(NumOfMonster,templatedId,IsRLControll, false);
}

void Room::MonsterSpawn(int32 NumOfMonster, int templatedId, bool IsRLControll, bool IsHusuabi)
{
    // [수정] 외부 쓰레드(ConsoleThread 등)에서 호출될 것을 대비해 
    // 실제 로직을 람다로 묶어 JobQueue에 넣습니다.
    RoomPtr self = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([self, NumOfMonster, templatedId, IsRLControll, IsHusuabi]() {
        std::vector<MonsterPtr> monsters;
        for (int i = 0; i < NumOfMonster; i++)
        {
            MonsterPtr monster = std::static_pointer_cast<Monster>(
                GObjcetManager.Create(GameObjectType::Monster, nullptr, templatedId, self->_coreroom,-1)
            );

            // 좌표 설정 등 로직 수행
            monster->Set_x(10.0f + i * 2.0f);
            monster->Set_z(10.0f);

            monster->SetRLControlled(IsRLControll);
            monster->SetHusuabi(IsHusuabi);

            monsters.push_back(monster);
        }

        // EnterMonsters 내부에서도 락을 잡고 데이터를 수정하므로 
        // Job 내부에서 실행되는 것이 안전합니다.
        self->EnterMonsters(monsters);

        std::cout << "[Job] MonsterSpawn completed: " << NumOfMonster << " monsters." << std::endl;
        });
}

void Room::HusuabiMonsterSpawn(int32 NumOfMonster, int templatedId)
{
    MonsterSpawn(NumOfMonster, templatedId, false, true);
}

PlayerPtr Room::GetNearestPlayer(Vector3 pos, float maxRange)
{
    return _coreroom->GetNearestPlayer(pos,  maxRange);
}

//위치 되감기
void Room::SendMoveResync(PlayerPtr player)
{
    // 1. 서버에 저장된 '이전' 좌표를 담은 패킷 생성
    Protocol::SC_MOVING resPkt;

    Protocol::PosInfo* resPos = resPkt.add_pos_info();
    //resPos->CopyFrom(*(player->Getpos())); // 업데이트 전의 서버 좌표
    CopyCorePosToProtocol(resPos, player->Getpos()); // 업데이트 전의 서버 좌표
    resPos->set_state((int)CreatureState::Idle);

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_MOVING);

    if (!sendBuffer) return;

    // 2. 해당 유저에게만 강제로 전송 (위치 되감기)
    if (auto s = player->session.lock())
        s->Send(sendBuffer);

}



void Room::BroadcastAround(SendBufferPtr sendBuffer, Vector3 centerPos, int32 passing_object_id)
{
    // 모든 플레이어가 아니라 인접한 플레이어에게만 보냄
    std::vector<PlayerPtr> adjacentPlayers = _coreroom->GetAdjacentPlayers(centerPos, passing_object_id);
    std::vector<std::shared_ptr<Session>> targets;
    for (auto& player : adjacentPlayers)
    {
        if (auto session = player->session.lock())
            targets.push_back(session);
    }

    Broadcast(sendBuffer, targets);
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

    //Key : Obejct_templatedId, Val: count Quest Event 용
    std::unordered_map<int32, int32> Object_and_count;

    // JobQueue::Execute()가 끝난 직후이므로 락 없이 안전하게 순회 가능합니다.
    for (auto& item : _coreroom->_objects)
    {
        if (item.second->GetType() == GameObjectType::Monster)
        {
            monstersToUpdate.push_back(std::static_pointer_cast<Monster>(item.second));

        }
        else if (item.second->GetType() == GameObjectType::Projectile)
        {
            projectilesToUpdate.push_back(std::static_pointer_cast<Projectile>(item.second));
            continue;
            // Projectile 의 소멸에 대해서는 방송할 필요가 없음, 오히려 투사체의 상태를 실시간으로 통신하지 않으려는 구조임, 생성되는 순간만 통보하고 끝
            // 데이터 기반으로 하기 위해서 bool 변수 하나를 gameObject 객체에 추가해서 데이터 로드할때 혹은 오브젝트 생성할때 이 오브젝트가 죽을때 방송 대상이 되는지를
            // 체크하는 방법도 있을 듯, 기획에 따라 투사체이지만 소멸할때 클라에 알려주고 싶을 수도 있기 때문
             
        }
        // [수정] GameObject가 아닌 생명체(Player, Monster)일 경우에만 상태를 체크하도록 캐스팅
        if (item.second->GetType() == GameObjectType::Player || item.second->GetType() == GameObjectType::Monster)
        {
            auto creature = std::static_pointer_cast<Creature>(item.second);
            if (creature && creature->GetState() == CreatureState::OnDead)
            {
                deadpkt.add_dead_object_id_list(creature->GetObjectId());

                //QuestEvent용
                Object_and_count[creature->GetTemplateId()] = Object_and_count[creature->GetTemplateId()] + 1;

                creature->SetState(CreatureState::Dead);
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

        //Quest 이벤트 전파(통신을 하는건 아님)
        for (const auto& [objectid, object] : _coreroom-> _objects)
        {
            if (object->GetType() == GameObjectType::Player)
            {
                PlayerPtr player = std::dynamic_pointer_cast<Player>(object);

                if (player)
                {
                    for (const auto& [objecttemplateid, count] : Object_and_count)
                    {
                        QuestEvent killevent;
                        killevent.questeventtype = QuestEventType::KillObejct;
                        killevent.questtype = QuestType::Kill;
                        killevent.target_Obejct_templatedId = objecttemplateid;
                        killevent.Count = count;


                        player->GetQuestComponent()->HandleEvent(killevent);
                    }
                    
                }
            }
        }
        
    }
    
}


void Room::UpdateObjectGrid(GameObjectPtr go, Vector3 oldPos, Vector3 newPos)
{
    _coreroom->UpdateObjectGrid(go, oldPos, newPos);
}

MapPtr Room::GetMapptr()
{
    return _coreroom->GetMapptr();
}

int32 Room::GetMapId()
{
    return GetMapptr() -> GetMapId();
}


