using System;
using Google.Protobuf;
using UnityEngine;

public class NetworkUtils
{
    public static byte[] MakeSendBuffer(IMessage packet, ushort packetId)
    {
        byte[] dataBuffer = packet.ToByteArray();
        ushort dataSize = (ushort)dataBuffer.Length;
        ushort totalSize = (ushort)(dataSize + 4); // 헤더(4) + 데이터

        // [디버깅 로그]
        Debug.Log($"[Send] ID: {packetId}, DataSize: {dataSize}, TotalSize: {totalSize}");


        byte[] sendBuffer = new byte[totalSize];

        // 헤더 채우기 (Size 2, ID 2)
        Array.Copy(BitConverter.GetBytes(totalSize), 0, sendBuffer, 0, 2);
        Array.Copy(BitConverter.GetBytes(packetId), 0, sendBuffer, 2, 2);

        // 데이터 복사 (Offset 4)
        Array.Copy(dataBuffer, 0, sendBuffer, 4, dataSize);

        return sendBuffer;
    }
}