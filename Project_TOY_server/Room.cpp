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


Room::Room(int32 roomId, int32 mapId) : _Selfroomid(roomId)
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
        }
        Protocol::SC_ENTER_GAME enterPkt;     
        *enterPkt.mutable_pos_info() = *(player->Getpos()); // 서버가 결정한 좌표

        enterPkt.set_templeteid(go->GetTempleteId()); //핸들러에서 결정된 템플릿 아이디
        
        enterPkt.set_mapid(_map->GetMapId()); //클라에 보내줄 맵 Id
        auto sendBuffer = ServerUtils::MakeSendBuffer(enterPkt, Protocol::PKT_SC_ENTER_GAME);
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

        spawnInfo->mutable_spawnposinfo()->CopyFrom(*player->Getpos());
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
        Protocol::SC_PLAYER_SPAWN playerspawnPkt;
        Protocol::SC_MONSTER_SPAWN monsterspawnPkt;

        // 방의 모든 오브젝트를 순회하며 나를 제외한 정보를 패킷에 추가
        for (auto& pair : _objects)
        {
            if (pair.second->GetType() == GameObjectType::Player) 
            {
                Protocol::SpawnInfo* spawnInfo = playerspawnPkt.add_players_spawn_info();

                //해당 슬롯에 기존 오브젝트의 위치 정보를 복사 
                spawnInfo->mutable_spawnposinfo()->CopyFrom(*(pair.second->Getpos()));
                spawnInfo->set_templeteid(pair.second->GetTempleteId());
            }
            else if (pair.second->GetType() == GameObjectType::Monster) 
            {
                Protocol::SpawnInfo* spawnInfo = monsterspawnPkt.add_monsters_spawn_info();

                //해당 슬롯에 기존 오브젝트의 위치 정보를 복사 
                spawnInfo->mutable_spawnposinfo()->CopyFrom(*(pair.second->Getpos()));
                spawnInfo->set_templeteid(pair.second->GetTempleteId());
            }

        }

        if (playerspawnPkt.players_spawn_info_size() > 0) // 데이터가 있을 때만 전송
        {
            // 패킷 시리얼라이즈 및 전송
            auto sendBuffer = ServerUtils::MakeSendBuffer(playerspawnPkt, Protocol::PKT_SC_PLAYER_SPAWN);

            if (auto s = player->session.lock())
            {
                
                s->Send(sendBuffer);
            }
        }
        else if (monsterspawnPkt.monsters_spawn_info_size() >0)
        {
            // 패킷 시리얼라이즈 및 전송
            auto sendBuffer = ServerUtils::MakeSendBuffer(playerspawnPkt, Protocol::PKT_SC_MONSTER_SPAWN);

            if (auto s = player->session.lock())
            {
                s->Send(sendBuffer);
            }
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

        SendBufferPtr sendBuffer = ServerUtils::MakeSendBuffer(monsterspawn_pkt, Protocol::PKT_SC_MONSTER_SPAWN);

        //전체에게 전송
        Broadcast(sendBuffer);
    }

}
void Room::SpawnBroadcast(MonsterPtr monster)
{
    SpawnBroadcast({monster});
}

void Room::BroadcastMove(GameObjectPtr go)
{
    if (go == nullptr) return;

    Protocol::SC_MOVING movePkt;
    movePkt.mutable_pos_info()->CopyFrom(*go->Getpos()); // 현재 오브젝트의 좌표 정보 복사

    auto sendBuffer = ServerUtils::MakeSendBuffer(movePkt, Protocol::PKT_SC_MOVING);

    //주변에 방송 (본인 제외 로직은 BroadcastAround에 구현된 passing_object_id 활용)
    BroadcastAround(sendBuffer, Vector3::PosInfoToVector3(go->Getpos()), go->GetObjectId());
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

    //[갱신] 그리드 업데이트
    {
        UpdateObjectGrid(player, currentPos, newPos);
    }


    // 2. [갱신] 서버 메모리에 플레이어 위치 정보 업데이트
    player->Setpos(pkt.pos_info());

    // 3. [전달] 방 안의 다른 유저들에게 이동 사실 브로드캐스트
    Protocol::SC_MOVING resPkt;
    auto* resPos = resPkt.mutable_pos_info();
    resPos->CopyFrom(*(player->Getpos()));

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_MOVING);
   
    BroadcastAround(sendBuffer, Vector3::PosInfoToVector3(player->Getpos()), player->GetObjectId());// 자기자신은 제외한 인접 플레이어들에게 방송

    
    //Loging
    /*std::cout << "RoomId: " << _Selfroomid << std::endl
        << "object Id : " << resPos->object_id() << "HandleMove : (" << resPos->x() << resPos->y() << resPos->z() << ")" << std::endl;*/

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
                    BroadcastAround(sendBuffer,player->Getpos_As_Vector3());

                    std::cout << "[Melee Hit] " << player->GetName() << " -> " << target->GetName() << " (Damage: " << damage << ")" << std::endl;
                }
            }
        }

     
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
    BroadcastAround(sendBuffer, player->Getpos_As_Vector3());
}

