using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TestObjectScript : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        // "Assets/Resources/Prefabs/TestObject.prefab"을 찾아 생성합니다.
        GameObject go = Managers.resourceManager.Instantiate("TestObject");

        // 5초 뒤에 삭제
        Managers.resourceManager.Destroy(go, 5.0f); // (Destroy에 시간 매개변수 추가 필요 시 오버로딩)
    }

    // Update is called once per frame
    void Update()
    {
        
    }
}
