using Google.Protobuf;
using Protocol; // Protobuf로 생성된 네임스페이스
using System;
using System.Collections.Generic;
using Unity.VisualScripting;
using UnityEngine;
using UnityEngine.XR;

public partial class PacketManager
{
    //// 싱글톤
    //private static PacketManager _instance = new PacketManager();
    //public static PacketManager Instance => _instance;

    // 패킷 ID에 따른 파싱 함수를 담는 딕셔너리
    Dictionary<ushort, Action<PacketSession, byte[], ushort>> _onRecv = new Dictionary<ushort, Action<PacketSession, byte[], ushort>>();

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
            action.Invoke(session, buffer, size);
        }
    }

    // 제네릭을 이용한 패킷 생성 헬퍼 함수
    void MakePacket<T>(PacketSession session, byte[] buffer, ushort size, ushort id ) where T : IMessage, new()
    {
        T pkt = new T();
        // 헤더 4바이트(Size 2, Id 2)를 제외한 나머지를 파싱
        pkt.MergeFrom(buffer, 4, size - 4);

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