using UnityEngine;

public class GameScene : BaseScene
{
    
    protected override void Init()
    {
        base.Init(); // 부모의 공통 초기화 수행
        SceneType = Define.SceneType.Game;

        Debug.Log("Game Scene INIT");

        
        //CS_ENTER_GAME 전송
        Protocol.CS_ENTER_GAME _CS_enter_game_pkt = new Protocol.CS_ENTER_GAME();
        Managers.networkManager.Send(_CS_enter_game_pkt);


        
    }

    void Start()
    {
        // 커서를 화면 중앙에 고정하고 숨김
        Cursor.lockState = CursorLockMode.Locked;
        Cursor.visible = false;
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
