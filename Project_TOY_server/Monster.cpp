#include "Monster.h"
#include "RoomManager.h"
#include "Map.h"
#include "Player.h"
#include "Room.h"


std::vector<float> Monster::GatherContext()
{
    std::vector<float> context;

    // 1. 본인 상태 (정규화 권장)
    context.push_back(static_cast<float>(GetCurrentHp()) / GetMaxHP());
    context.push_back(static_cast<float>(GetCurrentMp()) / GetMaxMP());

    // 2. 가장 가까운 타겟(플레이어) 정보
    PlayerPtr target =this->Getroomptr()->GetNearestPlayer(this->Getpos_As_Vector3(), this->_targetrange); // 시야(그리드) 내에서 탐색
    if (target)
    {
        Vector3 myPos = Vector3::PosInfoToVector3(this->Getpos());
        Vector3 targetPos = Vector3::PosInfoToVector3(target->Getpos());

        // 상대 거리 및 방향
        context.push_back(targetPos.x - myPos.x);
        context.push_back(targetPos.z - myPos.z);
        context.push_back(static_cast<float>(target->GetCurrentHp()) / target->GetMaxHP());

    }
    else
    {
        // 타겟이 없을 때의 패딩값
        context.insert(context.end(), { 0.0f, 0.0f, 0.0f });
    }

    return context;
}

void Monster::ExecuteHighLevelAction(int actionId, Vector3 targetPos)
{
    //TODO: actionId - action 맵핑을 해야됨 (강화학습 MDP 설정)
    switch (actionId)
    {
    case 0: // MoveTo
        this->MoveTo(targetPos);
        break;
    case 1: // Basic Attack
        this->UseSkill(nullptr,Vector3(0,0,0),1); // 일반 공격 스킬 ID
        break;
    case 2: // Flee
        //this->FleeFrom(targetPos);
        break;
    }
}

void Monster::MoveTo(Vector3 targetPos)
{
    // A* 알고리즘 연동
    // 1. Map::FindPath(myPos, targetPos) 호출하여 경로 리스트 획득
    // 2. 경로의 첫 번째 지점으로 이동 시작
    // 3. CreatureState를 Moving으로 변경 및 주변에 SC_MOVE 브로드캐스트

    RoomPtr myroom = this->Getroomptr();
    if (myroom == nullptr) return;

    auto mapptr = myroom->GetMapptr();
    if (mapptr == nullptr) return;

    if (Getpos_As_Vector3() == targetPos) return;

    std::vector<Vector3> newPath = myroom->GetMapptr()->FindPath(this->Getpos_As_Vector3(), targetPos);
    
    if (!newPath.empty())
    {
        _path = std::move(newPath); 
    }


    for (Vector3 vec : _path) 
    {
        if (std::isnan(vec.x) || std::isnan(vec.y) || std::isnan(vec.z))
        { 
            std::cout << "잘못된 path 입니다." << std::endl;
            return; 
        }
    }

    _pos.set_state((int32)CreatureState::Moving);
    
}

void Monster::JobUpdate()
{
    uint64 now = ::GetTickCount64();
    if (now < _nextDecisionTick)
        return; // 아직 주기가 안 되었으면 스킵
    
    if (GetState() == CreatureState::OnDead || GetState() == CreatureState::Dead)
    {
        return; // 방금 죽은 상태 (OnDead) 혹은 죽어 있는 상태 (Dead) 이면 스킵
    }

    _nextDecisionTick = now + DECISION_INTERVAL_MS;

    // 1. AI 판단 (목적지나 타겟 설정)
    // 내부에서 FindPath 등을 호출하여 _path를 채움
    UpdateAction();

    // 2. 실제 이동 처리 (검문소)
    // 여기서 이전에 작성한 isfinite 검사 로직이 돌아감
    ProcessMove();
}

void Monster::UpdateAction()
{
    // [판단] GatherContext를 통해 주변 상황 파악
    std::vector<float> context = GatherContext();

    // 가장 가까운 플레이어 탐색
    PlayerPtr target = Getroomptr()->GetNearestPlayer(Getpos_As_Vector3(), _targetrange);

    if (target)
    {

        // 타겟이 있으면 따라감 (ActionId 0: MoveTo)
        ExecuteHighLevelAction(0, target->Getpos_As_Vector3());
    }
    else
    {
        // 타겟이 없으면 멈춤
        if (GetState() == CreatureState::Moving)
        {
            SetState(CreatureState::Idle);
            _path.clear();
        }
    }
}


