#include "CoreRoom.h"
#include "Vector3.h"
#include "Projectile.h"
#include "Player.h"
#include <windows.h> 
#include "MapManager.h"
#include "Map.h"

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

bool CoreRoom::HandleMove(PlayerPtr player, Core::PosInfo posinfo)
{
    if (player == nullptr)
    {
        std::cout << "HandleMove : player ptr is null" << std::endl;
        return false;
    }

    Vector3 currentPos = Vector3::PosInfoToVector3(player->Getpos());
    Vector3 newPos = Vector3(posinfo.x, posinfo.y, posinfo.z);

    // 처음에만 0일 수 있으므로 예외 처리

    uint64 currentTick =::GetTickCount64(); // 현재 서버 시간 (Windows 기준)

    // 처음에만 0일 수 있으므로 예외 처리
    if (player->GetlastMoveTick() == 0) { player->SetlastMoveTick(currentTick - 100); }

    // deltaTime 계산 (초 단위로 변환)
    float deltaTime = (currentTick - player->GetlastMoveTick()) / 1000.0f;
    player->SetlastMoveTick(currentTick); // 현재 시간을 다음 검증을 위해 저장


    float dist = Vector3::Distance(currentPos, newPos);
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
        if (map->CanGo(newPos) == false)
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
void CoreRoom::HandleSkill(CreaturePtr SKillUser, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid)
{
    //TODO
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
    
    projectile->SetState(CreatureState::OnDead);
     
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

//인접 플레이어 추출 (Interest Management)
std::vector<std::shared_ptr<Session>> CoreRoom::GetAdjacentPlayersSessions(Vector3 pos, int32 passing_object_id)
{
    std::vector<std::shared_ptr<Session>> SessionsOfadjacentPlayers;

    auto [cellX, cellZ] = GetSectorPos(pos);

    // Snapshot relevant GameObjectPtr under lock to avoid concurrent modification
    std::vector<GameObjectPtr> snapshot;
    {
        //std::lock_guard<std::mutex> lock(_lock);
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

std::vector <PlayerPtr> CoreRoom::GetAdjacentPlayers(Vector3 pos, int32 passing_object_id)
{
    std::vector <PlayerPtr>adjacentPlayers;

    auto [cellX, cellZ] = GetSectorPos(pos);

    // Snapshot relevant GameObjectPtr under lock to avoid concurrent modification
    std::vector<GameObjectPtr> snapshot;
    {
        //std::lock_guard<std::mutex> lock(_lock);
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


