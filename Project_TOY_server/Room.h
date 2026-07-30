#pragma once
#include "Types.h"
#include "JobQueue.h"
#include "Protocol/Protocol.pb.h"

class Room : public JobQueue 
{
public:
    Room(int32 roomId, int32 mapId);

    //Room 초기화 함수
    void Init(); 
    void Enter(GameObjectPtr go);//단일 입장 , 가능하면 플레이어에게만 사용
    void EnterMonsters(const std::vector<MonsterPtr>& monsters); //복수의 몬스터들 한번에 입장

    void Leave(GameObjectPtr  go);

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
    void HandleSkill(CreaturePtr SKillUser, GameObjectPtr targetobj, Vector3 targetPos , int32 skillid);

    
    //본인에게 MP 변경 패킷 전송
    void UpdateMPToSelf(PlayerPtr player);
    //본인에게 HP 변경 패킷 전송
    void UpdateHPToSelf(PlayerPtr player);
    //대상(target)의 MP 변화를 broadcastcenter의 주변에 알림
    void UpdateMPToOthers(CreaturePtr target, Vector3 broadcastcenter);
    //대상(target)의 HP 변화를 broadcastcenter의 주변에 알림
    void UpdateHPToOthers(CreaturePtr target, CreaturePtr attacker, int damage, Vector3 broadcastcenter);

    void UpdateProjectile(std::shared_ptr<Projectile> projectile);


    void SetRoomid(int32 roomid) { _Selfroomid = roomid; }
    int32 GetRoomid() {return _Selfroomid;}

    //몬스터 스폰 
    void MonsterSpawn(int32 NumOfMonster, int templatedId);
    void MonsterSpawn(int32 NumOfMonster, int templatedId, bool IsRLControll);
    void MonsterSpawn(int32 NumOfMonster, int templatedId, bool IsRLControll,bool IsHusuabi);
    void HusuabiMonsterSpawn(int32 NumOfMonster, int templatedId);

    MapPtr GetMapptr();

    PlayerPtr GetNearestPlayer(Vector3 pos, float maxRange);

    void Execute() override;

    
public:
    CoreRoomPtr GetCoreRoom() { return _coreroom; }
    void UpdateObjectGrid(GameObjectPtr go, Vector3 oldPos, Vector3 newPos);
private:
    
    int32 _Selfroomid;

    

private:
    CoreRoomPtr _coreroom;
};
