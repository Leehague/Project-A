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
    public ulong Myplayer_playerId { get; set; }  

    private List<Protocol.SC_PLAYER_SPAWN> _spawnQueue = new List<Protocol.SC_PLAYER_SPAWN>();
    // Game 씬이 준비되었는지 나타내는 플래그
    public bool IsSceneReady { get; set; } = false;


    // 나머지 객체들은 ID로 관리합니다. 여기서 ID는 objectId로 서버에서 관리하는 번호입니다.
    Dictionary<ulong, GameObject> _objects = new Dictionary<ulong, GameObject>();

    public void Add(ulong objectid, GameObject go, bool isMyPlayer = false)
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

    public void Remove(ulong objectid)
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

    public GameObject Find(ulong id)
    {
        _objects.TryGetValue(id, out GameObject go);
        return go;
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
    public GameObject SpawnPlayer(PosInfo postion , bool isMyPlayer = false)
    {
        ulong templateId = postion.TempleteId;
        ulong objectId = postion.ObjectId;

        // 데이터 매니저에서 정보 추출 ( Knight, Mage 등 )
        if (Managers.dataManager.StatDict.TryGetValue(templateId, out Stat statInfo) == false)
        {
            Debug.Log("Spawn Fail : fail to take from stat info ");
            return null; 
        }

        Debug.Log($"Attempting to spawn: {statInfo.modelPath}");

        // 리소스 매니저로 프리팹 생성
        GameObject go = Managers.resourceManager.Instantiate(statInfo.modelPath);
        go.name = statInfo.name;

        Vector3 spawnPos = new Vector3(postion.X, postion.Y, postion.Z);
        go.transform.position = spawnPos;
        

        // 여기서 Add를 호출하여 관리 목록에 추가
        Add(objectId, go, isMyPlayer);

        //컨트롤러 초기화
        PlayerController pc = go.GetOrAddComponent<PlayerController>();
        pc.stat = statInfo;
        pc.IsMyPlayer = isMyPlayer;
        // 핵심: 생성 직후에 컨트롤러의 내부 목적지 좌표도 동기화
        pc.SyncPos(spawnPos);

        

        return go;
    }


    public void HandleSpawn(Protocol.SC_PLAYER_SPAWN packet)
    {
        //// 아직 게임 씬이 아니라면 큐에 보관만 하고 리턴
        //if (Managers.sceneManagerEx.CurrentScene.SceneType !=SceneType.Game)
        //{
        //    _spawnQueue.Add(packet);
        //    Debug.Log("Scene not ready. Spawn packet queued."); 
        //    return;
        //}

        // 씬이 준비되었다면 즉시 생성
        SpawnFromPacket(packet);
    }

 

    private void SpawnFromPacket(Protocol.SC_PLAYER_SPAWN packet)
    {
        foreach (var pos in packet.PlayersPosInfo)
        {
            bool isMyPlayer = (pos.ObjectId == Myplayer_playerId);
            if (isMyPlayer) { continue; }

            Debug.Log(pos.ObjectId);
            SpawnPlayer(pos);
        }
    }
}