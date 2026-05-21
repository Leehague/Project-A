#include "Player.h"
#include "Session.h"
#include "Room.h"


void Player::Init(int32 templateId)
{

    auto sessionPtr = session.lock();
    if (sessionPtr)
    {
        // 이제 shared_ptr로 관리되는 상태이므로 안전하게 호출 가능합니다.
        sessionPtr->SetPlayerPtr(std::static_pointer_cast<Player>(shared_from_this()));

    }
    this->InitStatData(templateId);
}
