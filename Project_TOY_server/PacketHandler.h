#pragma once
#include "Protocol/Protocol.pb.h"
#include "Types.h"
#include "ServerUtils.h"
#define MAX_PACKET_ID 65535

using SessionRef = std::shared_ptr<class Session>;
using PacketHandlerFunc = std::function<bool(SessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[65535];

bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len);
bool Handle_CS_LOGIN(SessionRef& session, Protocol::CS_LOGIN& pkt);
bool Handle_CS_CHAT(SessionRef& session, Protocol::CS_CHAT& pkt);
bool Handle_CS_WHISPER(SessionRef& session, Protocol::CS_WHISPER& pkt);


class PacketHandler
{
public:
    static void Init();
    static bool HandlePacket(SessionRef& session, BYTE* buffer, int32 len)
    {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
        uint16 packetId = header->id;
        if (packetId < MAX_PACKET_ID && GPacketHandler[packetId])
            return GPacketHandler[packetId](session, buffer, len);
        return false;
    }

private:
    template<typename T, typename ProcessFunc>
    static bool HandlePacket(ProcessFunc func, SessionRef& session, BYTE* buffer, int32 len)
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

    GPacketHandler[Protocol::PacketId::PKT_CS_LOGIN] = [](SessionRef& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_LOGIN>(Handle_CS_LOGIN, session, buffer, len);
    };
    GPacketHandler[Protocol::PacketId::PKT_SC_LOGIN_OK] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_CS_CHAT] = [](SessionRef& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_CHAT>(Handle_CS_CHAT, session, buffer, len);
    };
    GPacketHandler[Protocol::PacketId::PKT_SC_CHAT_BROADCAST] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_SC_WHISPER] = Handle_INVALID;
    GPacketHandler[Protocol::PacketId::PKT_CS_WHISPER] = [](SessionRef& session, BYTE* buffer, int32 len) {
        return HandlePacket<Protocol::CS_WHISPER>(Handle_CS_WHISPER, session, buffer, len);
    };
}
