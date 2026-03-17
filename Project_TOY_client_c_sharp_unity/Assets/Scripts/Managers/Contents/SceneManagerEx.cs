using UnityEngine;

public class SceneManagerEx
{
    public BaseScene CurrentScene { get { return GameObject.FindObjectOfType<BaseScene>(); } }

    public void LoadScene(SceneType type)
    {
        // 1. 기존 씬의 오브젝트들 정리 (Managers.Object.Clear() 등)
        Managers.objectManager.Clear();

        // 2. 유니티 씬 로드
        string name = System.Enum.GetName(typeof(SceneType), type);
        UnityEngine.SceneManagement.SceneManager.LoadScene(name);
    }
}