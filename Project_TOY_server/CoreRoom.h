#include "Types.h"
#include "InfoSturct.h"
#include <set>

class CoreRoom
{
public:

    CoreRoom(int32 mapId, bool& maploadsuccess);

    // 이동 패킷 처리 루틴
    bool HandleMove(PlayerPtr player, Core::PosInfo posinfo);
    void HandleSkill(CreaturePtr SKillUser, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid);

    //투사체 관련 함수
    void SpawnProjectile(CreaturePtr attacker, const SkillData* skillData, Vector3 targetPos);

    std::vector<Core::DamageResult> UpdateProjectile(std::shared_ptr<Projectile> projectile , bool& ishit);

    MapPtr GetMapptr() { return _map; };

    PlayerPtr GetNearestPlayer(Vector3 pos, float maxRange);

public:
    
    std::map<uint64, GameObjectPtr> _objects;
    MapPtr _map;

public:
    //그리드 시스템 관련
    // 1. 그리드 인덱스 계산 (좌표 -> 그리드 좌표)

    std::pair<int, int> GetSectorPos(Vector3 pos);
    // 2. 내 주변 9개 칸에 속한 플레이어 리스트 가져오기 (브로드캐스트 타겟 추출)
    std::vector<std::shared_ptr<Session>> GetAdjacentPlayersSessions(Vector3 pos, int32 passing_object_id = -1);

    std::vector<PlayerPtr> GetAdjacentPlayers(Vector3 pos, int32 passing_object_id = -1);

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


};
