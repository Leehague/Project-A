#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "Protocol.h"
#include <winsock2.h>
#include <iostream>
#include <string>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

// 서버로부터 패킷을 계속 수신하는 함수
void ReceiveThread(SOCKET clientSocket) {
    char recvBuffer[1024];

    while (true) {
        // 서버로부터 데이터 수신 (패킷이 올 때까지 대기)
        int len = recv(clientSocket, recvBuffer, 1024, 0);
        if (len <= 0) {
            std::cout << "Disconnected from Server." << std::endl;
            break;
        }

        // 수신된 데이터의 헤더 확인
        PacketHeader* header = reinterpret_cast<PacketHeader*>(recvBuffer);

        if (header->type == PKT_SC_CHAT_BROADCAST) {
            PKT_SC_CHAT_BROADCAST_DATA* pkt = reinterpret_cast<PKT_SC_CHAT_BROADCAST_DATA*>(recvBuffer);
            // 다른 사람이 보낸 채팅 출력
            std::cout << "\n[Player " << pkt->playerId << "]: " << pkt->chatMsg << std::endl;
            std::cout << "Input Message > "; // 입력 가이드 다시 표시
        }
        else if (header->type == PKT_SC_LOGIN_OK) {
            PKT_SC_LOGIN_OK_DATA* res = (PKT_SC_LOGIN_OK_DATA*)recvBuffer;
            std::cout << "Login Success! ID: " << res->playerGuid << std::endl;
        }
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