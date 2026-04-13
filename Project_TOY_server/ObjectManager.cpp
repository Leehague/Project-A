#include "ObjectManager.h"
#include "RoomManager.h"
#include "Player.h"

ObjcetManager GObjcetManager;

GameObjectPtr ObjcetManager::Create(GameObjectType type, std::shared_ptr<Session> session, int32 templateId)
{
    std::lock_guard<std::mutex> lock(_lock);
    int32 objcetId = ++GameobjcetCounter;

    GameObjectPtr go = nullptr;

    if (type == GameObjectType::Player) {
        PlayerPtr player = std::make_shared<Player>(objcetId, session, templateId);
        
        go = std::static_pointer_cast<GameObject>(player);
    }
    else 
    {
        go = std::make_shared<GameObject>(objcetId, type);
    }

    _objects[objcetId] = go;

    //Temp , 실제로는 DB등에서 알맞은 템플릿 ID (즉 만들고자 하는 GameObject 의 TempleteId 를 알아야 함)
    go->SetTempleteId_In_pos(1);
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
