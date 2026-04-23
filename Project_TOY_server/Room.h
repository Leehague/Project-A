#pragma once
#include "Types.h"



class Room : public std::enable_shared_from_this<Room>
{
public:
    Room(int32 roomId, int32 mapId);
    void Enter(GameObjectPtr go);//단일 입장 , 가능하면 플레이어에게만 사용
    void EnterMonsters(const std::vector<MonsterPtr>& monsters); //복수의 몬스터들 한번에 입장

    void Leave(PlayerPtr player);

    void Broadcast(SendBufferPtr sendBuffer);
    void Broadcast(SendBufferPtr sendBuffer, int32 passing_object_id);//passing_object_id 를 가지는 플레이어(유저)만 제외하고 브로드 캐스팅
    void SpawnBroadcast(PlayerPtr player);
    void SpawnBroadcast(const std::vector<MonsterPtr>& monsters);
    void SpawnBroadcast(MonsterPtr monster);

    void SendTo(PlayerPtr player, SendBufferPtr sendBuffer);
    void SendMoveResync(PlayerPtr player);


    // 이동 패킷 처리 루틴
    void HandleMove(PlayerPtr player ,Protocol::CS_MOVING& pkt);

    // 스킬 패킷 처리 
    void HandleSkill(PlayerPtr player , Protocol::CS_SKILL& pkt);

    void SetRoomid(int32 roomid) { _Selfroomid = roomid; }
    int32 GetRoomid() {return _Selfroomid;}

    //몬스터 스폰 
    void MonsterSpawn(int32 NumOfMonster);
private:
    std::mutex _lock;
    std::map<uint64, GameObjectPtr> _objects;

    int32 _Selfroomid;

    MapPtr _map;
};

