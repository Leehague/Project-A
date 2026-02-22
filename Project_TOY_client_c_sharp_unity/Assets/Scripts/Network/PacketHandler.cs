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

        //Temp : ObjectManager를 이용해서 리소스(prefeb) 가져와서 플레이어 캐릭터 로딩 하는 코드 
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


}