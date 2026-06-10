using Protocol;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class UI_Inventory : UI_Popup, IDragHandler, IPointerDownHandler
{
    public Transform gridPanel; // 유니티 에디터에서 Grid Layout Group이 있는 패널을 연결
    public RectTransform windowPanel; // 드래그로 움직일 실제 창(배경) 패널 연결

    private RectTransform _rectTransform;
    private bool _init = false;

    public override void Init()
    {
        if (_init) return;
        _init = true;
        base.Init(); // 부모의 Init 호출 -> UIManager가 Canvas와 GraphicRaycaster를 자동 부착하고 최상단 정렬함

        // 화면 전체를 덮는 Root 대신, 실제 창(WindowPanel)을 이동/정렬 타겟으로 잡습니다.
        _rectTransform = windowPanel; 
        if (_rectTransform != null)
        {
            _rectTransform.anchoredPosition = Vector2.zero; // 창이 열릴 때 화면 중앙(0, 0)으로 위치 초기화
        }

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

    // 패킷 핸들러에서 호출될 UI 갱신 함수
    public void RefreshUI(Google.Protobuf.Collections.RepeatedField<ItemInfo> items)
    {
        //// RefreshUI가 호출될 때 UI를 강제로 화면 정중앙으로 이동시킵니다.
        //if (_rectTransform == null) _rectTransform = GetComponent<RectTransform>();
        //if (_rectTransform != null)
        //{
        //    // 앵커를 코드로 강제 조작하면 에디터에서 설정한 창 크기(Width/Height)가 망가져 꽉 차게 될 수 있으므로 제거합니다.
        //    // 에디터에서 UI_Inventory의 Anchor를 중앙(Center)으로 맞추고 고정된 크기(예: 800x600)를 직접 지정해주세요.
        //    //_rectTransform.anchoredPosition = Vector2.zero;

        //    // 정보가 갱신될 때도 맨 앞으로 가져옵니다.
        //    transform.SetAsLastSibling();
        //}

        if (gridPanel == null)
        {
            Debug.LogError("UI_Inventory: gridPanel이 할당되지 않았습니다. 인스펙터 창을 확인해주세요.");
            return;
        }

        // Grid Layout Group의 정렬 방향을 왼쪽 위(Upper Left)로 강제 설정합니다.
        GridLayoutGroup gridLayout = gridPanel.GetComponent<GridLayoutGroup>();
        if (gridLayout != null)
        {
            gridLayout.childAlignment = TextAnchor.UpperLeft;
            gridLayout.startCorner = GridLayoutGroup.Corner.UpperLeft; // 왼쪽 위 구석에서부터
            gridLayout.startAxis = GridLayoutGroup.Axis.Horizontal;    // 가로 방향으로 먼저 채우기
        }

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

            if (go != null)
            {
                // 슬롯에 데이터 세팅
                UI_InventoryItem itemUI = go.GetComponent<UI_InventoryItem>();
                if (itemUI != null) itemUI.SetInfo(item);

            }
            
        }
    }

    // 마우스 클릭 시 창을 다른 UI들보다 맨 앞으로 가져옵니다.
    public void OnPointerDown(PointerEventData eventData)
    {
        transform.SetAsLastSibling();
    }

    // 마우스 드래그를 통해 창을 이동시킵니다.
    public void OnDrag(PointerEventData eventData)
    {
        if (_rectTransform != null)
        {
            Canvas canvas = GetComponentInParent<Canvas>();
            float scaleFactor = canvas != null ? canvas.scaleFactor : 1f;
            _rectTransform.anchoredPosition += eventData.delta / scaleFactor;
        }
    }
}
