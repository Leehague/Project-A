using System.Collections.Generic;
using UnityEngine;

public class ObjectManager
{
    // 내 캐릭터는 특별하니까 따로 관리합니다.
    public GameObject MyPlayer { get; set; }

    // 나머지 객체들은 ID로 관리합니다.
    Dictionary<int, GameObject> _objects = new Dictionary<int, GameObject>();

    public void Add(int id, GameObject go, bool isMyPlayer = false)
    {
        if (isMyPlayer)
        {
            MyPlayer = go;
        }

        _objects.Add(id, go);
    }

    public void Remove(int id)
    {
        if (id == 0) return; // 예외 처리

        if (_objects.TryGetValue(id, out GameObject go))
        {
            // 내 플레이어가 삭제되는 경우라면 참조 제거
            if (MyPlayer == go)
                MyPlayer = null;

            Managers.resourceManager.Destroy(go);
            _objects.Remove(id);
        }
    }

    public GameObject Find(int id)
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
}