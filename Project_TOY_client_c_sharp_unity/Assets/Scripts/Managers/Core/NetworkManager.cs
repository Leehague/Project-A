using Google.Protobuf;
using System;
using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using Unity.VisualScripting;
using UnityEngine;

public class NetworkManager 
{
    

    private Socket _socket;
    private PacketSession _session;
    private int _disconnected = 1; //0: connected , 1: disconnected
    private bool _isConnectSuccess = false;
    


    public bool enabled = true;
    public bool CanPushPacket = true;

    public bool IsSession_NULL() 
    { 
        if (_session == null) { return true; }
        return false;
    }

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

            _isConnectSuccess = true;

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
        if (CanPushPacket == false) return;
        _packetQueue.Enqueue(packet);
    }

    public void Update()
    {
        if (this.enabled == false) return;

        if (_isConnectSuccess && _session != null)
        {
            _isConnectSuccess = false;
            _session.OnConnected(_socket.RemoteEndPoint);
        }


        while (_packetQueue.TryDequeue(out PacketMessage packet))
        {
            Managers.packetManager.HandlePacket(_session, packet);
        }


    }
        
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

    public void Send(IMessage packet)
    {
        if (_session == null)
        {
            Debug.LogError("세션 객체가 없습니다");
            return;
        }
        _session.Send(packet);
    }
}