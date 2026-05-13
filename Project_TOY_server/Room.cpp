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
#include "DataContents.h"
#include "ObjectManager.h"
#include "JobSerializer.h"

Room::Room(int32 roomId, int32 mapId) :JobQueue(&GJobSerializer) , _Selfroomid(roomId)
{
    // ���� ������ �� �� �Ŵ����� ���� ���� �Ҵ�޽��ϴ�.
    // GMapManager�� ���� Ȥ�� �̱������� ����Ǿ� �־�� �մϴ�.
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
    //���� ���� �޸𸮿� ���� ����(�� ���� ó��)
    {
        std::lock_guard<std::mutex> lock(_lock);
        _objects[go->GetObjectId()] = go;
        go->SetroomId(_Selfroomid);

        auto [cellX, cellZ] = GetSectorPos(Vector3::PosInfoToVector3(go->Getpos())); // ��ǥ�� �ε��� ����
        _sectors[cellZ][cellX].insert(go);

    }
    //���ο��� ���� ���� �� ��ǥ �˸� (SC_ENTER_GAME)
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
        *enterPkt.mutable_pos_info() = *(player->Getpos()); // ������ ������ ��ǥ

        enterPkt.set_templeteid(go->GetTempleteId()); //�ڵ鷯���� ������ ���ø� ���̵�
        
        enterPkt.set_mapid(_map->GetMapId()); //Ŭ�� ������ �� Id
        auto sendBuffer = ServerUtils::MakeSendBuffer(enterPkt, Protocol::PKT_SC_ENTER_GAME);
        
        if (!sendBuffer) return;
        
        player->session.lock()->Send(sendBuffer);
    }
    else if (go->GetType() == GameObjectType::Monster)
    {
        MonsterPtr monster= std::static_pointer_cast<Monster>(go);
        SpawnBroadcast(monster);
    }
    // ���� : ���� ��Ŷ�� Room::Enter ���� �������� �ʰ� ���߿� ���� ��Ŷ�� �����ؼ� ������
    
}

