using Protocol;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_Inventory : UI_Popup
{
    public Transform gridPanel;
   
    private bool _init = false;
    public override void Init()
    {
        if (_init) return;
        _init = true;

        base.Init(); // 부모(UI_Popup)의 Init()을 호출하여 드래그 타겟 설정 및 화면 중앙 배치를 처리합니다.
        // 계층 구조상 맨 아래로 보내 강제로 화면 최상단(맨 앞)에 렌더링되도록 합니다.
        transform.SetAsLastSibling();
        // 창이 열릴 때 서버에 내 아이템 리스트를 요청합니다.
        CS_OWNED_ITEM_REQUEST req = new CS_OWNED_ITEM_REQUEST();
        req.PlayerId = Managers.objectManager.Myplayer_playerId;
        Managers.networkManager.Send(req);
    }
    private void Start()
    {
        Init();
    }
    public void RefreshUI(Google.Protobuf.Collections.RepeatedField<ItemInfo> items)
    {
        if (gridPanel == null)
        {
            Debug.LogError("UI_Inventory: gridPanel이 할당되지 않았습니다. 인스펙터 창을 확인해주세요.");
            return;
        }
        GridLayoutGroup gridLayout = gridPanel.GetComponent<GridLayoutGroup>();
        if (gridLayout != null)
        {
            gridLayout.childAlignment = TextAnchor.UpperLeft;
            gridLayout.startCorner = GridLayoutGroup.Corner.UpperLeft;
            gridLayout.startAxis = GridLayoutGroup.Axis.Horizontal;
        }
        foreach (Transform child in gridPanel)
        {
            Managers.resourceManager.Destroy(child.gameObject);
        }
        foreach (ItemInfo item in items)
        {
            GameObject go = Managers.resourceManager.Instantiate("UI/Popup/UI_InventoryItem", gridPanel);
            if (go != null)
            {
                UI_InventoryItem itemUI = go.GetComponent<UI_InventoryItem>();
                if (itemUI != null) itemUI.SetInfo(item);
            }
        }
    }
    
}
