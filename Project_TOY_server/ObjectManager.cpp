#include "ObjectManager.h"
#include "Player.h"
#include "Monster.h"
#include "Projectile.h"
#include "Item.h"

ObjcetManager GObjcetManager;

GameObjectPtr ObjcetManager::Create(GameObjectType type, std::shared_ptr<Session> session, int32 templateId, CoreRoomPtr coreroomptr)
{
    std::lock_guard<std::mutex> lock(_lock);
    int32 objcetId = ++GameobjcetCounter;

    GameObjectPtr go = nullptr;


    if (type == GameObjectType::Player) {
        PlayerPtr player = std::make_shared<Player>(objcetId, session, coreroomptr);
        player->Init(templateId);
        go = std::static_pointer_cast<GameObject>(player);
    }
    else if(type == GameObjectType::Monster)
    {
        MonsterPtr monster = std::make_shared<Monster>(objcetId, coreroomptr);
        monster->Init(templateId);
        go = std::static_pointer_cast<GameObject>(monster);
    }
    else if (type == GameObjectType::Projectile) 
    {
        ProjectilePtr projectile = std::make_shared<Projectile>(objcetId, coreroomptr);
        //Init 은 SpawnProjectile 에서 해주고 있음
        go = std::static_pointer_cast<GameObject>(projectile);
    }
    else if (type == GameObjectType::Item)
    {
        ItemPtr item = std::make_shared<Item>(objcetId, coreroomptr);
        item->Init(templateId);
        go = std::static_pointer_cast<GameObject>(item);
    }
    else 
    {
        go = std::make_shared<GameObject>(objcetId, type, coreroomptr);
        go->Init(templateId);
    }

    _objects[objcetId] = go;

    
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
