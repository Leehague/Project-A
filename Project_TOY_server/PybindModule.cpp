#include <pybind11/pybind11.h>
#include <pybind11/stl.h>  // std::vector, std::map 등을 파이썬 list, dict와 호환시키기 위해 필수!
#include <pybind11/functional.h> // std::function 콜백을 파이썬 람다/함수와 호환시키기 위해 필수!
#include "CoreRoom.h"
#include "Vector3.h"
#include "GameObject.h"
#include "Creature.h"
#include "Player.h"
#include "Monster.h"
#include "InfoSturct.h"
#include "Session.h" // Session weak_ptr lock 통과를 위해 추가
#include "RecvBuffer.h" // RecvBuffer 멤버 변수 링크 통과를 위해 추가
#include "DataManager.h" // DataManager 데이터 초기화를 위해 추가

namespace py = pybind11;

// =============================================================
// Session.cpp 및 RecvBuffer.cpp 가 빌드 대상에서 제외되므로 발생하는
// LNK2019 해결을 위한 최소한의 Mock 생성자/소멸자 구현체 제공
// =============================================================
Session::Session() {
    // 뼈대 객체용 빈 바디
}
Session::~Session() {
    // 뼈대 객체용 빈 바디
}

RecvBuffer::RecvBuffer() {
    // 뼈대 객체용 빈 바디
}
RecvBuffer::~RecvBuffer() {
    // 뼈대 객체용 빈 바디
}

