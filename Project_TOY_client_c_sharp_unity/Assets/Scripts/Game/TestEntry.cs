using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TestEntry : MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        // 이 한 줄이 실행되는 순간 @Managers 오브젝트가 자동 생성되고 접속을 시도합니다.
        var mgr = Managers.Instance;
        

    }

    // Update is called once per frame
    void Update()
    {
        
    }
}
