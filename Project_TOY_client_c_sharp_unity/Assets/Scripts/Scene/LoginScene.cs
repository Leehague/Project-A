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

       
    }
   
    public override void Clear()
    {
        // 씬 이동 시 오브젝트 풀 등을 정리
    }
}