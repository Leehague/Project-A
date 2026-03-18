using Protocol;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;

public class ObjectManager
{
    // 내 캐릭터는 특별하니까 따로 관리합니다.
    public GameObject MyPlayer { get; set; }

    public PosInfo MyplayerPosInfo { get; set; }
    public ulong Myplayer_playerId { get; set; } //서버에서 전송해준 내 캐릭터의 objectId 

    

    // 나머지 객체들은 ID로 관리합니다. 여기서 ID는 objectId로 서버에서 관리하는 번호입니다.
    Dictionary<ulong, GameObject> _objects = new Dictionary<ulong, GameObject>();

    public void Add(ulong objectid, GameObject go, bool isMyPlayer = false)
    {
        if (isMyPlayer)
        {
            MyPlayer = go;
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
    public GameObject SpawnPlayer(PosInfo postion ,bool isMyPlayer = false)
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
        
    
        go.transform.position = new Vector3(postion.X, postion.Y, postion.Z);
        


        // 여기서 Add를 호출하여 관리 목록에 추가
        Add(objectId, go, isMyPlayer);

        //컨트롤러 초기화
        PlayerController pc = go.GetOrAddComponent<PlayerController>();
        pc.stat = statInfo;
        pc.IsMyPlayer = isMyPlayer;
        

        return go;
    }

}