#include "Monster.h"
#include "RoomManager.h"
#include "Map.h"
#include "Player.h"
#include "DataManager.h"
#include "DataContents.h"
#include "GameObject.h"
#include "CoreRoom.h"
#include <windows.h>
#include "RLModelManager.h"

std::vector<float> Monster::GatherContext()
{
    std::vector<float> context;

    // 1. 본인 상태 (정규화 권장)
    context.push_back(static_cast<float>(GetCurrentHp()) / GetMaxHP());
    context.push_back(static_cast<float>(GetCurrentMp()) / GetMaxMP());

    // 2. 가장 가까운 타겟(플레이어/몬스터) 정보
    CreaturePtr target = this->GetCoreroomptr()->GetNearestCreature(this->Getpos_As_Vector3(), this->_targetrange, this->GetObjectId()); // 시야(그리드) 내에서 탐색

    
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

    // 3. 스킬 쿨타임

    // 4. 주변 위협


    return context;
}

void Monster::ExecuteHighLevelAction(int actionId, Vector3 targetPos)
{
    //TODO: actionId - action 맵핑을 해야됨 (강화학습 MDP 설정)
    switch (actionId)
    {
    case 0: // MoveTo
        this->SetState(CreatureState::Moving);
        this->MoveTo(targetPos);
        break;
    case 1: // Basic Attack
        this->SetState(CreatureState::Skill);
        this->UseSkill(nullptr,Vector3(0,0,0),101); // 일반 공격 스킬 ID (SkillData.json ID: 101)
        break;
    case 2: // Flee
        this->SetState(CreatureState::Moving);
        this->FleeFrom(targetPos);
        break;
    }
}

void Monster::FleeFrom(const Vector3& targetPos)
{
    Vector3 myPos = Getpos_As_Vector3();
    Vector3 dir = (myPos - targetPos).GetNormalized(); // 방향만 가져옴 (길이 1)
    dir.y = 0.0f; // 위아래로 도망가는 것 방지

    // 반대 방향으로 10.0f (적당한 고정 거리) 만큼 떨어진 곳을 목적지로 설정
    Vector3 destPos = myPos + (dir * 10.0f);
    
    MoveTo(destPos);
}

void Monster::MoveTo(const Vector3& targetPos)
{
    // A* 알고리즘 연동
    // 1. Map::FindPath(myPos, targetPos) 호출하여 경로 리스트 획득
    // 2. 경로의 첫 번째 지점으로 이동 시작
    // 3. CreatureState를 Moving으로 변경 및 주변에 SC_MOVE 브로드캐스트

    CoreRoomPtr mycoreroom = this->GetCoreroomptr();
    if (mycoreroom == nullptr) return;

    auto mapptr = mycoreroom->GetMapptr();
    if (mapptr == nullptr) return;

    if (Getpos_As_Vector3() == targetPos) return;

    std::vector<Vector3> newPath = mycoreroom->GetMapptr()->FindPath(this->Getpos_As_Vector3(), targetPos);
    
    if (!newPath.empty())
    {
        _path = std::move(newPath); 
    }


    for (Vector3 vec : _path) 
    {
        if (std::isnan(vec.x) || std::isnan(vec.y) || std::isnan(vec.z))
        { 
            std::cout << "Invalid path." << std::endl;
            return; 
        }
    }

    _pos.state = (CreatureState::Moving);
    
}

void Monster::JobUpdate()
{
    uint64 now = TimeManager::GetTickCount64();
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

    // 가장 가까운 타겟(플레이어/몬스터) 탐색
    CreaturePtr target = GetCoreroomptr()->GetNearestCreature(Getpos_As_Vector3(), _targetrange, GetObjectId());
    Vector3 targetPos = target ? target->Getpos_As_Vector3() : Vector3(0.0f, 0.0f, 0.0f);

    // 1. 파이썬 환경(훈련 시뮬레이션)에서 직접 콜백을 준 경우 (우선 처리)
    if (_rlPredictCallback)
    {
        int actionId = _rlPredictCallback(context);
        ExecuteHighLevelAction(actionId, targetPos);
    }
    // 2. 실서버에서 강화학습 컨트롤을 적용해야 할 경우 (ONNX 라이브러리 추론) 비동기
    else if (_isRLControlled)
    {
        auto roomQueue = GetOwnerJobQueue(); //1단계에서 주입받은 큐 획득
        auto self = std::static_pointer_cast<Monster>(shared_from_this());

         // 실서버 환경일 때만 비동기 큐를 타게 됨 (Python 환경에서는 roomQueue가 nullptr이므로 통과)
        if (roomQueue && self)
        {
            InferenceRequest req;
            req.monster = self;
            req.room = roomQueue; 
            req.context = context;
            RLModelManager::GetInstance().PushRequest(std::move(req));
        }


    }
    // 3. 기존의 룰 기반 AI (일반 추격 및 Idle FSM)
    else
    {
        if (target)
        {
            Vector3 currentpos = this->Getpos_As_Vector3();
            
            float distancesauared = Vector3::DistanceSquared(targetPos, currentpos);

            
            
            if (distancesauared < 5.0) //temp hardcoding
            {
                ExecuteHighLevelAction(1, targetPos); //basic attack
            }
            else
            {
                ExecuteHighLevelAction(0, targetPos); //moveto
            }
            
            
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
}


void Monster::ProcessMove()
{
    if (this->GetState() == CreatureState::Skill)
    {
        return;
    }

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
        std::cout << "ProcessMove ::(currentPos) Invalid position request" << std::endl;
        return;
    }

    if (std::isnan(nextPos.x) || std::isnan(nextPos.y) || std::isnan(nextPos.z))
    {
        std::cout << "ProcessMove ::(nextPos) Invalid position request" << std::endl;
        _path.erase(_path.begin()); // 터진 좌표는 제거
        return; 
    }

    Vector3 Normlizeddir = dir * (1.0f / dist);
    if (dist <= moveDist || dist < 0.01f)
    {
        // 목적지에 정확히 안착 (오차 누적 방지)
        _pos.x=nextPos.x;
        _pos.y=nextPos.y;
        _pos.z=nextPos.z;

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
            _pos.yaw=yaw;
        
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
        _pos.x=newPos.x;
        _pos.y=newPos.y;
        _pos.z=newPos.z;

        float yaw = CalculateYaw(Normlizeddir);
        // Yaw 값 범위 강제 제한 (0~360)
        if (std::isnan(yaw)) 
        { 
            std::cout << "yaw isnan" << std::endl;
            yaw = 0.0f;
        }

        //std::cout << "yaw: "<<yaw << std::endl;
        _pos.yaw=yaw;

        SyncPosAndBroadcast(currentPos, newPos);
    }
    

}

// 헬퍼 함수: 그리드 갱신과 브로드캐스트를 묶어서 처리
void Monster::SyncPosAndBroadcast(Vector3 oldPos, Vector3 newPos)
{
    if (auto room = GetCoreroomptr()) {
        // 섹터 변경 및 그리드 동기화
        room->UpdateObjectGrid(shared_from_this(), oldPos, newPos);
        // 코어 룸에 이동 사실을 알림 (실제 브로드캐스트는 룸에서 콜백으로 처리)
        room->OnObjectMoved(shared_from_this());
    }
}

void Monster::UseSkill(GameObjectPtr targetobj, Vector3 targetPos, int32 skillid)
{
    if (_onUseSkillCallback)
    {
        _onUseSkillCallback(std::static_pointer_cast<Monster>(shared_from_this()), targetobj, targetPos, skillid);
    }
}
