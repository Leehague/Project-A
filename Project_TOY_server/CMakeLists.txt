cmake_minimum_required(VERSION 3.15)
project(GameCoreSimulator)

# C++17 이상 권장
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 파이썬 환경 찾기
find_package(Python COMPONENTS Interpreter Development REQUIRED)

# 2. vcpkg를 통해 설치된 pybind11 찾기
find_package(pybind11 CONFIG REQUIRED)
find_package(Protobuf CONFIG REQUIRED)

# 3. 컴파일할 코어 소스 파일 목록
# 주의: Session.cpp, Room.cpp(기존 네트워크 포함된), main.cpp 등 
# 네트워크/소켓 통신 코드가 들어간 파일은 절대 포함하면 안 됩니다!
set(CORE_SOURCES
    CoreRoom.cpp
    GameObject.cpp
    Creature.cpp
    Player.cpp
    Monster.cpp
    Projectile.cpp
    Map.cpp
    MapManager.cpp
    
    # ... 필요한 다른 순수 로직 소스 파일들 추가 ...
    PybindModule.cpp 
)

# 4. 파이썬 확장 모듈(game_core.pyd) 생성
pybind11_add_module(game_core ${CORE_SOURCES})

# 5. Python 헤더 경로 명시적 포함 및 C++17 강제 설정
target_include_directories(game_core PRIVATE ${Python_INCLUDE_DIRS})
target_compile_features(game_core PRIVATE cxx_std_17)

# 6. Protobuf 라이브러리 명시적 링크
target_link_libraries(game_core PRIVATE protobuf::libprotobuf)

if(MSVC)
    target_compile_options(game_core PRIVATE /Zc:__cplusplus /std:c++17)
    # MSVC 자동 링크 및 디버그 모드 링킹을 원천 차단합니다.
    target_compile_definitions(game_core PRIVATE NDEBUG PROTOBUF_USE_DLLS)
    target_compile_options(game_core PRIVATE /U_DEBUG)
endif()
