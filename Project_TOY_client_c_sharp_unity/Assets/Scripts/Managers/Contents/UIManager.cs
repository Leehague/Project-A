using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UIManager
{
    // UI 레이어 순서를 관리 (Popup이 뜰 때마다 1씩 증가)
    int _order = 10;

    // 현재 띄워져 있는 팝업들을 관리하는 스택
    Stack<UI_Popup> _popupStack = new Stack<UI_Popup>();

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

        GameObject go = Managers.resourceManager.Instantiate($"UI/Popup/{name}");
        T popup = Extension.GetOrAddComponent<T>(go);
        _popupStack.Push(popup);

        go.transform.SetParent(Root.transform);

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
        Managers.resourceManager.Destroy(popup.gameObject);
        _order--;
    }

    public void CloseAllPopupUI()
    {
        while (_popupStack.Count > 0)
            ClosePopupUI();
    }

    // UIManager.cs 내부에 추가할 메소드
    public T FindUI<T>() where T : UI_Base
    {
        // 만약 UIManager 내부에서 생성된 UI들을 Dictionary나 Stack/List 등으로 관리하고 있다면, 
        // Object.FindObjectOfType 대신 해당 컬렉션에서 검색하여 반환하도록 최적화하는 것이 가장 좋습니다.
        // (아래는 임시로 FindObjectOfType을 래핑해 둔 예시입니다)

        return UnityEngine.Object.FindObjectOfType<T>();
    }

}
