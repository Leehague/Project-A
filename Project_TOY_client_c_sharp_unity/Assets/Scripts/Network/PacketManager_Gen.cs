using System;
using System.Collections.Generic;
using Google.Protobuf;
using Protocol;

public partial class PacketManager
{
    public void Register()
    {   
        _onRecv.Add((ushort)PacketId.PktScLoginOk, (s, b, sz) => MakePacket<SC_LOGIN_OK>(s, b, sz, (ushort)PacketId.PktScLoginOk));
        _handler.Add((ushort)PacketId.PktScLoginOk, PacketHandler.Handle_SC_LOGIN_OK);
        _onRecv.Add((ushort)PacketId.PktScChatBroadcast, (s, b, sz) => MakePacket<SC_CHAT_BROADCAST>(s, b, sz, (ushort)PacketId.PktScChatBroadcast));
        _handler.Add((ushort)PacketId.PktScChatBroadcast, PacketHandler.Handle_SC_CHAT_BROADCAST);
        _onRecv.Add((ushort)PacketId.PktScWhisper, (s, b, sz) => MakePacket<SC_WHISPER>(s, b, sz, (ushort)PacketId.PktScWhisper));
        _handler.Add((ushort)PacketId.PktScWhisper, PacketHandler.Handle_SC_WHISPER);

    }
}