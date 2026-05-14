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

   

    public int OnReceive(ArraySegment<byte> buffer)
    {
        int processLen = 0;

        while (true)
        {
            int dataSize = buffer.Count - processLen;
            if (dataSize < 4) break;

            ushort size = BitConverter.ToUInt16(buffer.Array, buffer.Offset + processLen);
            if (size < 4 || size > 1024 * 10) // 10KB 초과 시 차단
            {
                // 이런 경우가 발생한다면 서버의 데이터가 오염되었거나 헤더 설계가 잘못된 것임
                Debug.LogError($"Invalid Packet Size: {size}");
                Disconnect(); // 스트림 오염으로 간주하고 연결 종료
                return 0;
                
            }
            if (dataSize < size) break;
            
            ushort id = BitConverter.ToUInt16(buffer.Array, buffer.Offset + processLen + 2);

            

            //[검증 로그] 패킷 파싱 시도 직전의 Raw 데이터를 출력
            //string hex = BitConverter.ToString(buffer.Array, buffer.Offset + processLen, size);
            //Debug.Log($"[Recv Dump] ID: {id}, TotalSize: {size}, Hex: {hex}");

            // [점검용 로그] 모든 패킷의 헤더 정보를 찍습니다.
            //Debug.Log($"[Recv Check] ID: {id}, Size: {size}, CurrentProcessLen: {processLen}, BufferTotal: {buffer.Count}");
            if (size < 4 || size > 1024)
            {
                // 여기서 브레이크 포인트를 걸고 Hex 데이터를 확인하세요.
                Debug.LogError($"[Broken Packet] 위치: {processLen}, 읽은 Size: {size}");
            }
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

                // [로그 추가] 현재 수신된 총 바이트와 파싱 전 상태
                int beforeDataSize = _recvBuffer.DataSize;

                // 2. 패킷 조립 시도 (OnRecv 호출)
                int processLen = OnReceive(_recvBuffer.ReadSegment);

                // 읽기 커서 이동 (패킷 파싱 완료)
                if (_recvBuffer.OnRead(processLen) == false)
                {
                    Disconnect();
                    return;
                }

                // [로그 추가] 몬스터 스폰 시 패킷이 짤렸는지 확인
                if (processLen < beforeDataSize)
                {
                    Debug.Log($"[PacketSession] Partial Packet: Processed={processLen}, Remaining={beforeDataSize - processLen}");
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

        loginPacket.UserId = 123123;
        loginPacket.Password = "password 1234";

        Managers.networkManager.Send(loginPacket);

        //로그인 씬 로드
        Managers.sceneManagerEx.LoadScene(Define.SceneType.Login);


    }
}