using System.Collections;
using UnityEngine;
using UnityEngine.SceneManagement;

public class LoadingScene : BaseScene
{
    private AsyncOperation _asyncOp;

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
        // 1. 실제 게임 씬 비동기 로드 시작
        string name = System.Enum.GetName(typeof(SceneType), SceneType.Game);
        _asyncOp = SceneManager.LoadSceneAsync(name);
        _asyncOp.allowSceneActivation = false; // 다 로드되어도 화면 전환은 일단 대기

        while (_asyncOp.progress < 0.9f)
        {
            // 여기서 로딩 게이지 UI 업데이트
            yield return null;
        }

        // 2. 리소스 로딩이 끝났으니 서버에 진입 허가 요청
        Protocol.CS_ENTER_GAME enterPkt = new Protocol.CS_ENTER_GAME();
        Managers.networkManager.Send(enterPkt);
        Debug.Log("CS_ENTER_GAME 전송, 서버 승인 대기 중...");
    }

    // 3. PacketHandler에서 SC_ENTER_GAME을 받으면 이 함수를 실행하게 함
    public void OnServerEnterAccepted()
    {
        Debug.Log("서버 승인 완료, 게임 씬으로 전환합니다.");
        _asyncOp.allowSceneActivation = true; // 이제 화면 전환!
    }
}

