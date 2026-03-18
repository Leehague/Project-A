using System.Collections;
using UnityEngine;
using UnityEngine.SceneManagement;

public class LoadingScene : BaseScene
{
    private AsyncOperation _asyncOp;


    protected override void Awake()
    {
        
    }

    private void Start()
    {
        Init();
    }
    protected override void Init()
    {
        base.Init(); // 부모의 공통 초기화 수행
        SceneType = SceneType.Loading;
        

        StartCoroutine(LoadSceneAsync());
    }

    public override void Clear()
    {
        // 씬 이동 시 오브젝트 풀 등을 정리
    }

    IEnumerator LoadSceneAsync()
    {
        // 1. 혹시 모를 시간 정지 해제
        Time.timeScale = 1.0f;

        string name = System.Enum.GetName(typeof(SceneType), SceneType.Game);
        _asyncOp = SceneManager.LoadSceneAsync(name);
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

        // 3. 루프를 빠져나온 후 반드시 찍혀야 하는 로그
        Debug.Log("로딩 완료! 서버에 패킷을 보냅니다.");

        Protocol.CS_ENTER_GAME enterPkt = new Protocol.CS_ENTER_GAME();
        Managers.networkManager.Send(enterPkt);
        Debug.Log("CS_ENTER_GAME 전송 완료.");
    }

    // 3. PacketHandler에서 SC_ENTER_GAME을 받으면 이 함수를 실행하게 함
    public void OnServerEnterAccepted()
    {
        Debug.Log("서버 승인 완료, 게임 씬으로 전환합니다.");
        _asyncOp.allowSceneActivation = true; // 이제 화면 전환!
    }
}

