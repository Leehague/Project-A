using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Util
{
    // 1. 자식 오브젝트에서 특정 컴포넌트를 찾아주는 함수
    public static T FindChild<T>(GameObject go, string name = null, bool recursive = false) where T : UnityEngine.Object
    {
        if (go == null) return null;

        if (recursive == false) // 직계 자식만 검색
        {
            for (int i = 0; i < go.transform.childCount; i++)
            {
                Transform transform = go.transform.GetChild(i);
                if (string.IsNullOrEmpty(name) || transform.name == name)
                {
                    T component = transform.GetComponent<T>();
                    if (component != null) return component;
                }
            }
        }
        else // 모든 자식(손자 포함) 검색
        {
            foreach (T component in go.GetComponentsInChildren<T>(true))
            {
                if (string.IsNullOrEmpty(name) || component.name == name)
                    return component;
            }
        }

        return null;
    }

    // 2. 자식 오브젝트(GameObject 자체)를 찾아주는 함수
    public static GameObject FindChild(GameObject go, string name = null, bool recursive = false)
    {
        Transform transform = FindChild<Transform>(go, name, recursive);
        if (transform == null) return null;
        return transform.gameObject;
    }
}

// 3. 편리한 코딩을 위한 확장 메서드 (Extension Methods)
public static class Extension
{
    // 컴포넌트가 있으면 가져오고, 없으면 붙여서 반환 (Null 방지)
    public static T GetOrAddComponent<T>(this GameObject go) where T : UnityEngine.Component
    {
        T component = go.GetComponent<T>();
        if (component == null)
            component = go.AddComponent<T>();
        return component;
    }
    
    // UI 이벤트를 등록하는 도우미 (나중에 UI_EventHandler 연동 시 사용)
    // public static void AddUIEvent(this GameObject go, Action<PointerEventData> action, Define.UIEvent type = Define.UIEvent.Click)
    // {
    //     UI_Base.BindEvent(go, action, type);
    // }
}