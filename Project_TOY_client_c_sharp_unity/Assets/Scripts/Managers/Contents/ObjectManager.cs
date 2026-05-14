using Protocol;
using System;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;

public class ObjectManager
{
    // 내 캐릭터는 특별하니까 따로 관리합니다.
    public GameObject MyPlayer { get; set; }
    public PosInfo MyplayerPosInfo { get; set; }
    //서버에서 전송해준 내 캐릭터의 objectId
    public int Myplayer_playerId { get; set; }  

    private List<Protocol.SC_PLAYER_SPAWN> _spawnQueue = new List<Protocol.SC_PLAYER_SPAWN>();
    // Game 씬이 준비되었는지 나타내는 플래그
    public bool IsSceneReady { get; set; } = false;


    // 나머지 객체들은 ID로 관리합니다. 여기서 ID는 objectId로 서버에서 관리하는 번호입니다.
    Dictionary<int, GameObject> _objects = new Dictionary<int, GameObject>();

    public void Add(int objectid, GameObject go, bool isMyPlayer = false)
    {
        if (isMyPlayer)
        {
            MyPlayer = go;
        }
        //이미 있는 오브젝트 아이디 라면 추가하지 않고 파괴 대신 경고 메시지 출력
        if (_objects.ContainsKey(objectid))
        {
            Debug.LogWarning($"Object with ID {objectid} already exists. Skipping spawn.");
            Managers.resourceManager.Destroy(go);
            return;
        }
            _objects.Add(objectid, go);
    }

    public void Remove(int objectid)
    {
        if (objectid == 0) return; // 예외 처리

        if (_objects.TryGetValue(objectid, out GameObject go))
        {
            // 내 플레이어가 삭제되는 경우라면 참조 제거
            if (MyPlayer == go)
                MyPlayer = null;

            Managers.resourceManager.Destroy(go);
            _objects.Remove(objectid);
        }
    }

    public GameObject Find(int id)
    {
        _objects.TryGetValue(id, out GameObject go);
        return go;
    }

    public CreatureController FindController(int id) 
    {
        return Find(id).GetComponent<CreatureController>();
    }

    // 모든 객체 삭제 (씬 전환 등에서 사용)
    public void Clear()
    {
        foreach (GameObject go in _objects.Values)
            Managers.resourceManager.Destroy(go);

        _objects.Clear();
        MyPlayer = null;
    }

    // 생성 및 등록을 한 번에 처리하는 함수
    public GameObject SpawnPlayer(PosInfo postion, int templeteId , bool isMyPlayer = false)
    {
        int _templeteId = templeteId;
        int objectId = postion.ObjectId;

        // 데이터 매니저에서 정보 추출 ( Knight, Mage 등 )
        if (Managers.dataManager.StatDict.TryGetValue(_templeteId, out Stat statInfo) == false)
        {
            Debug.Log("Spawn Fail : fail to take from stat info ");
            return null; 
        }
        // 리소스 매니저로 프리팹 생성
        GameObject go = Managers.resourceManager.Instantiate(statInfo.modelPath);
        go.name = statInfo.name;

        Vector3 spawnPos = new Vector3(postion.X, postion.Y, postion.Z);
        go.transform.position = spawnPos;
        

        // 여기서 Add를 호출하여 관리 목록에 추가
        Add(objectId, go, isMyPlayer);

        //컨트롤러 초기화
        PlayerController pc = go.GetOrAddComponent<PlayerController>();
        pc.stat = new Stat(statInfo);

        pc.stat.hp = pc.stat.MaxHp; //스폰시 풀피로 

        pc.IsMyPlayer = isMyPlayer;
        // 핵심: 생성 직후에 컨트롤러의 내부 목적지 좌표도 동기화
        pc.SyncPos(spawnPos);

        return go;
    }

    public GameObject SpawnMonster(PosInfo postion, int templeteId)
    {
        int _templeteId = templeteId;
        int objectId = postion.ObjectId;

        // 데이터 매니저에서 정보 추출 ( Knight, Mage 등 )
        if (Managers.dataManager.StatDict.TryGetValue(_templeteId, out Stat statInfo) == false)
        {
            Debug.Log("Spawn Fail : fail to take from stat info ");
            return null;
        }

        GameObject go;
        if (Managers.poolingManager.TryPopcreatureObject(_templeteId, out CreatureController newcc))
        {
            //풀링 되어 있는게 있다면 그것을 택함
            go = newcc.gameObject;
            Debug.Log("여기?");
        }
        else
        {
            // 아니면 리소스 매니저로 프리팹 생성
            go = Managers.resourceManager.Instantiate(statInfo.modelPath);
        }
        
        go.name = statInfo.name;

        Debug.Log($"spawnPos : ({postion.X} , {postion.Y}, {postion.Z})");

        Vector3 spawnPos = new Vector3(postion.X, postion.Y, postion.Z);
        go.transform.position = spawnPos;


        // 여기서 Add를 호출하여 관리 목록에 추가
        Add(objectId, go); //'몬스터' 스폰이기때문에 IsMyplayer는 기본인 false 사용

        //컨트롤러 초기화
        MonsterController mc = go.GetOrAddComponent<MonsterController>();
        mc.stat = new Stat(statInfo);

        mc.stat.hp = mc.stat.MaxHp; //스폰시 풀피로 

        // 핵심: 생성 직후에 컨트롤러의 내부 목적지 좌표도 동기화
        mc.SyncPos(spawnPos);

        return go;
    }

    public void HandleSpawn(Protocol.SC_PLAYER_SPAWN packet)
    {
        //pooling 시스템 도입을 고민해야 할 수도?
        SpawnFromPacket(packet);
    }

 

    private void SpawnFromPacket(Protocol.SC_PLAYER_SPAWN packet)
    {
        foreach (var spawninfo in packet.PlayersSpawnInfo)
        {
            bool isMyPlayer = (spawninfo.Spawnposinfo.ObjectId == Myplayer_playerId);
            if (isMyPlayer) { continue; }

            Debug.Log(spawninfo.Spawnposinfo.ObjectId);
            SpawnPlayer(spawninfo.Spawnposinfo, spawninfo.TempleteId);
        }
    }
}