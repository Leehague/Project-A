#pragma once
#include "ObjectManager.h"
#include "RoomManager.h"
#include "Player.h"
#include "Monster.h"
#include "Projectile.h"

ObjcetManager GObjcetManager;

GameObjectPtr ObjcetManager::Create(GameObjectType type, std::shared_ptr<Session> session, int32 templateId)
{
    std::lock_guard<std::mutex> lock(_lock);
    int32 objcetId = ++GameobjcetCounter;

    GameObjectPtr go = nullptr;

    

    if (type == GameObjectType::Player) {
        PlayerPtr player = std::make_shared<Player>(objcetId, session);
        player->Init(templateId);
        go = std::static_pointer_cast<GameObject>(player);
    }
    else if(type == GameObjectType::Monster)
    {
        MonsterPtr monster = std::make_shared<Monster>(objcetId);
        monster->Init(templateId);
        go = std::static_pointer_cast<GameObject>(monster);
    }
    else if (type == GameObjectType::Projectile) 
    {
        ProjectilePtr projectile = std::make_shared<Projectile>(objcetId);
        //Init 은 SpawnProjectile 에서 해주고 있음
        go = std::static_pointer_cast<GameObject>(projectile);
    }
    else 
    {
        go = std::make_shared<GameObject>(objcetId, type);
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
