using Protocol;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UI_Inventory : UI_Popup
{
    public Transform gridPanel; // 유니티 에디터에서 Grid Layout Group이 있는 패널을 연결

    private void Start()
    {
        
        // 창이 열릴 때 서버에 내 아이템 리스트를 요청합니다.
        CS_OWNED_ITEM_REQUEST req = new CS_OWNED_ITEM_REQUEST();
        req.PlayerId = Managers.objectManager.Myplayer_playerId;
        Managers.networkManager.Send(req);
    }

    // 패킷 핸들러에서 호출될 UI 갱신 함수
    public void RefreshUI(Google.Protobuf.Collections.RepeatedField<ItemInfo> items)
    {
        // 1. 기존에 그려진 아이템 슬롯들을 모두 삭제 (초기화)
        foreach (Transform child in gridPanel)
        {
            Managers.resourceManager.Destroy(child.gameObject);
        }

        // 2. 서버에서 받은 아이템 정보로 슬롯 새로 생성
        foreach (ItemInfo item in items)
        {
            // UI_InventoryItem 프리팹을 gridPanel의 자식으로 생성
            GameObject go = Managers.resourceManager.Instantiate("UI/Popup/UI_InventoryItem", gridPanel);

            // 슬롯에 데이터 세팅
            UI_InventoryItem itemUI = go.GetComponent<UI_InventoryItem>();
            if (itemUI != null) itemUI.SetInfo(item);
        }
    }
}
