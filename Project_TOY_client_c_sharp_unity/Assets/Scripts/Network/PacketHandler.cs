using System;
using Google.Protobuf;
using Protocol;

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

        
        SC_PLAYER_SPAWN spawnPkt = packet as SC_PLAYER_SPAWN;
               
        
        Managers.objectManager.HandleSpawn(packet as SC_PLAYER_SPAWN);
               
        

    }
    public static void Handle_SC_ENTER_GAME(PacketSession session, IMessage packet) 
    {
        SC_ENTER_GAME enterGamePkt = packet as SC_ENTER_GAME;


        //로그인 패킷에서 받았던 playerId(obejcId)와 일치하는지 확인
        if (enterGamePkt.PosInfo.ObjectId != Managers.objectManager.Myplayer_playerId) { return; }

        //서버가 보내준 나의 초기 위치 정보를 저장
        Managers.objectManager.MyplayerPosInfo=enterGamePkt.PosInfo;

        //내 캐릭터 스폰
        Managers.objectManager.SpawnPlayer(enterGamePkt.PosInfo,true);

        //CS_GAME_READY 전송 로직
        Protocol.CS_GAME_READY _CS_GAME_READY = new Protocol.CS_GAME_READY();
        _CS_GAME_READY.PlayerId = Managers.objectManager.Myplayer_playerId; // This playerId must equal to 'Real' player Id in server.
        Managers.networkManager.Send(_CS_GAME_READY);
    }

    public static void Handle_SC_DESPAWN(PacketSession session, IMessage packet) 
    {
        SC_DESPAWN despawnPkt = packet as SC_DESPAWN;

        foreach (ulong id in despawnPkt.PlayerId)
        {
            Managers.objectManager.Remove(id); // 오브젝트 매니저에 제거 요청
        }
    }
    public static void Handle_SC_SKILL(PacketSession session, IMessage packet)
    { 
        //스킬 종류에 따라 분기해야할듯?
    }
    public static void Handle_SC_CHANGE_HP(PacketSession session, IMessage packet) 
    { 
        //바뀐 HP UI에 정보 전달
    }
    public static void Handle_SC_CHANGE_MP(PacketSession session, IMessage packet) 
    {
        //바뀐 MP UI에 정보 전달
    }
}
