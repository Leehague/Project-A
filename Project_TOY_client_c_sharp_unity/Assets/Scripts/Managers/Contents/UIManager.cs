using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UIManager
{
    // UI 레이어 순서를 관리 (Popup이 뜰 때마다 1씩 증가)
    int _order = 10;

    // 현재 띄워져 있는 팝업들을 관리하는 스택
    Stack<UI_Popup> _popupStack = new Stack<UI_Popup>();

    // 팝업 캐싱을 위한 딕셔너리 (비활성화 상태로 보관)
    Dictionary<string, UI_Popup> _popups = new Dictionary<string, UI_Popup>();

    // 현재 활성화된 Scene UI
    UI_Scene _sceneUI = null;

    // UI가 붙을 루트 오브젝트 (Hierarchy 정리용)
    public GameObject Root
    {
        get
        {
            GameObject root = GameObject.Find("@UI_Root");
            if (root == null)
                root = new GameObject { name = "@UI_Root" };
            return root;
        }
    }

    // 1. 캔버스 설정 (Sort Order 자동 지정)
    public void SetCanvas(GameObject go, bool sort = true)
    {
        Canvas canvas = Extension.GetOrAddComponent<Canvas>(go);
        canvas.renderMode = RenderMode.ScreenSpaceOverlay;
        canvas.overrideSorting = true;

        if (sort)
        {
            canvas.sortingOrder = _order;
            _order++;
        }
        else
        {
            canvas.sortingOrder = 0;
        }
    }

    // 2. Scene UI 띄우기 (화면 하단바, HP바 등)
    public T ShowSceneUI<T>(string name = null) where T : UI_Scene
    {
        if (string.IsNullOrEmpty(name))
            name = typeof(T).Name;

        GameObject go = Managers.resourceManager.Instantiate($"UI/Scene/{name}");
        T sceneUI = Extension.GetOrAddComponent<T>(go);
        _sceneUI = sceneUI;

        go.transform.SetParent(Root.transform);

        return sceneUI;
    }

    // 3. Popup UI 띄우기 (인벤토리, 설정 등)
    public T ShowPopupUI<T>(string name = null) where T : UI_Popup
    {
        if (string.IsNullOrEmpty(name))
            name = typeof(T).Name;

        T popup = null;
        if (_popups.TryGetValue(name, out UI_Popup cachedPopup) && cachedPopup != null)
        {
            popup = cachedPopup as T;
            popup.gameObject.SetActive(true); // 이미 생성된 팝업이면 활성화만 수행
        }
        else
        {
            GameObject go = Managers.resourceManager.Instantiate($"UI/Popup/{name}");
            popup = Extension.GetOrAddComponent<T>(go);
            _popups[name] = popup; // 새로 생성한 경우 캐시에 저장
            go.transform.SetParent(Root.transform);
        }

        _popupStack.Push(popup);

        return popup;
    }

    // 4. 팝업 닫기
    public void ClosePopupUI(UI_Popup popup)
    {
        if (_popupStack.Count == 0) return;

        if (_popupStack.Peek() != popup)
        {
            Debug.Log("Close Popup Failed!");
            return;
        }

        ClosePopupUI();
    }

    public void ClosePopupUI()
    {
        if (_popupStack.Count == 0) return;

        UI_Popup popup = _popupStack.Pop();
        popup.gameObject.SetActive(false); // Destroy 대신 비활성화 (캐시 유지)
        _order--;
    }

    public void CloseAllPopupUI()
    {
        while (_popupStack.Count > 0)
            ClosePopupUI();
    }


    public T FindUI<T>() where T : UI_Base
    {
        // 1. 씬 UI 캐시 확인
        if (_sceneUI != null && _sceneUI is T)
            return _sceneUI as T;

        // 2. 팝업 UI 캐시 확인 (비활성화 상태인 팝업도 O(1)로 즉시 찾음)
        if (_popups.TryGetValue(typeof(T).Name, out UI_Popup popup) && popup != null)
            return popup as T;

        // 3. 폴백: 씬 전체 검색 (비활성화 된 오브젝트 포함하도록 true 전달)
        return UnityEngine.Object.FindObjectOfType<T>(true);
    }

}
