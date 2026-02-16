using Google.Protobuf;
using Protocol;
using System;
using System.Net.Sockets;
using UnityEditor.VersionControl;
using UnityEngine;
using UnityEngine.XR;


public class PacketSession
{
    private Socket _socket;
    private byte[] _recvBuffer = new byte[1024 * 64];
    private int _readPos = 0;
    private int _writePos = 0;


    public void Init(Socket socket) { _socket = socket; }

    public void StartReceive()
    {
        _socket.BeginReceive(_recvBuffer, 0, _recvBuffer.Length, SocketFlags.None, OnReceiveCallback, null);
    }

    // 서버의 RecvBuffer 로직과 동일하게 작동해야 함
    public void OnReceive(byte[] buffer, int bytesTransferred)
    {
        // 1. 버퍼에 데이터 복사 및 커서 이동
        Array.Copy(buffer, 0, _recvBuffer, _writePos, bytesTransferred);
        _writePos += bytesTransferred;

        while (true)
        {
            int dataSize = _writePos - _readPos;
            // 최소 헤더 크기(Size 2 + Id 2 = 4바이트) 체크
            if (dataSize < 4) break;

            ushort size = BitConverter.ToUInt16(_recvBuffer, _readPos);
            if (dataSize < size) break;

            // 2. 패킷 번호 추출 및 파싱
            ushort id = BitConverter.ToUInt16(_recvBuffer, _readPos + 2);

            // [중요] 여기서 PacketManager를 통해 실제 패킷 객체로 변환
            Managers.packetManager.OnRecvPacket(id, _recvBuffer, _readPos, size,this);

            _readPos += size;
        }

        // 3. 남은 찌꺼기 데이터 정리 (Clean)
        if (_readPos > 0)
        {
            int remaining = _writePos - _readPos;
            Array.Copy(_recvBuffer, _readPos, _recvBuffer, 0, remaining);
            _readPos = 0;
            _writePos = remaining;
        }
    }

    private void OnReceiveCallback(IAsyncResult ar)
    {
        try
        {
            int bytesRead = _socket.EndReceive(ar);
            if (bytesRead > 0)
            {
                // 여기서 패킷 조립(서버에서 했던 것과 동일한 로직) 수행
                // 조립 완료된 패킷은 NetworkManager.Instance.PushPacket(packet); 으로 전달

                StartReceive(); // 다시 수신 대기
            }
        }
        catch (Exception e) { /* 연결 끊김 처리 */ }
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

    }
}