void Monster::ProcessMove()
{
    if (_path.empty()) {
        SetState(CreatureState::Idle);
        return;
    }

    Vector3 currentPos = Getpos_As_Vector3();
    Vector3 nextPos = _path.front();
    Vector3 dir = nextPos - currentPos;
    float dist = dir.Length();

    // 1. 도달 판정 (거리가 이동 속도보다 작거나 매우 가까우면 즉시 도달 처리)
    float moveDist = GetSpeed() * 1.0f; //1000ms 기준 이동 거리 DECISION_INTERVAL_MS = 1000 = 1s =1000ms

    if (std::isnan(currentPos.x) || std::isnan(currentPos.y) || std::isnan(currentPos.z))
    {
        std::cout << "ProcessMove ::(currentPos) 잘못된 포지션 요청" << std::endl;
        return;
    }

    if (std::isnan(nextPos.x) || std::isnan(nextPos.y) || std::isnan(nextPos.z))
    {
        std::cout << "ProcessMove ::(nextPos) 잘못된 포지션 요청" << std::endl;
        _path.erase(_path.begin()); // 터진 좌표는 제거
        return; 
    }

    Vector3 Normlizeddir = dir * (1.0f / dist);
    if (dist <= moveDist || dist < 0.01f)
    {
        // 목적지에 정확히 안착 (오차 누적 방지)
        _pos.set_x(nextPos.x);
        _pos.set_y(nextPos.y);
        _pos.set_z(nextPos.z);

        _path.erase(_path.begin());
        if (_path.empty()) SetState(CreatureState::Idle);

        
        float yaw = CalculateYaw(Normlizeddir);
            // Yaw 값 범위 강제 제한 (0~360)
            if (std::isnan(yaw)) 
            { 
                //std::cout << "yaw isnan" << std::endl;
                yaw = 0.0f;
            }

            //std::cout << "yaw: "<<yaw << std::endl;
            _pos.set_yaw(yaw);
         
        // [중요] 위치가 변했으므로 무조건 알림
        SyncPosAndBroadcast(currentPos, nextPos);
        return;
    }

    // 2. 방향 벡터 정규화 (NaN 방지)
    if (dist < 0.001f) {
        _path.erase(_path.begin());
        std::cout << "Wrong dist" << std::endl;
        return;
    }
    
    //FindPath에서 계산된 next pos 이 멀어서 도착을 한번에 못하면 이 로직으로 오게됨
    
    Vector3 newPos = currentPos + (Normlizeddir * moveDist);

    if (std::isfinite(newPos.x) && std::isfinite(newPos.y) && std::isfinite(newPos.z))
    {
        _pos.set_x(newPos.x);
        _pos.set_y(newPos.y);
        _pos.set_z(newPos.z);

        float yaw = CalculateYaw(Normlizeddir);
        // Yaw 값 범위 강제 제한 (0~360)
        if (std::isnan(yaw)) 
        { 
            std::cout << "yaw isnan" << std::endl;
            yaw = 0.0f;
        }

        //std::cout << "yaw: "<<yaw << std::endl;
        _pos.set_yaw(yaw);

        SyncPosAndBroadcast(currentPos, newPos);
    }
    

}

// 헬퍼 함수: 그리드 갱신과 브로드캐스트를 묶어서 처리
void Monster::SyncPosAndBroadcast(Vector3 oldPos, Vector3 newPos)
{
    if (auto room = Getroomptr()) {
        // 섹터 변경 및 그리드 동기화
        room->UpdateObjectGrid(shared_from_this(), oldPos, newPos);
        // 클라이언트에 이동 알림
        room->BroadcastMove(shared_from_this());
    }
}

void Monster::UseSkill(GameObjectPtr targetobj, Vector3 targetPos, int32 skillid)
{
    this->Getroomptr()->HandleSkill(shared_from_this(), targetobj, targetPos, skillid);
}
