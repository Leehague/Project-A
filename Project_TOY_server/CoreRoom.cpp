#include "CoreRoom.h"
#include "Vector3.h"
#include "Projectile.h"
#include "Player.h"
#include <windows.h> 
#include "MapManager.h"
#include "Map.h"
#include "DataManager.h"
#include "GameObject.h"

CoreRoom::CoreRoom(int32 mapId, bool& maploadsuccess)
{
    // 방이 생성될 때 맵 매니저를 통해 맵을 할당받습니다.
    // GMapManager는 전역 혹은 싱글톤으로 선언되어 있어야 합니다.
    _map = GMapManager.LoadMap(mapId);

    if (_map == nullptr)
    {
        maploadsuccess = false;
        return;
    }
    InitGridData(_map->GetMapData());
    maploadsuccess = true;
}

bool CoreRoom::HandleMove(PlayerPtr player, const Core::PosInfo& posinfo)
{
    if (player == nullptr)
    {
        std::cout << "HandleMove : player ptr is null" << std::endl;
        return false;
    }

    Vector3 currentPos = Vector3::PosInfoToVector3(player->Getpos());
    Vector3 newPos = Vector3(posinfo.x, posinfo.y, posinfo.z);

    uint64 currentTick =TimeManager::GetTickCount64(); // 현재 서버 시간 (Windows 기준)

    // 처음에만 0일 수 있으므로 예외 처리
    if (player->GetlastMoveTick() == 0) { player->SetlastMoveTick(currentTick - 100); }

    // deltaTime 계산 (초 단위로 변환)
    float deltaTime = (currentTick - player->GetlastMoveTick()) / 1000.0f;
    player->SetlastMoveTick(currentTick); // 현재 시간을 다음 검증을 위해 저장

    float dist = Vector3::XZDistance(currentPos, newPos);
    float maxAllowedDist = player->GetSpeed() * deltaTime * 1.2f; // 오차범위 20%


    //속도 검증
    if (dist > maxAllowedDist) {
        // 너무 멀리 이동함 (패킷 무시 혹은 강제 위치 복구)
        std::cout << "Abnormal move request detected" << std::endl;

        return false;
    }

    // 지형 검증
    MapPtr map = this->GetMapptr();
    if (map != nullptr)
    {
        //TEMP
        float Ypadding = 0.5f;
        if (posinfo.state == CreatureState::Jump || posinfo.state == CreatureState::Fall)
        {
            Ypadding = 5.0f; //하드코딩 했지만 실제로는 '점프력' 같은 최대 점프 높이를 받아 와야함
        }
        


        if (map->CanGo(newPos , Ypadding) == false)
        {
            return false;
        }
    }

    //[갱신] 그리드 업데이트
    {
        this->UpdateObjectGrid(player, currentPos, newPos);
    }

    // 2. [갱신] 서버 메모리에 플레이어 위치 정보 업데이트
    player->Setpos(posinfo);

    return true;

}


