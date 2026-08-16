using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

public class UI_Popup : UI_Base, IDragHandler, IPointerDownHandler
{
    [Header("Drag Window Settings")]
    [SerializeField] protected RectTransform windowPanel; // 드래그로 움직일 실제 창(배경) 패널
    [SerializeField] protected bool _isDraggable = true;  // 드래그 기능 사용 여부
    protected RectTransform _rectTransform;
    private Canvas _canvas;
    public virtual void Init()
    {
        // Popup UI는 다른 UI보다 앞에 와야 하므로 SortOrder를 높게 설정합니다.
        Managers.uiManager.SetCanvas(gameObject, true);
        // 드래그 타겟 설정 (windowPanel이 지정되어 있다면 그것을 쓰고, 없으면 자기 자신을 대상으로 설정)
        if (windowPanel != null)
        {
            _rectTransform = windowPanel;
        }
        else
        {
            _rectTransform = GetComponent<RectTransform>();
        }
        if (_rectTransform != null)
        {
            _rectTransform.anchoredPosition = Vector2.zero; // 창이 열릴 때 화면 중앙으로 위치 초기화
        }
        // 캐싱 처리
        _canvas = GetComponentInParent<Canvas>();
    }
    // 팝업을 닫는 공통 기능
    public virtual void ClosePopupUI()
    {
        Managers.uiManager.ClosePopupUI(this);
    }
    // 마우스 클릭 시 창을 다른 UI들보다 맨 앞으로 가져옵니다.
    public virtual void OnPointerDown(PointerEventData eventData)
    {
        transform.SetAsLastSibling();
    }
    // 마우스 드래그를 통해 창을 이동시킵니다.
    public virtual void OnDrag(PointerEventData eventData)
    {
        if (!_isDraggable || _rectTransform == null) return;
        float scaleFactor = _canvas != null ? _canvas.scaleFactor : 1f;
        _rectTransform.anchoredPosition += eventData.delta / scaleFactor;
    }
}
