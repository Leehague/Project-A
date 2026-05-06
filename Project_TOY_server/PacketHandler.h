#pragma once
#include "Protocol/Protocol.pb.h"
#include "Types.h"
#include "ServerUtils.h"
#define MAX_PACKET_ID 65535

using SessionPtr = std::shared_ptr<class Session>;
using PacketHandlerFunc = std::function<bool(SessionPtr&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[65535];

bool Handle_INVALID(SessionPtr& session, BYTE* buffer, int32 len);
bool Handle_CS_LOGIN(SessionPtr& session, Protocol::CS_LOGIN& pkt);
bool Handle_CS_CHAT(SessionPtr& session, Protocol::CS_CHAT& pkt);
bool Handle_CS_WHISPER(SessionPtr& session, Protocol::CS_WHISPER& pkt);
bool Handle_CS_MOVING(SessionPtr& session, Protocol::CS_MOVING& pkt);
bool Handle_CS_ENTER_GAME(SessionPtr& session, Protocol::CS_ENTER_GAME& pkt);
bool Handle_CS_GAME_READY(SessionPtr& session, Protocol::CS_GAME_READY& pkt);
bool Handle_CS_SKILL(SessionPtr& session, Protocol::CS_SKILL& pkt);


class PacketHandler
{
public:
    static void Init();
    static bool HandlePacket(SessionPtr& session, BYTE* buffer, int32 len)
    {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
        uint16 packetId = header->id;
        if (packetId < MAX_PACKET_ID && GPacketHandler[packetId])
            return GPacketHandler[packetId](session, buffer, len);
        return false;
    }

private:
    template<typename T, typename ProcessFunc>
    static bool HandlePacket(ProcessFunc func, SessionPtr& session, BYTE* buffer, int32 len)
    {
        T pkt;
        if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
            return false;
        return func(session, pkt);
    }
};

inline void PacketHandler::Init()
{
    for (int32 i = 0; i < 65535; i++) GPacketHandler[i] = Handle_INVALID;

    GPacketHandler[Protocol::PacketId::PKT_CS_LOGIN] = [](SessionPtr& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_LOGIN>(Handle_CS_LOGIN, session, buffer, len);
    };
    GPacketHandler[Protocol::PacketId::PKT_SC_LOGIN_OK] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_CS_CHAT] = [](SessionPtr& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_CHAT>(Handle_CS_CHAT, session, buffer, len);
    };
    GPacketHandler[Protocol::PacketId::PKT_SC_CHAT_BROADCAST] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_SC_WHISPER] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_CS_WHISPER] = [](SessionPtr& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_WHISPER>(Handle_CS_WHISPER, session, buffer, len);
    };
    GPacketHandler[Protocol::PacketId::PKT_CS_MOVING] = [](SessionPtr& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_MOVING>(Handle_CS_MOVING, session, buffer, len);
    };
    GPacketHandler[Protocol::PacketId::PKT_SC_MOVING] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_CS_ENTER_GAME] = [](SessionPtr& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_ENTER_GAME>(Handle_CS_ENTER_GAME, session, buffer, len);
    };
    GPacketHandler[Protocol::PacketId::PKT_SC_ENTER_GAME] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_SC_PLAYER_SPAWN] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_CS_GAME_READY] = [](SessionPtr& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_GAME_READY>(Handle_CS_GAME_READY, session, buffer, len);
    };
    GPacketHandler[Protocol::PacketId::PKT_SC_PLAYER_DESPAWN] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_CS_SKILL] = [](SessionPtr& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_SKILL>(Handle_CS_SKILL, session, buffer, len);
    };
    GPacketHandler[Protocol::PacketId::PKT_SC_SKILL] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_SC_CHANGE_HP] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_SC_CHANGE_MP] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_SC_MONSTER_SPAWN] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_SC_MONSTER_DEAD] = Handle_INVALID;
}
