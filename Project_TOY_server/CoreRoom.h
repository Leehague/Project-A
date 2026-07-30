#pragma once
#include "Types.h"
#include "InfoSturct.h"
#include <set>
#include <functional>
#include <map>
#include <vector>
#include "Vector3.h"
#include "GameObject.h"

class CoreRoom : public std::enable_shared_from_this<CoreRoom>
{
public:

    CoreRoom(int32 mapId, bool& maploadsuccess);

    // 이동 패킷 처리 루틴
    bool HandleMove(PlayerPtr player, const Core::PosInfo& posinfo);

  
    // 결과를 받을 필요가 없다면 기본값인 nullptr가 들어가 메모리 할당을 원천 차단합니다.
    bool HandleSkill(CreaturePtr SKillUser, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid , std::vector<Core::DamageResult>* results = nullptr, bool* ishit = nullptr, std::vector<GameObjectPtr>* spawnedObjects = nullptr);

    //투사체 관련 함수

    //투사체 소환
    ProjectilePtr SpawnProjectile(CreaturePtr attacker, const SkillData* skillData, Vector3 targetPos);

    std::vector<Core::DamageResult> UpdateProjectile(std::shared_ptr<Projectile> projectile , bool& ishit);

    MapPtr GetMapptr() { return _map; };

    PlayerPtr GetNearestPlayer(Vector3 pos, float maxRange);
    CreaturePtr GetNearestCreature(Vector3 pos, float maxRange, int32 excludeObjectId);

public:

    //Key: objcetId, value : GameObejctPtr
    std::map<uint64, GameObjectPtr> _objects;
    MapPtr _map;

public:
    //그리드 시스템 관련
    // 1. 그리드 인덱스 계산 (좌표 -> 그리드 좌표)

    std::pair<int, int> GetSectorPos(Vector3 pos);

    std::vector<PlayerPtr> GetAdjacentPlayers(Vector3 pos, int32 passing_object_id = -1);
    std::vector<CreaturePtr> GetAdjacentCreatures(Vector3 pos, int32 excludeObjectId);

public:
    // 3. 오브젝트 이동 시 그리드 갱신 (Enter/Move/Leave 시 호출)
    void UpdateObjectGrid(GameObjectPtr go, Vector3 oldPos, Vector3 newPos);


public:
    // [그리드 데이터 구조] _objectGrid[z][x] = {해당 칸에 있는 오브젝트 세트}
    // std::set을 쓰면 중복 제거 및 특정 오브젝트 탐색이 빠릅니다.
    std::vector<std::vector<std::set<GameObjectPtr>>> _sectors;

    int32 _sectorSize = 50;

    int32 _sectorCountX;
    int32 _sectorCountZ;


    // Map의 정보를 복사해두거나 직접 참조하여 인덱스 계산에 사용
    float _minX = 0, _minZ = 0, _cellSize = 0;
    int _gridWidth = 0, _gridHeight = 0;
public:
    void InitGridData(const MapData* mapdata);

public:
    // 이동 이벤트를 외부(Room)로 전달할 콜백 함수 포인터
    std::function<void(GameObjectPtr)> _onObjectMovedCallback = nullptr;

    // 오브젝트 생성요청을 외부(Room)로 전달할 콜백 함수 포인터
    std::function<GameObjectPtr(GameObjectType, int32, CoreRoomPtr)> _objectFactoryCallback = nullptr;

    // Monster나 Player가 이동했을 때 호출할 함수
    void OnObjectMoved(GameObjectPtr go) {
        if (_onObjectMovedCallback) {
            _onObjectMovedCallback(go); // 등록된 콜백이 있으면 실행!
        }
    }

    
};
