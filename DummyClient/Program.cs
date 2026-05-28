using System;
using System.Net;
using System.Net.Sockets;
using Google.Protobuf; // NuGet 설치 필요
using Protocol;        // Protocol.cs 네임스페이스

namespace DummyClient
{
    class Program
    {
        static void Main(string[] args)
        {
            // 1. 서버 주소 설정 (로컬 테스트)
            string host = Dns.GetHostName();
            IPHostEntry ipHost = Dns.GetHostEntry(host);
            IPAddress ipAddr = IPAddress.Parse("127.0.0.1");
            IPEndPoint endPoint = new IPEndPoint(ipAddr, 7777); // 서버 포트와 맞출 것

            while (true)
            {
                try
                {
                    Socket socket = new Socket(endPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
                    socket.Connect(endPoint);
                    Console.WriteLine($"Connected To Server");

                    while (true)
                    {
                        // 2. 패킷 생성 (정의하신 userId, password 사용)
                        CS_LOGIN loginPkt = new CS_LOGIN();
                        loginPkt.UserId = 12;      // userId -> UserId
                        loginPkt.Password = "password123";  // password -> Password

                        // 3. 패킷 전송
                        byte[] sendBuffer = Open(loginPkt, 1);
                        socket.Send(sendBuffer);

                        Console.WriteLine($"Sent CS_LOGIN: {loginPkt.UserId}");
                        Thread.Sleep(1000);
                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.ToString());
                    Thread.Sleep(1000); // 재시도 대기
                }
            }
        }

        // 패킷 직렬화 및 헤더 부착 함수
        static byte[] Open(IMessage pkt, ushort pktId)
        {
            // 1. 데이터를 먼저 추출 (인자 없는 깔끔한 방식)
            byte[] dataBuffer = pkt.ToByteArray();
            ushort size = (ushort)dataBuffer.Length;
            ushort totalSize = (ushort)(size + 4);

            byte[] sendBuffer = new byte[totalSize];

            // 2. 헤더 채우기 (0~3 바이트)
            Array.Copy(BitConverter.GetBytes(totalSize), 0, sendBuffer, 0, 2);
            Array.Copy(BitConverter.GetBytes(pktId), 0, sendBuffer, 2, 2);

            // 3. 데이터 복사 (4바이트 지점부터 끝까지)
            Array.Copy(dataBuffer, 0, sendBuffer, 4, size);

            return sendBuffer;
        }
    }
}