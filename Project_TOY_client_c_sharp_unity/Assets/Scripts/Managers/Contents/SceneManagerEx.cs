using UnityEngine;

public class SceneManagerEx
{
    public BaseScene CurrentScene { get { return GameObject.FindObjectOfType<BaseScene>(); } }

    public void LoadScene(Define.SceneType type)
    {
        // 1. 기존 씬의 오브젝트들 정리 (Managers.Object.Clear() 등)
        Managers.objectManager.Clear();

        // 2. 유니티 씬 로드
        string name = System.Enum.GetName(typeof(Define.SceneType), type);
        UnityEngine.SceneManagement.SceneManager.LoadScene(name);
    }
    public AsyncOperation LoadSceneAsync(Define.SceneType type) 
    {
        Managers.objectManager.Clear();


        string name = System.Enum.GetName(typeof(Define.SceneType), type);
        return UnityEngine.SceneManagement.SceneManager.LoadSceneAsync(name);
    }

}