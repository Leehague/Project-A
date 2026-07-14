using System.Collections;
using UnityEngine;
using UnityEngine.SceneManagement;

public class LoadingScene : BaseScene
{
    private AsyncOperation _asyncOp;
    private int _targetMapId = -1;


    protected override void Init()
    {
        base.Init(); // 부모의 공통 초기화 수행
        SceneType = Define.SceneType.Loading;


        Managers.sceneManagerEx.OnEnterGameReceived += OnReceiveEnterGame;
    }

    private void Start()
    {
        //CS_ENTER_GAME 전송
        Protocol.CS_ENTER_GAME _CS_enter_game_pkt = new Protocol.CS_ENTER_GAME();
        Managers.networkManager.Send(_CS_enter_game_pkt);

        //StartCoroutine(LoadSceneAsync());
    }

    public override void Clear()
    {
        // 씬 이동 시 오브젝트 풀 등을 정리
    }


    private void OnDestroy()
    {
        // 씬이 파괴될 때 이벤트 해제 (메모리 누수 방지)
        if (Managers.sceneManagerEx != null)
        {
            Managers.sceneManagerEx.OnEnterGameReceived -= OnReceiveEnterGame;
        }
    }
    private void OnReceiveEnterGame(int mapId)
    {
        _targetMapId = mapId;

        // 서버로부터 맵 ID를 응답받았으므로 씬 로딩 코루틴 실행
        StartCoroutine(LoadSceneAsync());
    }

    IEnumerator LoadSceneAsync()
    {
        //id
        _asyncOp = Managers.sceneManagerEx.LoadSceneAsync(Define.SceneType.Game, _targetMapId);
        
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

