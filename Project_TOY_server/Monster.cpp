#include "Monster.h"
#include "RoomManager.h"
#include "Map.h"
#include "Player.h"
#include "GameObject.h"

// Monster.cpp (또는 전용 AI 헬퍼)
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
    switch (actionId)
    {
    case 0: // MoveTo
        this->MoveTo(targetPos);
        break;
    case 1: // Basic Attack
        //this->UseSkill(1); // 일반 공격 스킬 ID
        break;
    case 2: // Flee
        //this->FleeFrom(targetPos);
        break;
    }
}

void Monster::MoveTo(Vector3 targetPos)
{
    // [TODO] A* 알고리즘 연동
    // 1. Map::FindPath(myPos, targetPos) 호출하여 경로 리스트 획득
    // 2. 경로의 첫 번째 지점으로 이동 시작
    // 3. CreatureState를 Moving으로 변경 및 주변에 SC_MOVE 브로드캐스트

    RoomPtr myroom = this->Getroomptr();
    _path = myroom->GetMapptr()->FindPath(this->Getpos_As_Vector3(), targetPos);
    
    if (_path.empty())
        return;
    _pos.set_state((int32)CreatureState::Moving);
    
}

void Monster::UpdateAction()
{
    // 1. 판단 로직 (RL 또는 AI Decision)
    uint64 now = ::GetTickCount64();
    if (now >= _nextDecisionTick)
    {
        _nextDecisionTick = now + DECISION_INTERVAL_MS;
        LogicUpdate();
    }

    // 2. 실행 로직 (물리적 이동 업데이트)
    if ((CreatureState)_pos.state() == CreatureState::Moving)
    {
        ProcessMove();
    }
}

void Monster::LogicUpdate()
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
    if (_path.empty())
    {
        SetState(CreatureState::Idle);
        return;
    }

    // 다음 목적지 지점
    Vector3 nextPos = _path.front();
    Vector3 currentPos = Getpos_As_Vector3();

    // 거리 체크 (너무 가까우면 다음 노드로)
    float distSq = Vector3::DistanceSquared(currentPos, nextPos);
    if (distSq < 0.01f)
    {
        _path.erase(_path.begin());
        if (_path.empty())
        {
            SetState(CreatureState::Idle);
            return;
        }
        nextPos = _path.front();
    }

    // 방향 및 Yaw 계산
    Vector3 dir = nextPos - currentPos;
    dir.Normalize();

    // 속도 적용 (Speed * 0.05s)
    float moveDist = GetSpeed() * 0.05f;
    Vector3 newPos = currentPos + (dir * moveDist);

    // 좌표 설정
    _pos.set_x(newPos.x);
    _pos.set_y(newPos.y);
    _pos.set_z(newPos.z);
    _pos.set_yaw(CalculateYaw(dir));

    // 주변 플레이어에게 이동 알림
    if (auto room = Getroomptr())
    {
        room->BroadcastMove(shared_from_this());
    }
}