bool CoreRoom::HandleSkill(CreaturePtr SKillUser, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid, std::vector<Core::DamageResult>* results, bool* ishit, std::vector<GameObjectPtr>* spawnedObjects)
{
    const SkillData* skilldata = DataManager::GetInstance().GetSkill(skillid);
    if (skilldata == nullptr)
    {
        return false;
    }
    int64 now = TimeManager::GetTickCount64();
    int64 lastUsed = SKillUser->GetSkillCoolTime(skilldata->id);
    int64 coolTime = skilldata->coolTime * 1000; // 초 단위를 ms로 변환

    


    if (now - lastUsed < coolTime) {
        // 아직 쿨타임 중! 요청 무시 혹은 에러 패킷 전송
        //std::cout << "it's cooltime" << std::endl;
        return false;
    }


    // 3. 코스트(마나 등) 체크 및 차감
    // (Player 클래스에 GetStat(), SetStat() 혹은 직접 접근 가능한 멤버가 있다고 가정)
    if (skilldata->costType == CostType::Mana) {

        int32 requiredMp = skilldata->cost; // 예시: 스킬 데이터에 코스트 수치를 추가하면 더 좋습니다.

        if (SKillUser->UseMp(requiredMp) == false)
        {
            //마나 부족, 본인에게 메시지등을 보낼수 있을것임
            //std::cout << "Mana is not enough " << std::endl;
            return false;
        }

    }

    // 검증 통과 후 사용 시점 갱신
    SKillUser->SetSkillCoolTime(skilldata->id, now);

    // 5. 스킬 타입별 피격 판정
    switch (skilldata->skillType)
    {
    case SkillType::Melee:
    {
        for (auto obj : _objects)
        {
            GameObjectPtr target = obj.second;
            CreaturePtr creaturetarget = std::dynamic_pointer_cast<Creature>(target);
            if (target && target->GetObjectId() != SKillUser->GetObjectId()) {

                Vector3 targetpos = target->Getpos_As_Vector3();
                Vector3 skilluserpos = SKillUser->Getpos_As_Vector3();

                //[추가] 각도 검증
                Vector3 dir = targetpos- skilluserpos;
                float diffyaw = (SKillUser->Getpos()->yaw)-Vector3::CalculateYaw(dir);

                if (std::abs(diffyaw) > 20) { continue; }

                // 거리 계산 (Vector3::Distance)
                float dist = Vector3::Distance(skilluserpos, targetpos);

                // 사거리 검증 (약간의 마진 부여: 0.5f)
                if (dist <= skilldata->range + 0.5f) {
                    if (ishit) *ishit = true; //피격판정


                    if (creaturetarget == nullptr) { continue; }

                    //이 아래로는 데미지 계산등 Creature 에게만 적욜할 로직이 들거마면 됨. 

                    // 데미지 계산 및 적용(서버 메모리 업데이트)
                    int32 damage = skilldata->damage + SKillUser->GetAttack(); // 스킬데미지 + 캐릭터공격력 수정 가능
                    creaturetarget->OnAttacked(damage); // 대상의 HP를 깎는 함수 호출

                    //results 에 타겟 추가
                    Core::DamageResult result;
                    result.damage = damage;
                    result.target = target;

                    if (results) results->push_back(result);
                    //UpdateHPToOthers(creaturetarget, SKillUser, damage, SKillUser->Getpos_As_Vector3());
                    //이건 room 에서 해줄것

                    std::cout << "[Melee Hit] " << SKillUser->GetName() << " -> " << creaturetarget->GetName() << " (Damage: " << damage << ")" << std::endl;
                }
            }
        }

    }
    break;

    case SkillType::Projectile:
    {
        std::cout << "Spawn Projectile" << std::endl;
        ProjectilePtr projectile = SpawnProjectile(SKillUser, skilldata, targetPos);
        if (projectile != nullptr)
        {
            if (spawnedObjects) spawnedObjects->push_back(projectile);
        }
        break;
    }
    case SkillType::Dash:
    {
        // 이동 가능 지역인지 확인 후 좌표 강제 갱신
        if (this->GetMapptr()->CanGo(targetPos))
        {
            //이동가능 지역인 경우
            SKillUser->Setpos(targetPos);
        }
        else
        {
            //이동 불가 지역인 경우

        }


        break;
    }
    }

    return true;
}

