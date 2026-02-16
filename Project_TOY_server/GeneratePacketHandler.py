# -*- coding: utf-8 -*-
import re
import os

def generate():
    # --- 경로 설정 (본인 환경에 맞게 수정하세요) ---
    proto_path = r"C:\Users\leehague\Desktop\Project A\Common\Protocol.proto"
    server_output_path = r"C:\Users\leehague\Desktop\Project A\Project_TOY_server\PacketHandler.h"
    # Unity 프로젝트 내의 스크립트 폴더 경로
    unity_output_path = r"C:\Users\leehague\Desktop\Project A\Project_TOY_client_c_sharp_unity\Assets\Scripts\Network"
    # ------------------------------------------

    if not os.path.exists(os.path.dirname(unity_output_path)):
        os.makedirs(unity_output_path)

    with open(proto_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Message 및 Enum 추출
    messages = re.findall(r'message\s+(\w+)\s*\{', content)
    packet_enums = re.findall(r'(PKT_\w+)\s*=\s*\d+', content)

    # ==========================================
    # PART 1: 서버용 C++ 코드 생성 (기존 로직 유지)
    # ==========================================
    handler_decls = ""
    register_part = "inline void PacketHandler::Init()\n{\n"
    register_part += "    for (int32 i = 0; i < 65535; i++) GPacketHandler[i] = Handle_INVALID;\n\n"

    for pkt_enum in packet_enums:
        msg_name = pkt_enum.replace("PKT_", "")
        if msg_name in messages:
            if "CS_" in msg_name:
                handler_decls += f"bool Handle_{msg_name}(SessionRef& session, Protocol::{msg_name}& pkt);\n"
                register_part += f"    GPacketHandler[Protocol::PacketId::{pkt_enum}] = [](SessionRef& session, BYTE* buffer, int32 len) {{\n"
                register_part += f"        return HandlePacket<Protocol::{msg_name}>(Handle_{msg_name}, session, buffer, len);\n"
                register_part += f"    }};\n"
            else:
                register_part += f"    GPacketHandler[Protocol::PacketId::{pkt_enum}] = Handle_INVALID;\n"

    register_part += "}"

    server_full_content = f"""#pragma once
#include "Protocol/Protocol.pb.h"
#include "Types.h"
#include "ServerUtils.h"
#define MAX_PACKET_ID 65535

using SessionRef = std::shared_ptr<class Session>;
using PacketHandlerFunc = std::function<bool(SessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[65535];

bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len);
{handler_decls}

class PacketHandler
{{
public:
    static void Init();
    static bool HandlePacket(SessionRef& session, BYTE* buffer, int32 len)
    {{
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
        uint16 packetId = header->id;
        if (packetId < MAX_PACKET_ID && GPacketHandler[packetId])
            return GPacketHandler[packetId](session, buffer, len);
        return false;
    }}

private:
    template<typename T, typename ProcessFunc>
    static bool HandlePacket(ProcessFunc func, SessionRef& session, BYTE* buffer, int32 len)
    {{
        T pkt;
        if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
            return false;
        return func(session, pkt);
    }}
}};

{register_part}
"""
    with open(server_output_path, 'w', encoding='utf-8') as f:
        f.write(server_full_content)

    # ==========================================
    # PART 2: 유니티용 C# 코드 생성 (추가됨)
    # ==========================================
    
    # C#용 Enum 변환 (PKT_SC_LOGIN_OK -> PktScLoginOk)
    def to_camel_case(s):
        return "".join(word.capitalize() for word in s.lower().split('_'))

    csharp_register_content = ""
    csharp_handler_content = ""

    for pkt_enum in packet_enums:
        msg_name = pkt_enum.replace("PKT_", "")
        if msg_name in messages:
            if "SC_" in msg_name: # 클라이언트가 서버로부터 받는 패킷
                enum_name = to_camel_case(pkt_enum)
                
                # csharp_register_content += f"        _onRecv.Add((ushort)PacketId.{enum_name}, MakePacket<{msg_name}>);\n"
                csharp_register_content += f"        _onRecv.Add((ushort)PacketId.{enum_name}, (s, b, sz) => MakePacket<{msg_name}>(s, b, sz, (ushort)PacketId.{enum_name}));\n"
                csharp_register_content += f"        _handler.Add((ushort)PacketId.{enum_name}, PacketHandler.Handle_{msg_name});\n"
                
                csharp_handler_content += f"""    public static void Handle_{msg_name}(PacketSession session, IMessage packet)
    {{
        {msg_name} pkt = packet as {msg_name};
        Debug.Log($"Handle_{msg_name}: {{pkt.ToString()}}");
    }}

"""

    # 2-1. PacketManager_Gen.cs (매번 덮어씀)
    manager_gen_path = os.path.join(unity_output_path, "PacketManager_Gen.cs")
    manager_gen_code = f"""using System;
using System.Collections.Generic;
using Google.Protobuf;
using Protocol;

public partial class PacketManager
{{
    public void Register()
    {{
{csharp_register_content}
    }}
}}"""
    with open(manager_gen_path, 'w', encoding='utf-8') as f:
        f.write(manager_gen_code)

    # 2-2. PacketHandler.cs (파일이 없을 때만 생성 - 로직 보호)
    handler_path = os.path.join(unity_output_path, "PacketHandler.cs")
    if not os.path.exists(handler_path):
        handler_code = f"""using System;
using Google.Protobuf;
using Protocol;
using UnityEngine;

public class PacketHandler
{{
{csharp_handler_content}
}}"""
        with open(handler_path, 'w', encoding='utf-8') as f:
            f.write(handler_code)
        print(f"Success: {handler_path} created.")
    else:
        print(f"Skip: {handler_path} already exists. (New handlers may need manual copy)")

    print(f"Success: C++ and C# codes are updated.")

if __name__ == "__main__":
    generate()