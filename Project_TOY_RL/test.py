import sys
import os

# 윈도우 파이썬 3.8+ 에서 vcpkg C++ DLL들을 정상적으로 로드할 수 있도록 DLL 디렉토리 명시적 추가
if os.name == 'nt':
    vcpkg_bin = r"C:\Users\leehague\vcpkg\installed\x64-windows\bin"
    build_bin = r"C:\Users\leehague\Desktop\Project A\Project_TOY_server\out\build\x64-Release"
    if os.path.exists(vcpkg_bin):
        os.add_dll_directory(vcpkg_bin)
    if os.path.exists(build_bin):
        os.add_dll_directory(build_bin)

# .pyd 파일이 있는 경로를 시스템 패스에 추가합니다.
sys.path.append(build_bin)

# 이제 C++ 코어로 빌드된 모듈을 파이썬에서 불러올 수 있습니다!
import game_core

print("game_core 모듈 로드 성공!")
print(dir(game_core)) # 모듈 안에 어떤 클래스/함수가 있는지 확인해 봅니다.

# C++ 상대경로 파일(Data/ 등) 로딩 성공을 위해 작업 디렉토리를 임시 변경
original_cwd = os.getcwd()
os.chdir(r"C:\Users\leehague\Desktop\Project A\Project_TOY_server")

from env import ToyMonsterEnv
try:
    print("ToyMonsterEnv 환경 생성 및 reset() 실행 테스트...")
    env = ToyMonsterEnv(map_id=1)
    obs, info = env.reset()
    print("ToyMonsterEnv reset() 성공!")
    print("초기 관측값 (Observation Shape:", obs.shape, "):", obs)
except Exception as e:
    print("테스트 도중 오류 발생:", e)
finally:
    os.chdir(original_cwd)