ProjectilePtr CoreRoom::SpawnProjectile(CreaturePtr attacker, const SkillData* skillData, Vector3 targetPos)
{
    if (attacker == nullptr || skillData == nullptr)
        return nullptr;

    // 1. 투사체 객체 생성 (팩토리 패턴)
    ProjectilePtr projectile;
    CoreRoomPtr self = std::static_pointer_cast<CoreRoom>(shared_from_this());
    if (_objectFactoryCallback)
    {
        // 실제 네트워크 서버 환경: Room이 연결해준 GObjcetManager를 통해 생성
        GameObjectPtr go = _objectFactoryCallback(GameObjectType::Projectile, skillData->projectileId, self);
        projectile = std::static_pointer_cast<Projectile>(go);
    }
    else
    {
        // 파이썬 시뮬레이션 환경: 매니저 없이 자체 생성 및 더미 ID 부여
        static int32 s_simObjectId = 1000000; // 시뮬레이터에서 안 겹치게 사용할 더미 ID
        projectile = std::make_shared<Projectile>(++s_simObjectId, self);
        // (필요 시 projectile->SetTemplateId(skillData->projectileId) 호출)
    }

    // 2. 공격자의 위치로 초기 좌표 설정

    projectile->Setpos(*attacker->Getpos());
    
    // 3. 투사체 방향 및 속도 등 초기화
    projectile->Init(attacker, skillData, targetPos);

    return projectile;
}




