using Google.Protobuf;

public class PacketMessage
{
    public ushort Id { get; set; } //Protocol의 PacketId 를 가지도록 하고 있음
    public IMessage Message { get; set; }
}

