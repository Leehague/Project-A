using System;

public class RecvBuffer
{
    ArraySegment<byte> _buffer;
    int _readPos;
    int _writePos;

    public RecvBuffer(int bufferSize)
    {
        _buffer = new ArraySegment<byte>(new byte[bufferSize], 0, bufferSize);
    }

    // 아직 읽지 않은 데이터의 크기 (조립 대기 중인 데이터)
    public int DataSize { get { return _writePos - _readPos; } }

    // 버퍼의 남은 빈 공간 (데이터를 더 채울 수 있는 크기)
    public int FreeSize { get { return _buffer.Count - _writePos; } }

    // 데이터의 시작 지점 (패킷 조립 시 여기부터 읽음)
    public ArraySegment<byte> ReadSegment
    {
        get { return new ArraySegment<byte>(_buffer.Array, _buffer.Offset + _readPos, DataSize); }
    }

    // 데이터를 채울 지점 (받은 데이터를 여기다 복사함)
    public ArraySegment<byte> WriteSegment
    {
        get { return new ArraySegment<byte>(_buffer.Array, _buffer.Offset + _writePos, FreeSize); }
    }

    // 데이터를 처리한 후 커서를 이동
    public bool OnRead(int numOfBytes)
    {
        if (numOfBytes > DataSize) return false;
        _readPos += numOfBytes;
        return true;
    }

    // 데이터를 받은 후 커서를 이동
    public bool OnWrite(int numOfBytes)
    {
        if (numOfBytes > FreeSize) return false;
        _writePos += numOfBytes;
        return true;
    }

    // 커서를 앞으로 당겨서 공간을 확보 (중요!)
    public void Clean()
    {
        int dataSize = DataSize;
        if (dataSize == 0)
        {
            // 남은 데이터가 없으면 그냥 커서를 처음으로 리셋
            _readPos = _writePos = 0;
        }
        else
        {
            // 남은 데이터가 있다면 맨 앞으로 복사해서 붙여넣음
            Array.Copy(_buffer.Array, _buffer.Offset + _readPos, _buffer.Array, _buffer.Offset, dataSize);
            _readPos = 0;
            _writePos = dataSize;
        }
    }
}