void Room::MonsterSpawn(int32 NumOfMonster)
{
    // 테스트용 몬스터 생성 및 입장 로직 jobqueue로 수정해야함
    std::vector<MonsterPtr> monsters;
    for (int i = 0; i < NumOfMonster; i++)
    {
        MonsterPtr monster = std::static_pointer_cast<Monster>(
            GObjcetManager.Create(GameObjectType::Monster, nullptr, 2) //몬스터는 session이 필요없기 때문에 nullptr로 전달
        );
        monster->Set_x(10.0f + i * 2.0f);
        monster->Set_z(10.0f);

        monsters.push_back(monster);
    }
    EnterMonsters(monsters);
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
    auto* resPos = resPkt.mutable_pos_info();
    resPos->CopyFrom(*(player->Getpos())); // 업데이트 전의 서버 좌표
    resPos->set_state((int)CreatureState::Idle);

    auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_MOVING);

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
std::vector<PlayerPtr> Room::GetAdjacentPlayers(Vector3 pos, int32 passing_object_id)
{
    std::vector<PlayerPtr> adjacentPlayers;
    auto [cellX, cellZ] = GetSectorPos(pos);

    // 주변 9개 칸 (자신 포함) 순회
    for (int dz = -1; dz <= 1; ++dz)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            int nx = cellX + dx;
            int nz = cellZ + dz;

            // 유효한 그리드 범위인지 확인
            if (nx >= 0 && nx < _sectorCountX && nz >= 0 && nz < _sectorCountZ)
            {
                for (auto& go : _sectors[nz][nx])
                {
                    if (go->GetType() == GameObjectType::Player && go->GetObjectId() != passing_object_id)
                    {
                        adjacentPlayers.push_back(std::static_pointer_cast<Player>(go));
                    }
                }
            }
        }
    }
    return adjacentPlayers;
}
void Room::UpdateObjectGrid(GameObjectPtr go, Vector3 oldPos, Vector3 newPos)
{
    // 1. 이전 좌표와 새 좌표의 그리드 인덱스 계산
    auto oldGridPos = GetSectorPos(oldPos);
    auto newGridPos = GetSectorPos(newPos);

    // 2. 만약 같은 칸 내에서의 이동이라면 갱신할 필요 없음
    if (oldGridPos == newGridPos)
        return;

    // 3. 이전 그리드에서 제거
    {
        int oldX = oldGridPos.first;
        int oldZ = oldGridPos.second;
        _sectors[oldZ][oldX].erase(go);
    }

    // 4. 새로운 그리드에 추가
    {
        int newX = newGridPos.first;
        int newZ = newGridPos.second;
        _sectors[newZ][newX].insert(go);
    }

    // [참고] 여기서 추가로 처리할 수 있는 로직:
    // 만약 플레이어라면, 새로 진입한 그리드 주변에만 본인의 정보를 브로드캐스트 하도록 유도 가능
}

void Room::BroadcastAround(SendBufferPtr sendBuffer, Vector3 centerPos, int32 passing_object_id)
{
    // 모든 플레이어가 아니라 인접한 플레이어에게만 보냄
    auto targets = GetAdjacentPlayers(centerPos, passing_object_id);

    for (PlayerPtr player : targets)
    {
        if (auto session = player->session.lock())
        {
            session->Send(sendBuffer);
        }
    }
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


void Room::Update()
{
    // 몬스터 AI 및 이동 업데이트
    // _objects를 순회하며 몬스터만 골라내거나, 별도의 _monsters 리스트를 관리하면 더 빠릅니다.
    for (auto& item : _objects)
    {

        GameObjectPtr go = item.second;

        if (go->GetType() == GameObjectType::Monster)
        {
            auto monster = std::static_pointer_cast<Monster>(go);
            monster->UpdateAction();
        }
    }

    // TODO: 프로젝트 TOY 서버의 다른 업데이트 로직 (환경 변화 등)
}