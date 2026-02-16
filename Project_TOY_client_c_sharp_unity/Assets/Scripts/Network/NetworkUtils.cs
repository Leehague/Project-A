using System;
using Google.Protobuf;

public class NetworkUtils
{
    public static byte[] MakeSendBuffer(IMessage packet, ushort packetId)
    {
        // 1. 패킷 본체 직렬화
        byte[] payload = packet.ToByteArray();
        ushort size = (ushort)(payload.Length + 4); // 헤더(4) + 본체

        // 2. 전체 버퍼 생성
        byte[] sendBuffer = new byte[size];

        // 3. 헤더 채우기 (Little Endian 기준)
        Array.Copy(BitConverter.GetBytes(size), 0, sendBuffer, 0, 2);
        Array.Copy(BitConverter.GetBytes(packetId), 0, sendBuffer, 2, 2);

        // 4. 본체 채우기
        Array.Copy(payload, 0, sendBuffer, 4, payload.Length);

        return sendBuffer;
    }
}