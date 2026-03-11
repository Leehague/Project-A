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
        Managers.objectManager.SpawnPlayer((int)pkt.TempleteId, (int)pkt.PlayerId, true);

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

    public static void Handle_SC_MOVE(PacketSession session, IMessage packet) 
    {
        SC_MOVE pkt = packet as SC_MOVE;
        Debug.Log($"Handle_SC_MOVE: {pkt.ToString()}");
    }
}