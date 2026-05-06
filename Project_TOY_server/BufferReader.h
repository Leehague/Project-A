#pragma once
#include "Types.h"

class BufferReader {
public:
    BufferReader() {}
    BufferReader(uint8* buffer, uint32 size, uint32 pos = 0)
        : _buffer(buffer), _size(size), _pos(pos) {
    }

    uint8* Buffer() { return _buffer; }
    uint32 Size() { return _size; }
    uint32 ReadPos() { return _pos; }
    uint32 FreeSize() { return _size - _pos; }

    template<typename T>
    bool Read(T* dest) { return Read(dest, sizeof(T)); }

    template<typename T>
    bool Read(T* dest, uint32 len) {
        if (FreeSize() < len) return false;
        ::memcpy(dest, &_buffer[_pos], len);

        // 방어 코드: 읽어온 값이 실수인데 유효하지 않다면
        if constexpr (std::is_floating_point_v<T>) {
            if (!std::isfinite(*dest)) {
                _pos += len;
                *dest = 0.0f; // 안전한 값으로 강제 치환하거나 false 반환
                return false;
            }
        }


        _pos += len;
        return true;
    }

    template<typename T>
    BufferReader& operator>>(T& dest) {
        Read(&dest, sizeof(T));
        return *this;
    }

private:
    uint8* _buffer = nullptr;
    uint32 _size = 0;
    uint32 _pos = 0;
};