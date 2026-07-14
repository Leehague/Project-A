using UnityEngine;

public class GameScene : BaseScene
{
    
    protected override void Init()
    {
        base.Init(); // 부모의 공통 초기화 수행
        SceneType = Define.SceneType.Game;

        Debug.Log("Game Scene INIT");
    }

    void Start()
    {
        // 커서를 화면 중앙에 고정하고 숨김
        Cursor.lockState = CursorLockMode.Locked;
        Cursor.visible = false;


        // [추가] 씬 로드가 완전히 끝난 시점에 캐릭터를 스폰하고 준비 패킷 송신
        if (Managers.objectManager.MyplayerPosInfo != null)
        {
            // 1. 보관 중이던 위치 정보와 템플릿 정보로 내 캐릭터 스폰
            Managers.objectManager.SpawnPlayer(
                Managers.objectManager.MyplayerPosInfo,
                Managers.objectManager.Myplayer_TemplateId,
                true
            );
            // 2. 서버에 로딩 완료 및 게임 참여 준비가 끝났음을 선언
            Protocol.CS_GAME_READY gameReadyPkt = new Protocol.CS_GAME_READY();
            gameReadyPkt.PlayerId = Managers.objectManager.Myplayer_playerId;
            Managers.networkManager.Send(gameReadyPkt);
            Debug.Log("[GameScene] 인게임 로드가 완전히 완료되어 CS_GAME_READY를 전송했습니다.");
        }
        else
        {
            Debug.LogError("[GameScene] 로딩 씬에서 넘겨받은 스폰 데이터가 누락되었습니다.");
        }
    }

    void Update()
    {
        // ESC를 누르면 다시 커서를 보여주는 방어 로직 (디버깅용)
        if (Input.GetKeyDown(KeyCode.Escape))
        {
            Cursor.lockState = CursorLockMode.None;
            Cursor.visible = true;
        }
    }

    public override void Clear()
    {
        // 씬 이동 시 오브젝트 풀 등을 정리
    }
}