std::vector<Core::DamageResult> CoreRoom::UpdateProjectile(std::shared_ptr<Projectile> projectile, bool& ishit)
{
    std::vector<Core::DamageResult> result;
    Vector3 oldPos = Vector3::PosInfoToVector3(projectile->Getpos());

    // 1. 투사체 이동 진행
    projectile->TickMove();

    Vector3 newPos = Vector3::PosInfoToVector3(projectile->Getpos());

    // 2. 지형 충돌 검사 (Map 클래스에 역할 위임)
    if (_map != nullptr && _map->CheckProjectileCollision(newPos))
    {
        projectile->SetState(CreatureState::OnDead);
        return result; // 지형(벽)에 부딪히면 즉시 소멸
    }

    // 3. 그리드 좌표 업데이트
    UpdateObjectGrid(projectile, oldPos, newPos);

    bool isHit = false;
    CreaturePtr attacker = std::dynamic_pointer_cast<Creature>(projectile->GetAttacker());
    //아래에서 attacker null check를 하고 있음


    const SkillData* skillData = projectile->GetSkillData();

    if (attacker && skillData)
    {
        // 4. 사거리 초과 시 투사체 소멸
        if (projectile->GetTraveledDistance() >= skillData->range)
        {
            projectile->SetState(CreatureState::OnDead);
            return result;
        }

        // 5. 주변 섹터의 동적 오브젝트와 타겟 충돌(피격) 검사 (Room 역할)
        auto [cellX, cellZ] = GetSectorPos(newPos);
        //std::vector<GameObjectPtr> targetsToHit;
        {
           
            for (int dz = -1; dz <= 1; ++dz) {
                for (int dx = -1; dx <= 1; ++dx) {
                    int nx = cellX + dx;
                    int nz = cellZ + dz;
                    if (nx >= 0 && nx < _sectorCountX && nz >= 0 && nz < _sectorCountZ) {
                        for (auto& go : _sectors[nz][nx]) {
                            if (go == nullptr || go == projectile || go->GetObjectId() == attacker->GetObjectId()) continue;
                            if (go->GetState() == CreatureState::Dead || go->GetState() == CreatureState::OnDead) continue;

                            if (go->GetType() != GameObjectType::Player && go->GetType() != GameObjectType::Monster) continue;



                            float dist = Vector3::Distance(newPos, Vector3::PosInfoToVector3(go->Getpos()));
                            if (dist <= 1.0f) { // 피격 반경 (임시 1.0f 적용)

                                Core::DamageResult damageresult;
                                damageresult.target = go;
                                damageresult.damage = 0; // 여기서는 혹시모를 버그를 대비하기 위해 0으로 초기화
                                result.push_back(damageresult);
                                isHit = true;
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        return result;
    }

    for (auto& damageresult : result)
    {
        CreaturePtr creaturetarget = std::dynamic_pointer_cast<Creature>(damageresult.target);

        if (creaturetarget == nullptr) { continue; }
        int32 damage = skillData->damage + attacker->GetAttack();
        creaturetarget->OnAttacked(damage);
        damageresult.damage = damage;
        //std::cout << "[Projectile Hit] " << attacker->GetName() << " -> " << creaturetarget->GetName() << " (Damage: " << damage << ")" << std::endl;
    }

    ishit = isHit;

    if (isHit)
    {
        projectile->SetState(CreatureState::OnDead);
    }

     
    return result;
}

PlayerPtr CoreRoom::GetNearestPlayer(Vector3 pos, float maxRange)
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


void CoreRoom::UpdateObjectGrid(GameObjectPtr go, Vector3 oldPos, Vector3 newPos)
{
    auto oldGridPos = GetSectorPos(oldPos);
    auto newGridPos = GetSectorPos(newPos);

    if (oldGridPos == newGridPos)
        return;

    int oldX = oldGridPos.first;
    int oldZ = oldGridPos.second;
    int newX = newGridPos.first;
    int newZ = newGridPos.second;

    // 기존의 lock이 있던 부분, coreroom에서는 신경쓰지 않음. room에서 호출하면서 알아서 jobqueue구조를 이용해 racecondition을 방지할 것
    if (oldZ >= 0 && oldZ < (int)_sectors.size() && oldX >= 0 && oldX < (int)_sectors[oldZ].size())
        _sectors[oldZ][oldX].erase(go);
    if (newZ >= 0 && newZ < (int)_sectors.size() && newX >= 0 && newX < (int)_sectors[newZ].size())
        _sectors[newZ][newX].insert(go);
    
}


std::pair<int, int> CoreRoom::GetSectorPos(Vector3 pos)
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

std::vector <PlayerPtr> CoreRoom::GetAdjacentPlayers(Vector3 pos, int32 passing_object_id)
{
    std::vector <PlayerPtr>adjacentPlayers;

    auto [cellX, cellZ] = GetSectorPos(pos);

    std::vector<GameObjectPtr> snapshot;
    {
        
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

void CoreRoom::InitGridData(const MapData* mapdata)
{
    _cellSize = mapdata->CellSize;
    _minX = mapdata->MinX;
    _minZ = mapdata->MinZ;
    // 섹터 개수 계산 (전체 가로/세로 타일 수 / 섹터 크기)
    _sectorCountX = (mapdata->width / _sectorSize) + 1;
    _sectorCountZ = (mapdata->height / _sectorSize) + 1;

    _sectors.assign(_sectorCountZ, std::vector<std::set<GameObjectPtr>>(_sectorCountX));
}

CreaturePtr CoreRoom::GetNearestCreature(Vector3 pos, float maxRange, int32 excludeObjectId)
{
    CreaturePtr nearestCreature = nullptr;
    float bestDistSq = maxRange * maxRange;

    std::vector<CreaturePtr> adjacentCreatures = GetAdjacentCreatures(pos, excludeObjectId);

    for (const CreaturePtr& creature : adjacentCreatures)
    {
        if (creature->GetState() == CreatureState::Dead || creature->GetState() == CreatureState::OnDead)
            continue;

        Vector3 creaturePos = Vector3::PosInfoToVector3(creature->Getpos());
        float distSq = Vector3::DistanceSquared(pos, creaturePos);

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            nearestCreature = creature;
        }
    }

    return nearestCreature;
}

std::vector<CreaturePtr> CoreRoom::GetAdjacentCreatures(Vector3 pos, int32 excludeObjectId)
{
    std::vector<CreaturePtr> adjacentCreatures;

    auto [cellX, cellZ] = GetSectorPos(pos);

    std::vector<GameObjectPtr> snapshot;
    {
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

    for (auto& go : snapshot)
    {
        if (!go) continue;
        if (go->GetObjectId() == excludeObjectId) continue;

        if (go->GetType() == GameObjectType::Player || go->GetType() == GameObjectType::Monster)
        {
            auto creature = std::static_pointer_cast<Creature>(go);
            adjacentCreatures.push_back(creature);
        }
    }

    return adjacentCreatures;
}
