using Google.Protobuf;
using Protocol; // Protobuf로 생성된 네임스페이스
using System;
using System.Collections.Generic;

using UnityEngine;



public partial class PacketManager
{
    

    // [추가] Type을 키로 하여 PacketId(ushort)를 저장
    Dictionary<Type, ushort> _typeToId = new Dictionary<Type, ushort>();

    // [추가] 외부에서 ID를 조회할 수 있는 함수
    public ushort GetId(Type type)
    {
        if (_typeToId.TryGetValue(type, out ushort id))
            return id;
        return 0;
    }


    // 패킷 ID에 따른 파싱 함수를 담는 딕셔너리
    Dictionary<ushort, Action<PacketSession, byte[],int, ushort>> _onRecv = new Dictionary<ushort, Action<PacketSession, byte[],int, ushort>>();

    // 유니티 메인 스레드에서 실행될 핸들러 함수들
    Dictionary<ushort, Action<PacketSession, IMessage>> _handler = new Dictionary<ushort, Action<PacketSession, IMessage>>();

    public PacketManager()
    {
        Register(); //Partial class에서 정의 (PacketManager_Gen.cs 에 정의 되어 있음)
    }
      
    // 바이트 배열을 실제 Protobuf 객체로 변환
    public void OnRecvPacket(ushort id, byte[] buffer, int offset, ushort size, PacketSession session)
    {
        if (_onRecv.TryGetValue(id, out var action))
        {
            action.Invoke(session, buffer, offset, size);
        }
    }

    // 제네릭을 이용한 패킷 생성 헬퍼 함수
    void MakePacket<T>(PacketSession session, byte[] buffer, int offset, ushort size, ushort id ) where T : IMessage, new()
    {
        T pkt = new T();
        int dataSize = size - 4;
        int newoffset = offset +4;
        
        try
        {
            pkt.MergeFrom(buffer, newoffset, dataSize);
        }
        catch (Exception ex)
        {
            // 에러 발생 시 버퍼의 상태를 스냅샷으로 찍음
            Debug.LogError($"[Packet Error] ID: {id}, Size: {size}");
            Debug.LogError(ex.Message);
            //Debug.LogError($"Current Buffer State: ReadPos={buffer.ReadPos}, WritePos={buffer.WritePos}");

        }
        
        
        // 로직 처리를 위해 NetworkManager 큐에 삽입
        Managers.networkManager.PushPacket(new PacketMessage { Id = id, Message = pkt});
    }

    public Action<PacketSession, IMessage> GetHandler(ushort id)
    {
        if (_handler.TryGetValue(id, out var action))
            return action;
        return null;
    }

    public void HandlePacket(PacketSession session, PacketMessage packet)
    {
        ushort id = packet.Id;

        if (_handler.TryGetValue(id, out Action<PacketSession, IMessage> action))
        {
            action(session, packet.Message);
                     
        }
        else
        {
            // 핸들러가 등록되지 않은 경우 로그 출력
            Debug.LogWarning($"No handler registered for Packet ID: {id}");
        }
    }

    
}