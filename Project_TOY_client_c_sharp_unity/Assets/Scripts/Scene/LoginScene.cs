using UnityEngine;
using Google.Protobuf;
using Protocol;


public class LoginScene : BaseScene 
{
    protected override void Init()
    {
        base.Init(); // 부모의 공통 초기화 수행
        SceneType = SceneType.Login;

        CS_LOGIN loginPacket = new CS_LOGIN();
        //TEMP, TOOD: 실제로는 user id와 password등의 인증정보에 관한 로직이 들어가야함
        loginPacket.UserId = 123123;
        loginPacket.Password = "password 1234";

        Managers.networkManager.Send(loginPacket);
        
        Debug.Log("LoginScene 로딩 완료, CS_LOGIN 전송");
    }

    public override void Clear()
    {
        // 씬 이동 시 오브젝트 풀 등을 정리
    }
}