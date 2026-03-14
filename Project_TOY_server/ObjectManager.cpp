#include "ObjectManager.h"
#include "RoomManager.h"
#include "Player.h"

ObjcetManager GObjcetManager;

GameObjectPtr ObjcetManager::Create(int32 RoomId, GameObjectType type, std::shared_ptr<Session> session)
{
    std::lock_guard<std::mutex> lock(_lock);
    int32 objcetId = GameobjcetCounter++;

    GameObjectPtr go = nullptr;

    if (type == GameObjectType::Player) {
        PlayerPtr player = std::make_shared<Player>(objcetId, RoomId, session);
        // Cast PlayerPtr to GameObjectPtr for assignment
        player->Init();
        go = std::static_pointer_cast<GameObject>(player);
    }
    else 
    {
        go = std::make_shared<GameObject>(objcetId, RoomId, type);
    }

    _objects[objcetId] = go;
    RoomPtr room = GRoomManager.FindRoom(RoomId);
    room->Enter(go);
    return go;
}

GameObjectPtr ObjcetManager::Find(int32 objectId)
{
    return _objects[objectId];
}

void ObjcetManager::Removeobjcet(int32 objectId)
{
    _objects.erase(objectId);
}

int32 ObjcetManager::GetobjcetCounter()
{
    return GameobjcetCounter;
}
