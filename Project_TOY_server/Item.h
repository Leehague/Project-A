#pragma once
#include "GameObject.h"
#include "Types.h"

class Item : public GameObject
{
public:
    Item(int32 objectId) : GameObject(objectId, GameObjectType::Item)
    {

    };
    virtual ~Item();

    void Init(GameObjectPtr attacker, const SkillData* skillData, Vector3 targetPos);
    
    
private:
    
    
    //아이템에서 HP는 일종의 내구도 개념으로 이해 하면 될것
};
