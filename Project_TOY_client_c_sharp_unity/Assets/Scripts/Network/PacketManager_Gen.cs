using System;
using System.Collections.Generic;
using Google.Protobuf;
using Protocol;

public partial class PacketManager
{
    public void Register()
    {
        _typeToId.Add(typeof(CS_LOGIN), (ushort)PacketId.PktCsLogin);
        _typeToId.Add(typeof(SC_LOGIN_OK), (ushort)PacketId.PktScLoginOk);
        _onRecv.Add((ushort)PacketId.PktScLoginOk, (s, b, sz) => MakePacket<SC_LOGIN_OK>(s, b, sz, (ushort)PacketId.PktScLoginOk));
        _handler.Add((ushort)PacketId.PktScLoginOk, PacketHandler.Handle_SC_LOGIN_OK);
        _typeToId.Add(typeof(CS_CHAT), (ushort)PacketId.PktCsChat);
        _typeToId.Add(typeof(SC_CHAT_BROADCAST), (ushort)PacketId.PktScChatBroadcast);
        _onRecv.Add((ushort)PacketId.PktScChatBroadcast, (s, b, sz) => MakePacket<SC_CHAT_BROADCAST>(s, b, sz, (ushort)PacketId.PktScChatBroadcast));
        _handler.Add((ushort)PacketId.PktScChatBroadcast, PacketHandler.Handle_SC_CHAT_BROADCAST);
        _typeToId.Add(typeof(SC_WHISPER), (ushort)PacketId.PktScWhisper);
        _onRecv.Add((ushort)PacketId.PktScWhisper, (s, b, sz) => MakePacket<SC_WHISPER>(s, b, sz, (ushort)PacketId.PktScWhisper));
        _handler.Add((ushort)PacketId.PktScWhisper, PacketHandler.Handle_SC_WHISPER);
        _typeToId.Add(typeof(CS_WHISPER), (ushort)PacketId.PktCsWhisper);
        _typeToId.Add(typeof(CS_MOVING), (ushort)PacketId.PktCsMoving);
        _typeToId.Add(typeof(SC_MOVING), (ushort)PacketId.PktScMoving);
        _onRecv.Add((ushort)PacketId.PktScMoving, (s, b, sz) => MakePacket<SC_MOVING>(s, b, sz, (ushort)PacketId.PktScMoving));
        _handler.Add((ushort)PacketId.PktScMoving, PacketHandler.Handle_SC_MOVING);
        _typeToId.Add(typeof(CS_ENTER_GAME), (ushort)PacketId.PktCsEnterGame);
        _typeToId.Add(typeof(SC_ENTER_GAME), (ushort)PacketId.PktScEnterGame);
        _onRecv.Add((ushort)PacketId.PktScEnterGame, (s, b, sz) => MakePacket<SC_ENTER_GAME>(s, b, sz, (ushort)PacketId.PktScEnterGame));
        _handler.Add((ushort)PacketId.PktScEnterGame, PacketHandler.Handle_SC_ENTER_GAME);
        _typeToId.Add(typeof(SC_PLAYER_SPAWN), (ushort)PacketId.PktScPlayerSpawn);
        _onRecv.Add((ushort)PacketId.PktScPlayerSpawn, (s, b, sz) => MakePacket<SC_PLAYER_SPAWN>(s, b, sz, (ushort)PacketId.PktScPlayerSpawn));
        _handler.Add((ushort)PacketId.PktScPlayerSpawn, PacketHandler.Handle_SC_PLAYER_SPAWN);
        _typeToId.Add(typeof(CS_GAME_READY), (ushort)PacketId.PktCsGameReady);
        _typeToId.Add(typeof(SC_DESPAWN), (ushort)PacketId.PktScDespawn);
        _onRecv.Add((ushort)PacketId.PktScDespawn, (s, b, sz) => MakePacket<SC_DESPAWN>(s, b, sz, (ushort)PacketId.PktScDespawn));
        _handler.Add((ushort)PacketId.PktScDespawn, PacketHandler.Handle_SC_DESPAWN);
        _typeToId.Add(typeof(CS_SKILL), (ushort)PacketId.PktCsSkill);
        _typeToId.Add(typeof(SC_SKILL), (ushort)PacketId.PktScSkill);
        _onRecv.Add((ushort)PacketId.PktScSkill, (s, b, sz) => MakePacket<SC_SKILL>(s, b, sz, (ushort)PacketId.PktScSkill));
        _handler.Add((ushort)PacketId.PktScSkill, PacketHandler.Handle_SC_SKILL);
        _typeToId.Add(typeof(SC_CHANGE_HP), (ushort)PacketId.PktScChangeHp);
        _onRecv.Add((ushort)PacketId.PktScChangeHp, (s, b, sz) => MakePacket<SC_CHANGE_HP>(s, b, sz, (ushort)PacketId.PktScChangeHp));
        _handler.Add((ushort)PacketId.PktScChangeHp, PacketHandler.Handle_SC_CHANGE_HP);
        _typeToId.Add(typeof(SC_CHANGE_MP), (ushort)PacketId.PktScChangeMp);
        _onRecv.Add((ushort)PacketId.PktScChangeMp, (s, b, sz) => MakePacket<SC_CHANGE_MP>(s, b, sz, (ushort)PacketId.PktScChangeMp));
        _handler.Add((ushort)PacketId.PktScChangeMp, PacketHandler.Handle_SC_CHANGE_MP);

    }
}