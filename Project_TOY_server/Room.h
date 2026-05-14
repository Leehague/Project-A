#pragma once
#include "Types.h"
#include "JobQueue.h"


class Room : public JobQueue 
{
public:
    Room(int32 roomId, int32 mapId);
    void Enter(GameObjectPtr go);//단일 입장 , 가능하면 플레이어에게만 사용
    void EnterMonsters(const std::vector<MonsterPtr>& monsters); //복수의 몬스터들 한번에 입장

    void Leave(PlayerPtr player);

    //방안의 모든 플레이어(세션들)에게 방송
    void Broadcast(SendBufferPtr sendBuffer);
    //passing_object_id 를 가지는 플레이어(유저)만 제외하고 브로드 캐스팅
    void Broadcast(SendBufferPtr sendBuffer, int32 passing_object_id);
    //targets에게만 브로드캐스팅
    void Broadcast(SendBufferPtr sendBuffer, std::vector<std::shared_ptr<Session>> targets);
    // 인접 플레이어 에게 방송
    void BroadcastAround(SendBufferPtr sendBuffer, Vector3 centerPos, int32 passing_object_id = -1);

    void SpawnBroadcast(PlayerPtr player);
    void SpawnBroadcast(const std::vector<MonsterPtr>& monsters);
    void SpawnBroadcast(MonsterPtr monster);
    
    void BroadcastMove(GameObjectPtr go);
    void BroadcastMove(const std::vector<GameObjectPtr>& gameobjects);
    


    void SendTo(PlayerPtr player, SendBufferPtr sendBuffer);
    void SendMoveResync(PlayerPtr player);


    // 이동 패킷 처리 루틴
    void HandleMove(PlayerPtr player ,Protocol::CS_MOVING& pkt);


    // 스킬 패킷 처리 
    void HandleSkillForPlayer(PlayerPtr player , Protocol::CS_SKILL& pkt);
    void HandleSkillForMonster(MonsterPtr monster, GameObjectPtr targetobj, Vector3 targetPos, int32 skillid);
    void HandleSkill(GameObjectPtr SKillUser, GameObjectPtr targetobj, Vector3 targetPos , int32 skillid);

    
    //본인에게 MP 변경 패킷 전송
    void UpdateMPToSelf(PlayerPtr player);
    //본인에게 HP 변경 패킷 전송
    void UpdateHPToSelf(PlayerPtr player);
    //대상(target)의 MP 변화를 broadcastcenter의 주변에 알림
    void UpdateMPToOthers(GameObjectPtr target, Vector3 broadcastcenter);
    //대상(target)의 HP 변화를 broadcastcenter의 주변에 알림
    void UpdateHPToOthers(GameObjectPtr target, GameObjectPtr attacker, int damage, Vector3 broadcastcenter);

    //투사체 관련 함수
    void SpawnProjectile(GameObjectPtr attacker, const SkillData *skillData, Vector3 targetPos);
    void UpdateProjectile(std::shared_ptr<Projectile> projectile);


    void SetRoomid(int32 roomid) { _Selfroomid = roomid; }
    int32 GetRoomid() {return _Selfroomid;}

    //몬스터 스폰 
    void MonsterSpawn(int32 NumOfMonster, int templatedId);

    MapPtr GetMapptr() { return _map; };

    PlayerPtr GetNearestPlayer(Vector3 pos, float maxRange);

    void Execute() override;

private:
    std::mutex _lock;
    std::map<uint64, GameObjectPtr> _objects;

    int32 _Selfroomid;

    MapPtr _map;

private: //그리드 시스템 관련
    // 1. 그리드 인덱스 계산 (좌표 -> 그리드 좌표)
    
    std::pair<int, int> GetSectorPos(Vector3 pos);
    // 2. 내 주변 9개 칸에 속한 플레이어 리스트 가져오기 (브로드캐스트 타겟 추출)
    std::vector<std::shared_ptr<Session>> GetAdjacentPlayersSessions(Vector3 pos, int32 passing_object_id =-1);

    std::vector<PlayerPtr> GetAdjacentPlayers(Vector3 pos, int32 passing_object_id = -1);

public:
    // 3. 오브젝트 이동 시 그리드 갱신 (Enter/Move/Leave 시 호출)
    void UpdateObjectGrid(GameObjectPtr go, Vector3 oldPos, Vector3 newPos);

    
private:
    // [그리드 데이터 구조] _objectGrid[z][x] = {해당 칸에 있는 오브젝트 세트}
    // std::set을 쓰면 중복 제거 및 특정 오브젝트 탐색이 빠릅니다.
    std::vector<std::vector<std::set<GameObjectPtr>>> _sectors;

    int32 _sectorSize = 50;

    int32 _sectorCountX;
    int32 _sectorCountZ;


    // Map의 정보를 복사해두거나 직접 참조하여 인덱스 계산에 사용
    float _minX=0, _minZ=0, _cellSize=0;
    int _gridWidth=0, _gridHeight=0;
private:
    void InitGridData(const MapData* mapdata);


private:
    //방송관련 함수들
     
};

