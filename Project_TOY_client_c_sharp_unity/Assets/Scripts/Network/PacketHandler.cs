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

        //내 플레이어 스폰
        //패킷정보 캐스팅에서 문제 생길가능성 있음
        Managers.objectManager.SpawnPlayer(pkt.TempleteId, pkt.PlayerId, true);

        //TODO : pkt.PlayerId 캐싱. 클라이언트 에서도 이 playerId(objectId)로 objectManager에서 
        //object들을 관리하고 있으므로 자신의 playerId(objectId)를 알고 있을 필요가 있음

        //TODO : 다른 플레이어들 스폰

        //씬 매니저를 만들어서 그곳의 씬 초기화 함수를 호출하는 방식으로 변경예정
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

    }
}