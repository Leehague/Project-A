using Google.Protobuf;
using Protocol;
using System;
using System.Linq;
using System.Net.Sockets;
using System.Threading;
using UnityEditor.VersionControl;
using UnityEngine;
using UnityEngine.XR;


public class PacketSession
{
    private Socket _socket;
    //private byte[] _recvBuffer = new byte[1024 * 64];
    private RecvBuffer _recvBuffer= new RecvBuffer(1024 * 64);
    private int _readPos = 0;
    private int _writePos = 0;
    

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
        // 패킷의 이름을 통해 ID를 찾는 로직 (간단한 예시)
        string name = packet.Descriptor.Name.Replace("_", "");
        // Enum 이름을 활용하거나 직접 넘겨받도록 구현
        PacketId packetId = (PacketId)Enum.Parse(typeof(PacketId), "Pkt" + name);

        byte[] sendBuffer = NetworkUtils.MakeSendBuffer(packet, (ushort)packetId);

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
}