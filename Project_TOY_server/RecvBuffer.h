#pragma once
#include <vector>


class RecvBuffer
{
public:
    RecvBuffer();
    RecvBuffer(int bufferSize);
    ~RecvBuffer();

    // 데이터 정리 (남은 데이터를 맨 앞으로 복사)
    void Clean();

    // 데이터 기록 성공 시 쓰기 커서 이동
    bool OnWrite(int numOfBytes);

    // 데이터 처리 완료 시 읽기 커서 이동
    bool OnRead(int numOfBytes);

    // 쓰기 위치 주소 (WSARecv에 넘겨줄 버퍼 시작점)
    //char* WritePos() { return &_buffer[_writePos]; }
    char* WritePos() { return _buffer.data() + _writePos; } // 인덱싱[] 대신 포인터 산술 연산 사용
    
    // 읽기 위치 주소 (패킷 해석을 시작할 지점)
    char* ReadPos() { return &_buffer[_readPos]; }

    // 현재 버퍼에 쌓여있는 데이터 크기
    int DataSize() { return _writePos - _readPos; }
    // 남은 빈 공간 크기
    int FreeSize() { return _capacity - _writePos; }

private:
    int             _capacity = 0;
    int             _readPos = 0;
    int             _writePos = 0;
    std::vector<char> _buffer;
};