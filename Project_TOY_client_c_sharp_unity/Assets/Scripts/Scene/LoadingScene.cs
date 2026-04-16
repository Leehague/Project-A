using System.Collections;
using UnityEngine;
using UnityEngine.SceneManagement;

public class LoadingScene : BaseScene
{
    private AsyncOperation _asyncOp;



    protected override void Init()
    {
        base.Init(); // 부모의 공통 초기화 수행
        SceneType = Define.SceneType.Loading;

         StartCoroutine(LoadSceneAsync());
    }

    public override void Clear()
    {
        // 씬 이동 시 오브젝트 풀 등을 정리
    }

    IEnumerator LoadSceneAsync()
    {
        
        _asyncOp = Managers.sceneManagerEx.LoadSceneAsync(Define.SceneType.Game);
        
        _asyncOp.allowSceneActivation = false;

        // 2. progress가 0.9에 도달할 때까지 루프
        // 0.9f 미만일 때만 도는 게 아니라, 도달하면 break 하도록 수정
        while (true)
        {
            Debug.Log($"[Debug] Current Progress: {_asyncOp.progress}");

            if (_asyncOp.progress >= 0.9f)
                break;

            yield return null;
        }
        _asyncOp.allowSceneActivation = true;
        
    }


}