PYBIND11_MODULE(game_core, m) {
    m.doc() = "Project TOY Simulation Core";

    // =============================================================
    // 1. Enum Types 바인딩
    // (이름 충돌 방지를 위해 .export_values() 제거하여 네임스페이스 격리)
    // =============================================================
    py::enum_<GameObjectType>(m, "GameObjectType")
        .value("None", GameObjectType::None)
        .value("Player", GameObjectType::Player)
        .value("Monster", GameObjectType::Monster)
        .value("Projectile", GameObjectType::Projectile)
        .value("Item", GameObjectType::Item);

    py::enum_<CreatureState>(m, "CreatureState")
        .value("Idle", CreatureState::Idle)
        .value("Moving", CreatureState::Moving)
        .value("Skill", CreatureState::Skill)
        .value("OnDead", CreatureState::OnDead)
        .value("Dead", CreatureState::Dead);

    // ==========================================
    // 2. Structs & Math 바인딩
    // ==========================================
    py::class_<Vector3>(m, "Vector3")
        .def(py::init<float, float, float>())
        .def_readwrite("x", &Vector3::x)
        .def_readwrite("y", &Vector3::y)
        .def_readwrite("z", &Vector3::z)
        .def("__repr__", [](const Vector3& v) {
            return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
        });

    py::class_<Core::PosInfo>(m, "PosInfo")
        .def(py::init<>())
        .def_readwrite("object_id", &Core::PosInfo::object_id)
        .def_readwrite("x", &Core::PosInfo::x)
        .def_readwrite("y", &Core::PosInfo::y)
        .def_readwrite("z", &Core::PosInfo::z)
        .def_readwrite("yaw", &Core::PosInfo::yaw)
        .def_readwrite("state", &Core::PosInfo::state);

    // ==========================================
    // 3. Game Objects & Characters 바인딩 (상속 구조)
    // ==========================================
    
    // Session 바인딩 (Player의 session weak_ptr 유효성 보장을 위한 목적)
    py::class_<Session, std::shared_ptr<Session>>(m, "Session")
        .def(py::init<>());

    // GameObject (부모)
    py::class_<GameObject, std::shared_ptr<GameObject>>(m, "GameObject")
        .def("GetObjectId", &GameObject::GetObjectId)
        .def("SetObjectId", &GameObject::SetObjectId)
        .def("Setpos", [](GameObject& go, const Vector3& vecpos) {
            go.Setpos(vecpos);
        })
        .def("Getpos", &GameObject::Getpos_As_Vector3)
        .def("GetType", &GameObject::GetType)
        .def("GetroomId", &GameObject::GetroomId)
        .def("GetTemplateId", &GameObject::GetTemplateId);

    // Creature (GameObject 상속)
    py::class_<Creature, GameObject, std::shared_ptr<Creature>>(m, "Creature")
        // 오버로딩 모호성이 없으므로 람다 대신 멤버 함수 포인터를 직접 바인딩하여 MSVC 람다 해석 버그 우회
        .def("Init", &Creature::Init)
        .def("GetCurrentHp", &Creature::GetCurrentHp)
        .def("GetMaxHP", &Creature::GetMaxHP)
        .def("GetCurrentMp", &Creature::GetCurrentMp)
        .def("GetMaxMP", &Creature::GetMaxMP)
        .def("GetSpeed", &Creature::GetSpeed)
        .def("GetAttack", &Creature::GetAttack)
        .def("GetName", &Creature::GetName)
        .def("GetState", &Creature::GetState)
        .def("SetState", &Creature::SetState)
        .def("GetSkillCoolTime", &Creature::GetSkillCoolTime)
        .def("SetSkillCoolTime", &Creature::SetSkillCoolTime);

    // Player (Creature 상속)
    py::class_<Player, Creature, std::shared_ptr<Player>>(m, "Player")
        .def(py::init([](int32_t objectId, std::shared_ptr<Session> sessionPtr) {
            return std::make_shared<Player>(objectId, sessionPtr, nullptr);
        }))
        .def(py::init<int32_t, std::shared_ptr<Session>, CoreRoomPtr>());

    // Monster (Creature 상속)
    py::class_<Monster, Creature, std::shared_ptr<Monster>>(m, "Monster")
        .def(py::init([](int32_t objectId) {
            return std::make_shared<Monster>(objectId, nullptr);
        }))
        .def(py::init<int32_t, CoreRoomPtr>())
        // 몬스터 AI용 강화학습 함수 연동
        .def("GatherContext", &Monster::GatherContext)
        .def("ExecuteHighLevelAction", &Monster::ExecuteHighLevelAction)
        .def("MoveTo", &Monster::MoveTo)
        .def("FleeFrom", &Monster::FleeFrom)
        .def("JobUpdate", &Monster::JobUpdate)   // 이동 및 AI 판단 틱 갱신을 위해 추가
        .def("UpdateAction", &Monster::UpdateAction)
        .def("SetRLControlled", &Monster::SetRLControlled)
        .def("IsRLControlled", &Monster::IsRLControlled)
        .def("SetRLPredictCallback", &Monster::SetRLPredictCallback);

    // ==========================================
    // 4. CoreRoom (시뮬레이터 핵심 엔진) 바인딩
    // ==========================================
    py::class_<CoreRoom, std::shared_ptr<CoreRoom>>(m, "CoreRoom")
        // 람다를 활용하여 파이썬 측 생성자 심플하게 변경 (success 인자 생략 가능하도록)
        .def(py::init([](int32_t mapId) {
            // DataManager 전역 리소스/데이터 초기화 구동 (JSON 데이터 로드 보장)
            DataManager::GetInstance().Init();

            bool success = false;
            auto room = std::make_shared<CoreRoom>(mapId, success);
            if (!success) {
                throw std::runtime_error("Map load failed!");
            }
            return room;
        }))
        .def("HandleMove", &CoreRoom::HandleMove)
        .def("HandleSkill", [](CoreRoom& room, CreaturePtr skillUser, GameObjectPtr targetObj, Vector3 targetPos, int32_t skillId) {
            // self 변수명 충돌 방지를 위해 room으로 매개변수명 변경
            return room.HandleSkill(skillUser, targetObj, targetPos, skillId, nullptr, nullptr, nullptr);
        })
        .def("GetNearestPlayer", &CoreRoom::GetNearestPlayer)
        .def("GetAdjacentPlayers", &CoreRoom::GetAdjacentPlayers)
        .def("GetSectorPos", &CoreRoom::GetSectorPos)
        .def("UpdateObjectGrid", &CoreRoom::UpdateObjectGrid)
        .def("AddObject", [](std::shared_ptr<CoreRoom> room, GameObjectPtr go) {
            room->_objects[go->GetObjectId()] = go;
            go->SetCoreroomptr(room);
            auto [cellX, cellZ] = room->GetSectorPos(go->Getpos_As_Vector3());
            if (cellZ >= 0 && cellZ < (int)room->_sectors.size() && cellX >= 0 && cellX < (int)room->_sectors[cellZ].size()) {
                room->_sectors[cellZ][cellX].insert(go);
            }
        })
        .def("RemoveObject", [](CoreRoom& room, int32_t objectId) {
            room._objects.erase(objectId);
        })
        .def_readonly("_objects", &CoreRoom::_objects); // 맵에 배치된 오브젝트 딕셔너리 정보

    // ==========================================
    // 5. TimeManager (가상 시간 시스템) 바인딩
    // ==========================================
    m.def("set_use_virtual_time", [](bool use) { TimeManager::g_useVirtualTime = use; });
    m.def("get_use_virtual_time", []() { return TimeManager::g_useVirtualTime; });
    m.def("set_virtual_time", [](uint64_t tick) { TimeManager::g_virtualTick = tick; });
    m.def("get_virtual_time", []() { return TimeManager::g_virtualTick; });
    m.def("add_virtual_time", [](uint64_t ticks) { TimeManager::g_virtualTick += ticks; });
}
