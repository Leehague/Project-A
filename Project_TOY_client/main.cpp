#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Protocol.h"
#include "RecvBuffer.h"
#include "Types.h"
#include <winsock2.h>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

void HandlePacket(PacketHeader* header) {
    switch (header->type) {
    case PKT_SC_LOGIN_OK: {
        auto pkt = reinterpret_cast<PKT_SC_LOGIN_OK_DATA*>(header);
        std::cout << "Login Success! ID: " << pkt->playerGuid << std::endl;
        break;
    }
    case PKT_SC_CHAT_BROADCAST: {
        auto pkt = reinterpret_cast<PKT_SC_CHAT_BROADCAST_DATA*>(header);
        std::cout << "\n[Player " << pkt->playerId << "]: " << pkt->chatMsg << std::endl;
        std::cout << "Input Message > ";
        break;
    }
    }
}

void ReceiveThread(SOCKET clientSocket) {
    // 서버에서 썼던 그 RecvBuffer 클래스를 생성 (예: 4KB 크기)
    RecvBuffer recvBuffer(4096);

    while (true) {
        // 1. 수신할 수 있는 최대 크기 계산
        int freeSize = recvBuffer.FreeSize();
        if (freeSize <= 0) break; // 버퍼 꽉 참 (풀링 필요)

        // 2. 실제 데이터 수신
        int len = recv(clientSocket, recvBuffer.WritePos(), freeSize, 0);
        if (len <= 0) break;

        // 3. 수신 성공 시 write 위치 이동
        if (recvBuffer.OnWrite(len) == false) break;

        // 4. 패킷 조립 및 처리 (핵심 루프)
        while (true) {
            int dataSize = recvBuffer.DataSize();

            // 헤더(size, type)조차 읽을 수 없을 만큼 적게 왔다면 다음 recv 대기
            if (dataSize < sizeof(PacketHeader)) break;

            PacketHeader* header = reinterpret_cast<PacketHeader*>(recvBuffer.ReadPos());

            // 헤더에 적힌 패킷 전체 크기만큼 데이터가 아직 안 왔다면 다음 recv 대기
            if (dataSize < header->size) break;

            // [패킷 완성!] 이제 안전하게 처리
            HandlePacket(header);

            // 처리한 만큼 버퍼에서 제거
            recvBuffer.OnRead(header->size);
        }

        // 5. 버퍼가 너무 뒤로 밀렸다면 앞으로 당기기 (CleanUp)
        recvBuffer.Clean();
    }
}
int main()
{
    WSAData wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 로컬 접속
    serverAddr.sin_port = htons(7777);

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        std::cout << "Connect Error" << std::endl;
        return 0;
    }

    // [1] 로그인 패킷 전송
    PKT_CS_LOGIN_DATA loginPkt;
    loginPkt.header.size = sizeof(PKT_CS_LOGIN_DATA);
    loginPkt.header.type = PKT_CS_LOGIN;
    strcpy_s(loginPkt.userId, "MyUser");
    send(clientSocket, (char*)&loginPkt, sizeof(loginPkt), 0);

    // [2] 수신 전용 스레드 시작
    std::thread t(ReceiveThread, clientSocket);
    t.detach(); // 메인 스레드와 독립적으로 실행

    // [3] 메인 스레드: 채팅 입력 루프
    while (true) {
        std::string msg;
        std::cout << "Input Message > ";
        std::getline(std::cin, msg);

        if (msg == "exit") break;

        // 채팅 패킷 조립 및 전송
        PKT_CS_CHAT_DATA chatPkt;
        chatPkt.header.size = sizeof(chatPkt);
        chatPkt.header.type = PKT_CS_CHAT;
        strcpy_s(chatPkt.chatMsg, msg.c_str());

        send(clientSocket, (char*)&chatPkt, sizeof(chatPkt), 0);
    }

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}