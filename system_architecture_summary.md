# Flow Graph Summary
- **Total Nodes**: 5
- **Total Edges**: 5

## Node List
| Node ID | Label |
|---|---|
| `Client` | Unity Client |
| `CodeGen` | Packet Generator |
| `DB` | Database |
| `ProtoFiles` | Protobuf Definition |
| `Server` | C++ IOCP Server |

## Flow Relations
| Source Node | Destination Node | Edge Label (Action) |
|---|---|---|
| `Client` (Unity Client) | `Server` (C++ IOCP Server) | NetworkManager (TCP/IP) |
| `Server` (C++ IOCP Server) | `DB` (Database) | Entity Framework / Stored Procedure |
| `ProtoFiles` (Protobuf Definition) | `CodeGen` (Packet Generator) | Input (.proto) |
| `CodeGen` (Packet Generator) | `Server` (C++ IOCP Server) | Generate PacketHandler.h |
| `CodeGen` (Packet Generator) | `Client` (Unity Client) | Generate PacketManager_Gen.cs |