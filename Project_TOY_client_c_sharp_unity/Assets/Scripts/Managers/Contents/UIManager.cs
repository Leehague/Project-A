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
}