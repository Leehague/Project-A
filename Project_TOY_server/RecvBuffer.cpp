#include "RecvBuffer.h"
#include <algorithm>


RecvBuffer::RecvBuffer()
{
}

RecvBuffer::RecvBuffer(int bufferSize) : _capacity(bufferSize)
{
    //버퍼 크기를 지정 (보통 한 패킷 최대 크기의 몇 배)
    _buffer.resize(bufferSize);
}

RecvBuffer::~RecvBuffer()
{ 
}

//이미 읽어서 유효하지 않은 데이터 제거 
void RecvBuffer::Clean()
{
    int dataSize = DataSize();
    if (dataSize == 0)
    {
        // 남은 데이터가 없으면 커서만 초기화
        _readPos = _writePos = 0;
    }
    else
    {
        // 남은 데이터가 있다면 맨 앞으로 복사 (데이터 밀기)
        std::copy(_buffer.begin() + _readPos, _buffer.begin() + _writePos, _buffer.begin());
        _readPos = 0;
        _writePos = dataSize;
    }
}

bool RecvBuffer::OnWrite(int numOfBytes)
{
    if (numOfBytes > FreeSize())
        return false;

    _writePos += numOfBytes;
    return true;
}

bool RecvBuffer::OnRead(int numOfBytes)
{
    if (numOfBytes > DataSize())
        return false;

    _readPos += numOfBytes;
    return true;
}