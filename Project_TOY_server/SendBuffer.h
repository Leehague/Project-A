#pragma once
#include <vector>
#include <memory>

class SendBuffer
{
public:
    SendBuffer(int bufferSize);
    ~SendBuffer();

    // 데이터를 버퍼에 기록
    void Write(void* data, int size);

    // WSASend에 넘겨줄 버퍼 시작 주소 및 크기
    char* Buffer() { return _buffer.data(); }
    int Size() { return _writeSize; }

    // (선택) 보낼 패킷 데이터 구조체를 직접 넘기는 편의 함수
    template<typename T>
    void CopyData(T& data) { Write(&data, sizeof(T)); }

private:
    std::vector<char> _buffer;
    int _writeSize = 0;
    int _capacity = 0;
};

// 스마트 포인터 별칭 정의
using SendBufferRef = std::shared_ptr<SendBuffer>;