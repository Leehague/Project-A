using Google.Protobuf;
using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using UnityEditor.Sprites;
using UnityEditor.VersionControl;
using UnityEngine;

public class NetworkManager 
{
    
    //public static NetworkManager Instance { get; private set; }

    private Socket _socket;
    private PacketSession _session;
    private int _disconnected = 0; //0: connected , 1: disconnected

    // 서버에서 받은 패킷을 메인 스레드에서 처리하기 위한 큐
    private ConcurrentQueue<PacketMessage> _packetQueue = new ConcurrentQueue<PacketMessage>();

    public void Init()
    {
        // 서버 연결 시작 (예시 IP: 127.0.0.1, Port: 7777)
        Connect("127.0.0.1", 7777);
    }

    public void Connect(string ip, int port)
    {
        //플래그 초기화
        Interlocked.Exchange(ref _disconnected, 0);

        IPAddress ipAddr = IPAddress.Parse(ip);
        IPEndPoint endPoint = new IPEndPoint(ipAddr, port);

        _socket = new Socket(endPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

        // 비동기 연결 시도
        _socket.BeginConnect(endPoint, OnConnectCallback, _socket);
    }

    private void OnConnectCallback(IAsyncResult ar)
    {
        try
        {
            Socket socket = (Socket)ar.AsyncState;
            socket.EndConnect(ar);
            Debug.Log($"Connected to server: {socket.RemoteEndPoint}");

            _session = new PacketSession();
            _session.Init(socket);

            // 수신 시작
            _session.StartReceive();
        }
        catch (Exception e)
        {
            Debug.LogError($"Connect Fail: {e.Message}");
        }
    }

    // 패킷 수신 스레드에서 패킷을 큐에 넣을 때 사용
    public void PushPacket(PacketMessage packet)
    {
        _packetQueue.Enqueue(packet);
    }

    public void Update()
    {
        // [중요] 메인 스레드(Update)에서 쌓인 패킷을 하나씩 꺼내서 처리
        while (_packetQueue.TryDequeue(out PacketMessage packet))
        {
            Managers.packetManager.HandlePacket(_session, packet);
        }
    }

    //public void Send(IMessage packet)
    //{
    //    _session?.Send(packet);
    //}
        
    public void Close() 
    {
        // 1. 중복 호출 방지
        if (Interlocked.Exchange(ref _disconnected, 1) == 1)
            return;

        if (_session != null)
        {
            _session.Disconnect(); // 소켓의 Close나 Shutdown 호출
            _session = null;
        }
    }
}