using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UI_Scene : UI_Base
{
    // Scene UI는 화면을 가득 채우는 경우가 많으므로 초기화 시 캔버스 설정을 해줍니다.
    public virtual void Init()
    {
        // UIManager가 생성 시점에 호출해줄 예정입니다.
        Managers.uiManager.SetCanvas(gameObject, false);
    }
}