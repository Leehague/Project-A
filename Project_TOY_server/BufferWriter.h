#pragma once
#include "Types.h"

class BufferWriter {
public:
    BufferWriter() {}
    BufferWriter(uint8* buffer, uint32 size, uint32 pos = 0)
        : _buffer(buffer), _size(size), _pos(pos) {
    }

    uint8* Buffer() { return _buffer; }
    uint32 Size() { return _size; }
    uint32 WritePos() { return _pos; }
    uint32 FreeSize() { return _size - _pos; }

    template<typename T>
    bool Write(T* src) { return Write(src, sizeof(T)); }

    bool Write(void* src, uint32 len) {
        if (FreeSize() < len) return false;
        ::memcpy(&_buffer[_pos], src, len);
        _pos += len;
        return true;
    }

    // 편의를 위한 연산자 오버로딩
    template<typename T>
    BufferWriter& operator<<(T dest) {
        Write(&dest, sizeof(T));
        return *this;
    }

private:
    uint8* _buffer = nullptr;
    uint32 _size = 0;
    uint32 _pos = 0;
};