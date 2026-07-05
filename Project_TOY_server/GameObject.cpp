#include "GameObject.h"
#include "DataContents.h"
#include "DataManager.h"
#include "RoomManager.h"
#include <windows.h>

namespace TimeManager {
    bool g_useVirtualTime = false;
    uint64 g_virtualTick = 0;

    uint64 GetTickCount64() {
        if (g_useVirtualTime) return g_virtualTick;
        return ::GetTickCount64();
    }
}


Vector3  GameObject::Getpos_As_Vector3() { return Vector3::PosInfoToVector3(&_pos); }



void GameObject::Setpos(const Vector3& vecpos)
{
    Vector3 dir(vecpos.x - _pos.x, vecpos.y - _pos.y, vecpos.z - _pos.z);
    _pos.x=(vecpos.x);
    _pos.y=(vecpos.y);
    _pos.z=(vecpos.z);
    _pos.yaw=(Vector3::CalculateYaw(dir));
}

void GameObject::Setpos(const Core::PosInfo& pos)
{
    _pos.x = pos.x;
    _pos.y = pos.y;
    _pos.z = pos.z;
    _pos.yaw = pos.yaw;
    _pos.state = pos.state;
}



