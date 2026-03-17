using UnityEngine;
using UnityEngine.EventSystems;


public abstract class BaseScene : MonoBehaviour
{
    // 씬의 타입을 구분하기 위함 (Login, Game 등)
    public SceneType SceneType { get; protected set; } = SceneType.Unknown;

    void Awake()
    {
        Init();
    }

    protected virtual void Init()
    {
        // 1. 모든 씬의 필수 요소인 Managers 초기화 확인
        // Managers.Instance가 호출될 때 없으면 자동 생성되도록 설계했을 것입니다.
        var managers = Managers.Instance;

        // 2. EventSystem이 없으면 모든 씬에서 UI 클릭이 안 되므로 자동 생성
        Object obj = GameObject.FindObjectOfType<EventSystem>();
        if (obj == null)
        {
            GameObject eventSystem = new GameObject { name = "@EventSystem" };
            eventSystem.AddComponent<EventSystem>();
            eventSystem.AddComponent<StandaloneInputModule>();
        }
    }

    // 각 씬에서 구체적으로 할 일은 자식에서 구현
    public abstract void Clear();
}