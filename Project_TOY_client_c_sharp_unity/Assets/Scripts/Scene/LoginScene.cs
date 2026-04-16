using UnityEngine;
using Google.Protobuf;
using Protocol;
using System.Collections;


public class LoginScene : BaseScene 
{
    
    protected override void Init()
    {
        base.Init(); // 부모의 공통 초기화 수행
        SceneType = Define.SceneType.Login;

        //StartCoroutine(CoLogin()); // send를 두번 하는 문제를 유발해서 그냥 connected 가 packetsession에서 되는 시점으로 로직 이동
    }
    //IEnumerator CoLogin()
    //{
    //    // 세션이 준비될 때까지 대기
    //    while (Managers.networkManager.IsSession_NULL())
    //    {
    //        yield return null;
    //    }

    //    if (_loginSent == false)
    //    {
    //        _loginSent = true;
    //        CS_LOGIN loginPacket = new CS_LOGIN();

    //        loginPacket.UserId = 123123;
    //        loginPacket.Password = "password 1234";

    //        Managers.networkManager.Send(loginPacket);
    //    }
    //}
    public override void Clear()
    {
        // 씬 이동 시 오브젝트 풀 등을 정리
    }
}