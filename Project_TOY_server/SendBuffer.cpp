#include "SendBuffer.h"
#include <iostream>

SendBuffer::SendBuffer(int bufferSize) : _capacity(bufferSize)
{
    _buffer.resize(bufferSize);
}

SendBuffer::~SendBuffer()
{
}


void SendBuffer::Write(void* data, int size)
{
    if (size > _capacity - _writeSize)
    {
        //예외처리 , 버퍼크기 조정
        return;
    }
    /*if (isClosed) 
    {
        std::cout << "Already Closed SendBuffer" << std::endl;
        return;
    }*/
    // 데이터를 현재 쓰기 위치에 복사
    ::memcpy(&_buffer[_writeSize], data, size);
    _writeSize += size;
}