void Room::EnterMonsters(const std::vector<MonsterPtr>& monsters)
{
    //���� ���� �޸𸮿� ���� ����(�� ���� ó��)
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
            return; // �̹� �����ų� ���� ��ü�� ����

        GameObjectPtr go = it->second;

        // �׸��忡�� ����
        auto [cellX, cellZ] = GetSectorPos(Vector3::PosInfoToVector3(go->Getpos()));
        _sectors[cellZ][cellX].erase(go);

        _objects.erase(playerId); // 1. ���� ���� ��Ͽ��� ����
    }

    // 2. Ÿ�ε鿡�� �� ������ �������� �˸� (SC_DESPAWN)
    Protocol::SC_PLAYER_DESPAWN despawnPkt;
    despawnPkt.add_player_id(playerId);
    

    auto sendBuffer = ServerUtils::MakeSendBuffer(despawnPkt, Protocol::PKT_SC_PLAYER_DESPAWN);
    if (!sendBuffer) return;
    
    Broadcast(sendBuffer, playerId); // ������ �̹� �������Ƿ� ����


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
    Broadcast(sendBuffer,targets);//target�� �ִ� ������ Braodcast�� ȣ����
    
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
    // Capture self to keep the room alive during job execution
    RoomPtr self = std::static_pointer_cast<Room>(shared_from_this());
    uint32 bufferSize = sendBuffer->Size();


    this->Push([self, sendBuffer, targets, bufferSize]() {
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
    //(player ����)Ÿ�ε鿡�� ���� �˸� (SC_PLAYER_SPAWN ��ε�ĳ��Ʈ)
    {
        Protocol::SC_PLAYER_SPAWN spawnPkt;
              
        Protocol::SpawnInfo* spawnInfo = spawnPkt.add_players_spawn_info();

        spawnInfo->mutable_spawnposinfo()->CopyFrom(*player->Getpos());
        spawnInfo->set_templeteid(player->GetTempleteId());


        if (spawnPkt.players_spawn_info_size() > 0) // �����Ͱ� ���� ���� ����
        {
            
            SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(spawnPkt, Protocol::PKT_SC_PLAYER_SPAWN);

            if (!sendBuffer) return;

            // ���� ������ ��ο��� ���� (������ ���� Broadcast �Լ� Ȱ��)
            Broadcast(sendBuffer, player->GetObjectId());
        }

    }

    //(player ����)������ ���� ������Ʈ���� �˸� (SC_PLAYER_SPAWN ��� ����)
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

    if (monsterspawn_pkt.monsters_spawn_info_size() > 0) // �����Ͱ� ���� ���� ����
    {

        auto sendBuffer = ServerUtils::MakeSendBuffer(monsterspawn_pkt, Protocol::PKT_SC_MONSTER_SPAWN);

        if (sendBuffer) {
            //��ü���� ����
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
                //�ֺ��� ��� (���� ���� ������ BroadcastAround�� ������ passing_object_id Ȱ��)
                BroadcastAround(sendBuffer, Vector3::PosInfoToVector3(gameobjects[0]->Getpos()), gameobjects[0]->GetObjectId());
            }
            else 
            { 
                //TODO BroadcastAround �� �ϳ��� ��ġ �������� �ϴ°� �ƴ϶� �������� ��ġ�� ������� ������ ������ ����ϵ��� �����ε��ʿ�
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
    RoomPtr self = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([self, player, posInfo]() {
    // 1. [����] ���� ��ġ�� �� ��ġ�� �Ÿ� ���̰� �ʹ� ũ�� �����ϰų� ���� (�� ����)
    Vector3 currentPos = Vector3::PosInfoToVector3(player->Getpos());
    Vector3 newPos = Vector3(posInfo.x(), posInfo.y(), posInfo.z());

    uint64 currentTick = ::GetTickCount64(); // ���� ���� �ð� (Windows ����)

    // ó������ 0�� �� �����Ƿ� ���� ó��
    if (player->lastMoveTick == 0) player->lastMoveTick = currentTick - 100;

    // deltaTime ��� (�� ������ ��ȯ)
    float deltaTime = (currentTick - player->lastMoveTick) / 1000.0f;
    player->lastMoveTick = currentTick; // ���� �ð��� ���� ������ ���� ����


    float dist = Vector3::Distance(currentPos, newPos);
    float maxAllowedDist = player->GetSpeed() * deltaTime * 1.2f; // �������� 20%

    //�ӵ� ����
    if (dist > maxAllowedDist) {
        // �ʹ� �ָ� �̵��� (��Ŷ ���� Ȥ�� ���� ��ġ ����)
        std::cout << "������ �̵� ��û �ν�" << std::endl;

        self->SendMoveResync(player);
        return;
    }
    
    // ���� ���� 
    MapPtr currentmap = self->GetMapptr();
    if (currentmap != nullptr)
    {
        if (currentmap->CanGo(newPos) == false)
        {
            // �浹 �߻�! Ŭ���̾�Ʈ���� ���� ��ġ ���� ��Ŷ ����
            self->SendMoveResync(player);
            return;
        }
    }

    //[����] �׸��� ������Ʈ
    {
        self->UpdateObjectGrid(player, currentPos, newPos);
    }


    // 2. [����] ���� �޸𸮿� �÷��̾� ��ġ ���� ������Ʈ
    player->Setpos(posInfo);

    // 3. [����] �� ���� �ٸ� �����鿡�� �̵� ��� ��ε�ĳ��Ʈ
    
    self->BroadcastMove(player);
    
    });
}

void Room::HandleSkillForPlayer(PlayerPtr player, Protocol::CS_SKILL& pkt)
{
    int32 skillid = pkt.skill_id();
    int32 targetObjectId = pkt.has_target() ? pkt.target().target_object_id() : -1;
    bool hasDestPos = pkt.has_dest_pos();
    Vector3 targetPos = hasDestPos ? Vector3(pkt.dest_pos().x(), pkt.dest_pos().y(), pkt.dest_pos().z()) : Vector3(0, 0, 0);

    // Job �ȿ��� �����ϰ� �� �ڽ��� �����ϱ� ���� shared_ptr�� ĳ����
    RoomPtr self = std::static_pointer_cast<Room>(shared_from_this());
    
    // ���� ���� ������ �����ϰ� ���� �����忡�� ���������� ó���ϱ� ���� JobQueue�� Push
    this->Push([self, player, targetObjectId, targetPos, skillid]() {
        
        GameObjectPtr targetobj = nullptr;
        if (targetObjectId != -1)
        {
            // Job�� ������ ����Ǵ� ������ �ֽ� ������Ʈ ���¸� ã��
            targetobj = GObjcetManager.Find(targetObjectId);
        }
        
        self->HandleSkill(player, targetobj, targetPos, skillid);
    });
}

void Room::HandleSkillForMonster(MonsterPtr monster, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid)
{
    RoomPtr self = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([self, monster, targetobj, targetPos, skillid]() {
        // ���� ��ų ��� �ÿ��� ���� ��ų ó�� ����(HandleSkill)�� �¿�ϴ�.
        self->HandleSkill(monster, targetobj, targetPos, skillid);
    });
}

void Room::HandleSkill(GameObjectPtr SKillUser, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid)
{
    // [����] �÷��̾ ��� -> ��� ��ų�� ���� �ִ� ���� ������Ʈ�� ����ϴ� �޼ҵ�
    // TODO: [DB������ �߰� �ʿ�][�� ����]_skillCooltimes �� �̿��ϴ� �ƴϸ� �ٸ� �޸𸮿����� �߰��ϴ� �ؼ� ��ų�� ��¥ �� ĳ���Ͱ� ���� �ִ� ��ų���� üũ�ϴ� �����ʿ�

    const SkillData* skilldata = DataManager::GetInstance().GetSkill(skillid);
    int64 now = GetTickCount64();
    int64 lastUsed = SKillUser->_skillCooltimes[skilldata->id];
    int64 coolTime = skilldata->coolTime * 1000; // �� ������ ms�� ��ȯ

    if (now - lastUsed < coolTime) {
        // ���� ��Ÿ�� ��! ��û ���� Ȥ�� ���� ��Ŷ ����
        return;
    }

    // ���� ��� �� ��� ���� ����
    SKillUser->_skillCooltimes[skilldata->id] = now;

    // 3. �ڽ�Ʈ(���� ��) üũ �� ����
    // (Player Ŭ������ GetStat(), SetStat() Ȥ�� ���� ���� ������ ����� �ִٰ� ����)
    if (skilldata->costType == CostType::Mana) {
        int32 currentMp = SKillUser->GetCurrentMp(); // �÷��̾� ���� MP ��������
        int32 requiredMp = skilldata->cost; // ����: ��ų �����Ϳ� �ڽ�Ʈ ��ġ�� �߰��ϸ� �� �����ϴ�.


        if (SKillUser->UseMp(currentMp - requiredMp) == false)
        {
            //���� ����, ���ο��� �޽������� ������ ��������
            return;
        }

    }

    // 4. ���� ��� �� ��� ���� ����
    SKillUser->_skillCooltimes[skilldata->id] = now;

    // 5. ��ų Ÿ�Ժ� �ǰ� ����
    bool isHit = false;

    switch (skilldata->skillType)
    {
    case SkillType::Melee:
    {
        for (auto obj : _objects)
        {
            GameObjectPtr target = obj.second;
            if (target && target->GetObjectId() != SKillUser->GetObjectId()) {
                // �Ÿ� ��� (Vector3::Distance)
                float dist = Vector3::Distance(Vector3::PosInfoToVector3(SKillUser->Getpos()), Vector3::PosInfoToVector3(target->Getpos()));

                // ��Ÿ� ���� (�ణ�� ���� �ο�: 0.5f)
                if (dist <= skilldata->range + 0.5f) {
                    isHit = true;

                    // ������ ��� �� ����(���� �޸� ������Ʈ)
                    int32 damage = skilldata->damage + SKillUser->GetAttack(); // ��ų������ + ĳ���Ͱ��ݷ� ���� ����
                    target->OnAttacked(damage); // ����� HP�� ��� �Լ� ȣ��

                    UpdateHPToOthers(target, SKillUser, damage, SKillUser->Getpos_As_Vector3());

                    std::cout << "[Melee Hit] " << SKillUser->GetName() << " -> " << target->GetName() << " (Damage: " << damage << ")" << std::endl;
                }
            }
        }

    }
    break;

    case SkillType::Projectile:
        // ����ü�� ��� �ǰ��� �ƴ϶� Projectile ��ü�� �����Ͽ� Update���� ó��
        // SpawnProjectile(player, skilldata, pkt.target_pos());
        break;

    case SkillType::Dash:
        // �̵� ���� �������� Ȯ�� �� ��ǥ ���� ����
        break;
    }

    // 6. ��� ��ε�ĳ��Ʈ (�ֺ� ��ο��� �ִϸ��̼� �˸�)
    if (SKillUser->GetType() == GameObjectType::Player)
    {
        PlayerPtr player = std::static_pointer_cast<Player>(SKillUser);
        //���ο��� MP ��ȭ ��Ŷ ����
        UpdateMPToSelf(player);
    }
    

    Protocol::SC_SKILL resPkt;
    resPkt.set_object_id(SKillUser->GetObjectId());
    resPkt.set_skill_id(skilldata->id);

    bool Istargetpos = (skilldata->targetType == SkillTargetType::positionTarget);

    if (targetobj)
    {
        // mutable_target()�� ���� TargetobjectInfo ��ü�� �����͸� ��� �� ����
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

//���ο��� MP ���� ��Ŷ ���� 
void Room::UpdateMPToSelf(PlayerPtr player)
{
    
    Protocol::SC_CHANGE_MP mp_Change_pkt;
    mp_Change_pkt.set_object_id(player->GetObjectId());
    mp_Change_pkt.set_current_mp(player->GetCurrentMp());
    auto sendBuffer = ServerUtils::MakeSendBuffer(mp_Change_pkt, Protocol::PKT_SC_CHANGE_MP);

    if (!sendBuffer) return;

    SendTo(player, sendBuffer);

}

//���ο��� HP ���� ��Ŷ ����
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
    //Mp ��ȭ ���
    Protocol::SC_CHANGE_MP mp_changed_pkt;
    mp_changed_pkt.set_object_id(target->GetObjectId());
    mp_changed_pkt.set_current_mp(target->GetCurrentHp());
    
    auto sendBuffer = ServerUtils::MakeSendBuffer(mp_changed_pkt, Protocol::PKT_SC_CHANGE_MP);
    if (!sendBuffer) return;

    BroadcastAround(sendBuffer, broadcastcenter);

}


void Room::UpdateHPToOthers(GameObjectPtr target, GameObjectPtr attacker, int damage, Vector3 broadcastcenter)
{
    //Hp ��ȭ ���
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
    // [����] �ܺ� ������(ConsoleThread ��)���� ȣ��� ���� ����� 
    // ���� ������ ���ٷ� ���� JobQueue�� �ֽ��ϴ�.
    RoomPtr self = std::static_pointer_cast<Room>(shared_from_this());

    this->Push([self, NumOfMonster, templatedId]() {
        std::vector<MonsterPtr> monsters;
        for (int i = 0; i < NumOfMonster; i++)
        {
            MonsterPtr monster = std::static_pointer_cast<Monster>(
                GObjcetManager.Create(GameObjectType::Monster, nullptr, templatedId)
            );

            // ��ǥ ���� �� ���� ����
            monster->Set_x(10.0f + i * 2.0f);
            monster->Set_z(10.0f);

            monsters.push_back(monster);
        }

        // EnterMonsters ���ο����� ���� ��� �����͸� �����ϹǷ� 
        // Job ���ο��� ����Ǵ� ���� �����մϴ�.
        self->EnterMonsters(monsters);

        std::cout << "[Job] MonsterSpawn completed: " << NumOfMonster << " monsters." << std::endl;
        });
}

PlayerPtr Room::GetNearestPlayer(Vector3 pos, float maxRange)
{
    PlayerPtr nearestPlayer = nullptr;
    float bestDistSq = maxRange * maxRange; // ������ ������ ���ϱ� ���� �Ÿ��� ���� ���

    // 1. �ֺ� 9�� �׸��忡 �ִ� �÷��̾� ����Ʈ�� ������ (�̹� ������ �Լ� Ȱ��)
    std::vector<PlayerPtr> adjacentPlayers = GetAdjacentPlayers(pos);

    // 2. ����Ʈ�� ��ȸ�ϸ� ���� ����� �÷��̾� Ž��
    for (const PlayerPtr& player : adjacentPlayers)
    {
        // ��� ������ �÷��̾�� ���� (�ʿ� ��)
        if (player->GetState() == CreatureState::Dead)
            continue;

        Vector3 playerPos = Vector3::PosInfoToVector3(player->Getpos());

        // �� ���� ������ �Ÿ� ���� ��� (sqrt�� �� �Ἥ ���� �̵�)
        float distSq = Vector3::DistanceSquared(pos, playerPos);

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            nearestPlayer = player;
        }
    }

    return nearestPlayer;
}

//��ġ �ǰ���
void Room::SendMoveResync(PlayerPtr player)
{
    Protocol::PosInfo beforepos = *(player->Getpos());
    
    this->Push([beforepos, player]() {
    // 1. ������ ����� '����' ��ǥ�� ���� ��Ŷ ����
    Protocol::SC_MOVING resPkt;

    Protocol::PosInfo* resPos = resPkt.add_pos_info();
    resPos->CopyFrom(beforepos); // ������Ʈ ���� ���� ��ǥ
    resPos->set_state((int)CreatureState::Idle);

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_MOVING);

    if (!sendBuffer) return;

    // 2. �ش� �������Ը� ������ ���� (��ġ �ǰ���)
    if (auto playersession = player->session.lock())
        playersession->Send(sendBuffer);
    });
}

std::pair<int, int> Room::GetSectorPos(Vector3 pos)
{
    // 1. ���� ���� Ÿ�� ��ǥ�� ��ȯ
    int cellX = static_cast<int>(std::floor((pos.x - _minX) / _cellSize));
    int cellZ = static_cast<int>(std::floor((pos.z - _minZ) / _cellSize));

    // 2. ���� ��ǥ�� ���� ũ��� ������ ���� �ε��� ����
    int sectorX = cellX / _sectorSize;
    int sectorZ = cellZ / _sectorSize;

    // ���� ����
    sectorX = std::clamp(sectorX, 0, _sectorCountX - 1);
    sectorZ = std::clamp(sectorZ, 0, _sectorCountZ - 1);

    return { sectorX, sectorZ };
}

//���� �÷��̾� ���� (Interest Management)
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
    // ��� �÷��̾ �ƴ϶� ������ �÷��̾�Ը� ����
    std::vector<std::shared_ptr<Session>> targets = GetAdjacentPlayersSessions(centerPos, passing_object_id);

    
    Broadcast(sendBuffer, targets);
}

void Room::InitGridData(const MapData* mapdata)
{
    _cellSize = mapdata->CellSize;
    _minX = mapdata->MinX;
    _minZ = mapdata->MinZ;
    // ���� ���� ��� (��ü ����/���� Ÿ�� �� / ���� ũ��)
    _sectorCountX = (mapdata->width / _sectorSize) + 1;
    _sectorCountZ = (mapdata->height / _sectorSize) + 1;

    _sectors.assign(_sectorCountZ, std::vector<std::set<GameObjectPtr>>(_sectorCountX));
}



void Room::Execute() 
{
    
    // 1. Execute queued jobs for this room
    JobQueue::Execute();

    // 2. Collect monsters under lock to avoid concurrent-modification while iterating
    
    //���⼭ ��� ���� ��Ŷ ���
    Protocol::SC_MONSTER_DEAD deadpkt; bool anyDead = false;

    std::vector<MonsterPtr> monstersToUpdate;
    {
        std::lock_guard<std::mutex> lock(_lock);
        for (auto& item : _objects)
        {
            if (item.second->GetType() == GameObjectType::Monster)
            {
                monstersToUpdate.push_back(std::static_pointer_cast<Monster>(item.second));
            }
            if (item.second->GetState() == CreatureState::OnDead)
            {
                
                deadpkt.add_dead_object_id_list(item.second->GetObjectId());
                item.second->SetState(CreatureState::Dead);
                anyDead = true; // ���� ���Ͱ� ���� ���� �÷��� Ȱ��ȭ
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
    if (anyDead) 
    {
        SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(deadpkt, Protocol::PKT_SC_MONSTER_DEAD);

        if (!sendBuffer) return;

        Broadcast(sendBuffer);

        std::cout << "SomeOne is dead, boradcasting" << std::endl;
    }
    
}