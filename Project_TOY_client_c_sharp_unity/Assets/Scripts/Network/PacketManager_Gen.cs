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
        _onRecv.Add((ushort)PacketId.PktScLoginOk, (s, b, off, sz) => MakePacket<SC_LOGIN_OK>(s, b, off, sz, (ushort)PacketId.PktScLoginOk));
        _handler.Add((ushort)PacketId.PktScLoginOk, PacketHandler.Handle_SC_LOGIN_OK);
        _typeToId.Add(typeof(CS_CHAT), (ushort)PacketId.PktCsChat);
        _typeToId.Add(typeof(SC_CHAT_BROADCAST), (ushort)PacketId.PktScChatBroadcast);
        _onRecv.Add((ushort)PacketId.PktScChatBroadcast, (s, b, off, sz) => MakePacket<SC_CHAT_BROADCAST>(s, b, off, sz, (ushort)PacketId.PktScChatBroadcast));
        _handler.Add((ushort)PacketId.PktScChatBroadcast, PacketHandler.Handle_SC_CHAT_BROADCAST);
        _typeToId.Add(typeof(SC_WHISPER), (ushort)PacketId.PktScWhisper);
        _onRecv.Add((ushort)PacketId.PktScWhisper, (s, b, off, sz) => MakePacket<SC_WHISPER>(s, b, off, sz, (ushort)PacketId.PktScWhisper));
        _handler.Add((ushort)PacketId.PktScWhisper, PacketHandler.Handle_SC_WHISPER);
        _typeToId.Add(typeof(CS_WHISPER), (ushort)PacketId.PktCsWhisper);
        _typeToId.Add(typeof(CS_MOVING), (ushort)PacketId.PktCsMoving);
        _typeToId.Add(typeof(SC_MOVING), (ushort)PacketId.PktScMoving);
        _onRecv.Add((ushort)PacketId.PktScMoving, (s, b, off, sz) => MakePacket<SC_MOVING>(s, b, off, sz, (ushort)PacketId.PktScMoving));
        _handler.Add((ushort)PacketId.PktScMoving, PacketHandler.Handle_SC_MOVING);
        _typeToId.Add(typeof(CS_ENTER_GAME), (ushort)PacketId.PktCsEnterGame);
        _typeToId.Add(typeof(SC_ENTER_GAME), (ushort)PacketId.PktScEnterGame);
        _onRecv.Add((ushort)PacketId.PktScEnterGame, (s, b, off, sz) => MakePacket<SC_ENTER_GAME>(s, b, off, sz, (ushort)PacketId.PktScEnterGame));
        _handler.Add((ushort)PacketId.PktScEnterGame, PacketHandler.Handle_SC_ENTER_GAME);
        _typeToId.Add(typeof(SC_PLAYER_SPAWN), (ushort)PacketId.PktScPlayerSpawn);
        _onRecv.Add((ushort)PacketId.PktScPlayerSpawn, (s, b, off, sz) => MakePacket<SC_PLAYER_SPAWN>(s, b, off, sz, (ushort)PacketId.PktScPlayerSpawn));
        _handler.Add((ushort)PacketId.PktScPlayerSpawn, PacketHandler.Handle_SC_PLAYER_SPAWN);
        _typeToId.Add(typeof(CS_GAME_READY), (ushort)PacketId.PktCsGameReady);
        _typeToId.Add(typeof(SC_PLAYER_DESPAWN), (ushort)PacketId.PktScPlayerDespawn);
        _onRecv.Add((ushort)PacketId.PktScPlayerDespawn, (s, b, off, sz) => MakePacket<SC_PLAYER_DESPAWN>(s, b, off, sz, (ushort)PacketId.PktScPlayerDespawn));
        _handler.Add((ushort)PacketId.PktScPlayerDespawn, PacketHandler.Handle_SC_PLAYER_DESPAWN);
        _typeToId.Add(typeof(CS_SKILL), (ushort)PacketId.PktCsSkill);
        _typeToId.Add(typeof(SC_SKILL), (ushort)PacketId.PktScSkill);
        _onRecv.Add((ushort)PacketId.PktScSkill, (s, b, off, sz) => MakePacket<SC_SKILL>(s, b, off, sz, (ushort)PacketId.PktScSkill));
        _handler.Add((ushort)PacketId.PktScSkill, PacketHandler.Handle_SC_SKILL);
        _typeToId.Add(typeof(SC_CHANGE_HP), (ushort)PacketId.PktScChangeHp);
        _onRecv.Add((ushort)PacketId.PktScChangeHp, (s, b, off, sz) => MakePacket<SC_CHANGE_HP>(s, b, off, sz, (ushort)PacketId.PktScChangeHp));
        _handler.Add((ushort)PacketId.PktScChangeHp, PacketHandler.Handle_SC_CHANGE_HP);
        _typeToId.Add(typeof(SC_CHANGE_MP), (ushort)PacketId.PktScChangeMp);
        _onRecv.Add((ushort)PacketId.PktScChangeMp, (s, b, off, sz) => MakePacket<SC_CHANGE_MP>(s, b, off, sz, (ushort)PacketId.PktScChangeMp));
        _handler.Add((ushort)PacketId.PktScChangeMp, PacketHandler.Handle_SC_CHANGE_MP);
        _typeToId.Add(typeof(SC_MONSTER_SPAWN), (ushort)PacketId.PktScMonsterSpawn);
        _onRecv.Add((ushort)PacketId.PktScMonsterSpawn, (s, b, off, sz) => MakePacket<SC_MONSTER_SPAWN>(s, b, off, sz, (ushort)PacketId.PktScMonsterSpawn));
        _handler.Add((ushort)PacketId.PktScMonsterSpawn, PacketHandler.Handle_SC_MONSTER_SPAWN);
        _typeToId.Add(typeof(SC_MONSTER_DEAD), (ushort)PacketId.PktScMonsterDead);
        _onRecv.Add((ushort)PacketId.PktScMonsterDead, (s, b, off, sz) => MakePacket<SC_MONSTER_DEAD>(s, b, off, sz, (ushort)PacketId.PktScMonsterDead));
        _handler.Add((ushort)PacketId.PktScMonsterDead, PacketHandler.Handle_SC_MONSTER_DEAD);
        _typeToId.Add(typeof(CS_OWNED_ITEM_REQUEST), (ushort)PacketId.PktCsOwnedItemRequest);
        _typeToId.Add(typeof(SC_ITEM_RESPONSE), (ushort)PacketId.PktScItemResponse);
        _onRecv.Add((ushort)PacketId.PktScItemResponse, (s, b, off, sz) => MakePacket<SC_ITEM_RESPONSE>(s, b, off, sz, (ushort)PacketId.PktScItemResponse));
        _handler.Add((ushort)PacketId.PktScItemResponse, PacketHandler.Handle_SC_ITEM_RESPONSE);
        _typeToId.Add(typeof(CS_QUEST_CREATED_REQUEST), (ushort)PacketId.PktCsQuestCreatedRequest);
        _typeToId.Add(typeof(SC_QUEST_CREATED_RESPONSE), (ushort)PacketId.PktScQuestCreatedResponse);
        _onRecv.Add((ushort)PacketId.PktScQuestCreatedResponse, (s, b, off, sz) => MakePacket<SC_QUEST_CREATED_RESPONSE>(s, b, off, sz, (ushort)PacketId.PktScQuestCreatedResponse));
        _handler.Add((ushort)PacketId.PktScQuestCreatedResponse, PacketHandler.Handle_SC_QUEST_CREATED_RESPONSE);
        _typeToId.Add(typeof(CS_QUEST_ACCEPT_REQUEST), (ushort)PacketId.PktCsQuestAcceptRequest);
        _typeToId.Add(typeof(SC_QUEST_ACCEPT_RESPONSE), (ushort)PacketId.PktScQuestAcceptResponse);
        _onRecv.Add((ushort)PacketId.PktScQuestAcceptResponse, (s, b, off, sz) => MakePacket<SC_QUEST_ACCEPT_RESPONSE>(s, b, off, sz, (ushort)PacketId.PktScQuestAcceptResponse));
        _handler.Add((ushort)PacketId.PktScQuestAcceptResponse, PacketHandler.Handle_SC_QUEST_ACCEPT_RESPONSE);
        _typeToId.Add(typeof(SC_QUEST_PROGRESS_UPDATE), (ushort)PacketId.PktScQuestProgressUpdate);
        _onRecv.Add((ushort)PacketId.PktScQuestProgressUpdate, (s, b, off, sz) => MakePacket<SC_QUEST_PROGRESS_UPDATE>(s, b, off, sz, (ushort)PacketId.PktScQuestProgressUpdate));
        _handler.Add((ushort)PacketId.PktScQuestProgressUpdate, PacketHandler.Handle_SC_QUEST_PROGRESS_UPDATE);
        _typeToId.Add(typeof(CS_QUEST_LIST_REQUEST), (ushort)PacketId.PktCsQuestListRequest);
        _typeToId.Add(typeof(SC_QUEST_LIST_RESPONSE), (ushort)PacketId.PktScQuestListResponse);
        _onRecv.Add((ushort)PacketId.PktScQuestListResponse, (s, b, off, sz) => MakePacket<SC_QUEST_LIST_RESPONSE>(s, b, off, sz, (ushort)PacketId.PktScQuestListResponse));
        _handler.Add((ushort)PacketId.PktScQuestListResponse, PacketHandler.Handle_SC_QUEST_LIST_RESPONSE);

    }
}