using System;
using Google.Protobuf;
using Protocol;
using Unity.VisualScripting.Dependencies.Sqlite;
using UnityEngine;

public class PacketHandler
{
    public static void Handle_SC_LOGIN_OK(PacketSession session, IMessage packet)
    {
        SC_LOGIN_OK pkt = packet as SC_LOGIN_OK;
        Debug.Log($"Handle_SC_LOGIN_OK: {pkt.ToString()}");

        Managers.objectManager.Myplayer_playerId = pkt.PlayerId;


        if (pkt.Success)
        {

            Managers.sceneManagerEx.LoadScene(SceneType.Loading);
        }
        else 
        {
            Debug.Log("Login is not ok");
        }
        
    }

    public static void Handle_SC_CHAT_BROADCAST(PacketSession session, IMessage packet)
    {
        SC_CHAT_BROADCAST pkt = packet as SC_CHAT_BROADCAST;
        Debug.Log($"Handle_SC_CHAT_BROADCAST: {pkt.ToString()}");
    }

    public static void Handle_SC_WHISPER(PacketSession session, IMessage packet)
    {
        SC_WHISPER pkt = packet as SC_WHISPER;
        Debug.Log($"Handle_SC_WHISPER: {pkt.ToString()}");
    }

    public static void Handle_SC_MOVING(PacketSession session, IMessage packet)
    {
        SC_MOVING movepkt = packet as SC_MOVING;
        if (movepkt == null) return;
        GameObject go = Managers.objectManager.Find(movepkt.PosInfo.ObjectId);
        if (go == null) return;
        PlayerController pc;
        if (go.TryGetComponent<PlayerController>(out pc))
        {
            if (pc == null) { return; }

            if (pc.IsMyPlayer)
            {
                // [내 캐릭터 로직]
                // 서버가 보내온 위치(확정된 위치)와 현재 내 유니티 위치를 비교
                Vector3 serverPos = new Vector3(movepkt.PosInfo.X, movepkt.PosInfo.Y, movepkt.PosInfo.Z);
                float distance = Vector3.Distance(go.transform.position, serverPos);

                // 오차가 임계값(예: 0.5m)보다 크면 강제 보정
                if (distance > 0.5f)
                {
                    // 너무 멀면 순간이동 시키거나, 부드럽게 위치를 땡겨옴 (Reconciliation)
                    go.transform.position = serverPos;
                }
            }
            else
            {
                // [타인 캐릭터 로직]
                // 다른 유저가 움직인 것이므로 목표 위치만 갱신해줌
                pc.RefreshPos(movepkt.PosInfo);
            }
        }
        else 
        {
            Debug.Log("not found player controller");
        }
    }

    public static void Handle_SC_PLAYER_SPAWN(PacketSession session, IMessage packet)
    { 
        SC_PLAYER_SPAWN spawn_Pkt = packet as SC_PLAYER_SPAWN;

        Debug.Log("Handle_SC_PLAYER_SPAWN 수신");

        if (spawn_Pkt == null) 
        {
            Debug.Log("spawn_Pkt is null");
            return; 
        }

        foreach (PosInfo pos in spawn_Pkt.PlayersPosInfo) 
        {
            Debug.Log($"현재 소환 시점의 씬: {UnityEngine.SceneManagement.SceneManager.GetActiveScene().name}");

            bool IsMyplayer = false;
            if (pos.ObjectId == Managers.objectManager.Myplayer_playerId) { IsMyplayer = true; }
            Managers.objectManager.SpawnPlayer(pos,IsMyplayer);
        }

        
    }
    public static void Handle_SC_ENTER_GAME(PacketSession session, IMessage packet) 
    {
        SC_ENTER_GAME enterGamePkt = packet as SC_ENTER_GAME;

        // 1. 서버가 보내준 나의 초기 위치 정보를 저장
        Managers.objectManager.MyplayerPosInfo=enterGamePkt.PosInfo;

        // 2. 현재 씬이 로딩 씬인지 확인하고 승인 함수 호출
        LoadingScene loadingScene = Managers.sceneManagerEx.CurrentScene as LoadingScene;
        if (loadingScene != null)
        {
            loadingScene.OnServerEnterAccepted();
        }
        else
        {
            // 만약 로딩 씬이 아닌데 이 패킷이 왔다면 예외 처리
            Debug.LogWarning("Received SC_ENTER_GAME but current scene is not LoadingScene");
        }
    }

    
}