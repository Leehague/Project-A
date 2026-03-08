using Google.Protobuf;
using Protocol;
using System;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using UnityEditor.VersionControl;
using UnityEngine;
using UnityEngine.XR;


public class PacketSession
{
    private Socket _socket;
    private RecvBuffer _recvBuffer= new RecvBuffer(1024 * 64);
    
    

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
        // [수정] 문자열 파싱 대신 PacketManager의 딕셔너리 이용
        ushort packetId = Managers.packetManager.GetId(packet.GetType());

        if (packetId == 0)
        {
            Debug.LogError($"Could not find PacketId for {packet.GetType().Name}");
            return;
        }

        byte[] sendBuffer = NetworkUtils.MakeSendBuffer(packet, packetId);

        string hex = BitConverter.ToString(sendBuffer).Replace("-", " ");
        Debug.Log($"[Raw Send Data] {hex}");

        try
        {
            _socket.BeginSend(sendBuffer, 0, sendBuffer.Length, SocketFlags.None, OnSendCallback, null);
        }
        catch (Exception e)
        {
            Debug.LogError($"Send Error: {e.Message}");
        }
    }

    private void OnSendCallback(IAsyncResult ar)
    {
        _socket.EndSend(ar);
    }


    public void Disconnect() 
    {
        
        try
        {
            //  송수신 차단 및 소켓 닫기
            // Shutdown을 먼저 호출하여 상대방에게 종료 신호(FIN)를 보냅니다.
            _socket.Shutdown(SocketShutdown.Both);
            _socket.Close();
        }
        catch (Exception e)
        {
            Debug.Log($"Disconnect Error: {e}");
        }

        // 3. 엔진/매니저 로직 정리
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

        Send(loginPacket);
    }
}