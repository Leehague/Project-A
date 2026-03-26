using UnityEngine;

public class GameScene : BaseScene
{
    
    protected override void Init()
    {
        base.Init(); // 부모의 공통 초기화 수행
        SceneType = SceneType.Game;

        Debug.Log("Game Scene INIT");

        //Managers.objectManager.OnSceneReady(); // 스폰 패킷이 와있다면 여기서 처리 (하지만 CS_GAME_READY 이 추가 됬으므로 정상적이라면 없어야 함)

        //// 여기서 네트워크를 켜주는 것도 방법입니다.
        //Managers.networkManager.enabled = true;
        //Managers.networkManager.CanPushPacket = true;

        //CS_ENTER_GAME 전송
        Protocol.CS_ENTER_GAME _CS_enter_game_pkt = new Protocol.CS_ENTER_GAME();
        Managers.networkManager.Send(_CS_enter_game_pkt);


        
    }

    public override void Clear()
    {
        // 씬 이동 시 오브젝트 풀 등을 정리
    }
}