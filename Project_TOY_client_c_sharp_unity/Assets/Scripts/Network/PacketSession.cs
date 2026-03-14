using Google.Protobuf;
using Protocol;
using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using UnityEngine;



public class PacketSession
{
    private Socket _socket;
    private RecvBuffer _recvBuffer= new RecvBuffer(1024 * 64);
    
    readonly System.Object _lock = new System.Object();
    Queue<ArraySegment<byte>> _sendQueue = new Queue<ArraySegment<byte>>();
    List<ArraySegment<byte>> _pendingList = new List<ArraySegment<byte>>(); // 현재 전송 중인 목록
    bool _pending = false;


    public void Init(Socket socket) { _socket = socket; }

    public void StartReceive()
    {
        ArraySegment<byte> segment = _recvBuffer.WriteSegment;
        _socket.BeginReceive(
            segment.Array,
            segment.Offset,
            segment.Count,
            SocketFlags.None,
            OnReceiveCallback,
            null
        );
    }

    // 서버의 RecvBuffer 로직과 동일하게 작동해야 함
    public int OnReceive(ArraySegment<byte> buffer)
    {
        int processLen = 0;

        while (true)
        {
            int dataSize = buffer.Count - processLen;
            if (dataSize < 4) break;

            // buffer.Offset + processLen 위치부터 읽어야 함
            ushort size = BitConverter.ToUInt16(buffer.Array, buffer.Offset + processLen);
            if (dataSize < size) break;

            ushort id = BitConverter.ToUInt16(buffer.Array, buffer.Offset + processLen + 2);

            // 패킷 매니저에게 조립된 패킷 전달
            Managers.packetManager.OnRecvPacket(id, buffer.Array, buffer.Offset + processLen, size, this);

            processLen += size;
        }

        return processLen;
    }

    private void OnReceiveCallback(IAsyncResult ar)
    {
        try
        {
            int bytesRead = _socket.EndReceive(ar);
            if (bytesRead > 0)
            {

                // 쓰기 커서 이동 (데이터를 받았으므로)
                if (_recvBuffer.OnWrite(bytesRead) == false)
                {
                    Disconnect();
                    return;
                }

                // 2. 패킷 조립 시도 (OnRecv 호출)
                int processLen = OnReceive(_recvBuffer.ReadSegment);

                // 읽기 커서 이동 (패킷 파싱 완료)
                if (_recvBuffer.OnRead(processLen) == false)
                {
                    Disconnect();
                    return;
                }

                _recvBuffer.Clean();

                StartReceive(); // 다시 수신 대기
            }
            else 
            {
                Debug.Log("bytesRead<=0");
                Disconnect();
            }
        }
        catch (Exception e)
        {
            Debug.LogError($"Receive Error: {e.Message}");
            Disconnect();
        }
    }

    public void Send(IMessage packet)
    {


        ushort packetId = Managers.packetManager.GetId(packet.GetType());

        if (packetId == 0)
        {
            Debug.LogError($"Could not find PacketId for {packet.GetType().Name}");
            return;
        }

        byte[] sendBuffer = NetworkUtils.MakeSendBuffer(packet, packetId);




        // 큐 기반 Send 호출
        SendInternal(new ArraySegment<byte>(sendBuffer));
        
    }

    private void SendInternal(ArraySegment<byte> sendBuffer)
    {
        if (_socket == null) return;
        lock (_lock)
        {
            _sendQueue.Enqueue(sendBuffer);

            // 전송 중이 아니라면 전송 프로세스 시작
            if (_pending == false)
                RegisterSend();
        }
    }

    private void RegisterSend()
    {
        _pending = true;
        // 큐에 있는 모든 데이터를 하나의 리스트로 이동
        _pendingList.Clear();
        while (_sendQueue.Count > 0)
            _pendingList.Add(_sendQueue.Dequeue());

        try
        {
            // Socket.BeginSend는 BufferList를 지원하여 여러 세그먼트를 한 번에 보낼 수 있습니다.
            _socket.BeginSend(_pendingList, SocketFlags.None, OnSendCallback, null);
        }
        catch (Exception e)
        {
            Debug.LogError($"RegisterSend Error: {e.Message}");
            Disconnect();
        }
    }
        

    private void OnSendCallback(IAsyncResult ar)
    {
        try
        {
            int bytesTransferred = _socket.EndSend(ar);

            lock (_lock)
            {
                if (_sendQueue.Count > 0)
                {
                    // 아직 보낼 게 더 있다면 다시 등록
                    RegisterSend();
                }
                else
                {
                    _pending = false;
                }
            }
        }
        catch (Exception e)
        {
            Debug.LogError($"OnSendCallback Error: {e.Message}");
            Disconnect();
        }
    }


    public void Disconnect() 
    {

        lock (_lock)
        {
            if (_socket == null) return;

            try
            {
                _socket.Shutdown(SocketShutdown.Both);
                _socket.Close();
                _socket = null; // Dispose된 객체 접근 방지
            }
            catch (Exception e)
            {
                Debug.Log($"Disconnect Error: {e}");
            }
        }
        OnDisconnected();
    }

    private void OnDisconnected()
    {

    }

    // 연결 성공 시 호출될 함수
    public virtual void OnConnected(EndPoint endPoint)
    {
        Debug.Log($"OnConnected : {endPoint}");
             
        CS_LOGIN loginPacket = new CS_LOGIN();
        //TEMP, TOOD: 실제로는 user id와 password등의 인증정보에 관한 로직이 들어가야함
        loginPacket.UserId = 123123;
        loginPacket.Password = "password 1234";

        Send(loginPacket);
    }
}