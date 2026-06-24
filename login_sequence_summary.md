# Flow Graph Summary
- **Total Nodes**: 3
- **Total Edges**: 7

## Node List
| Node ID | Label |
|---|---|
| `Client` | Unity Client |
| `OtherClients` | Other Unity Clients |
| `Server` | C++ IOCP Server |

## Flow Relations
| Source Node | Destination Node | Edge Label (Action) |
|---|---|---|
| `Client` (Unity Client) | `Server` (C++ IOCP Server) | CS_LOGIN (networkmanger::update) |
| `Server` (C++ IOCP Server) | `Client` (Unity Client) | SC_LOGIN_OK (PacketHandler) |
| `Client` (Unity Client) | `Server` (C++ IOCP Server) | CS_ENTER_GAME (GameScene::Init) |
| `Server` (C++ IOCP Server) | `Client` (Unity Client) | SC_ENTER_GAME (Room::Enter) |
| `Client` (Unity Client) | `Server` (C++ IOCP Server) | CS_GAME_READY |
| `Server` (C++ IOCP Server) | `Client` (Unity Client) | SC_PLAYER_SPAWN (Spawn Self / Monster Spawn) |
| `Server` (C++ IOCP Server) | `OtherClients` (Other Unity Clients) | SC_PLAYER_SPAWN (Spawn Broadcast) |