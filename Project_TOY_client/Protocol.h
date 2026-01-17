#pragma pack(push, 1) // 1바이트 정렬 (메모리 패딩 제거, 네트워크 전송 시 필수)

struct PacketHeader {
    unsigned short size;
    unsigned short type;
};

// 패킷 타입 정의
enum PacketType : unsigned short {
    PKT_CS_LOGIN = 1,
    PKT_SC_LOGIN_OK = 2,
    PKT_CS_CHAT = 3,
    PKT_SC_CHAT_BROADCAST = 4
};

// 클라이언트 -> 서버 로그인 요청
struct PKT_CS_LOGIN_DATA {
    PacketHeader header;
    char userId[32];
};

// 서버 -> 클라이언트로 보내는 로그인 결과 패킷
struct PKT_SC_LOGIN_OK_DATA {
    PacketHeader header; // [2 + 2 = 4 bytes]

    bool success;        // 성공 여부 [1 byte]
    int playerGuid;      // 유저 고유 ID [4 bytes]
    // 필요하다면 캐릭터 레벨, 닉네임 등을 추가
};

// 클라이언트가 서버로 보내는 채팅 패킷
struct PKT_CS_CHAT_DATA {
    PacketHeader header;
    char chatMsg[100];
};

// 서버가 모든 클라이언트에게 뿌리는 패킷
struct PKT_SC_CHAT_BROADCAST_DATA {
    PacketHeader header;
    int playerId;      // 누가 보냈는지 구분용
    char chatMsg[100];
};

#pragma pack(pop)