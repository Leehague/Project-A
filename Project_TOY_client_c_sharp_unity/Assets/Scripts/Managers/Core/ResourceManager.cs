using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ResourceManager
{
    // 리소스를 로드합니다. T는 로드할 에셋의 타입(GameObject, AudioClip 등)입니다.
    public T Load<T>(string path) where T : Object
    {
        // 나중에 경로 앞에 공통 폴더명을 붙이거나, 
        // 로드된 에셋을 딕셔너리에 캐싱(Caching)하여 최적화할 수 있습니다.
        return Resources.Load<T>(path);
    }

    // 프리팹을 생성합니다.
    public GameObject Instantiate(string path, Transform parent = null)
    {
        // 1. 프리팹 리소스를 가져옵니다.
        GameObject prefab = Load<GameObject>($"Prefabs/{path}");
        if (prefab == null)
        {
            Debug.LogError($"Failed to load prefab : {path}");
            return null;
        }

        // 2. 생성합니다.
        return Object.Instantiate(prefab, parent);
    }

    // 오브젝트를 파괴합니다.
    public void Destroy(GameObject go)
    {
        if (go == null)
            return;

        Object.Destroy(go);
    }
    public void Destroy(GameObject go, float delay) 
    {
        if (go == null)
            return;


        Object.Destroy(go, delay);
    }
}
