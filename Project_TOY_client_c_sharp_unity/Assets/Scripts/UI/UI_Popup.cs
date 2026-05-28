using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UI_Popup : UI_Base
{
    public virtual void Init()
    {
        // Popup UI는 다른 UI보다 앞에 와야 하므로 SortOrder를 높게 설정합니다.
        Managers.uiManager.SetCanvas(gameObject, true);
    }

    // 팝업을 닫는 공통 기능
    public virtual void ClosePopupUI()
    {
        Managers.uiManager.ClosePopupUI(this);
    }
}