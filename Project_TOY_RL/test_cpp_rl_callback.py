import sys
import os
import time

# DLL 로드 경로 설정 (Windows 환경)
if os.name == 'nt':
    vcpkg_bin = r"C:\Users\leehague\vcpkg\installed\x64-windows\bin"
    build_bin = r"C:\Users\leehague\Desktop\Project A\Project_TOY_server\out\build\x64-Release"
    if os.path.exists(vcpkg_bin):
        os.add_dll_directory(vcpkg_bin)
    if os.path.exists(build_bin):
        os.add_dll_directory(build_bin)

sys.path.append(build_bin)

try:
    import game_core
    from stable_baselines3 import PPO
    from env import ToyMonsterEnv
except ImportError as e:
    print(f"[Import Error] {e}")
    sys.exit(1)

def test_callback():
    model_path = "./models/monster_ppo_model.zip"
    if not os.path.exists(model_path):
        print(f"[오류] 모델이 존재하지 않습니다: {model_path}")
        return

    print("모델 로드 중...")
    model = PPO.load(model_path)
    
    print("환경 초기화 중...")
    env = ToyMonsterEnv(map_id=1, max_steps=50)
    obs, info = env.reset()
    
    # C++ Monster 객체에 파이썬 RL 모델의 예측 함수 등록!
    print("C++ Monster에 Python RL 예측 콜백 등록 중...")
    
    # 람다 함수로 래핑하여 std::function<int(const std::vector<float>&)> 규격을 맞춤
    def predict_callback(context_list):
        # C++에서 넘어오는 것은 float 리스트입니다.
        import numpy as np
        obs_arr = np.array(context_list, dtype=np.float32)
        action, _ = model.predict(obs_arr, deterministic=True)
        return int(action)

    env.monster.SetRLPredictCallback(predict_callback)
    
    print("=" * 60)
    print("C++ JobUpdate()를 통한 자율적 RL 의사결정 시뮬레이션 시작")
    print("=" * 60)
    
    for step in range(1, 21):
        # 가상 시간 전진 (의사결정 주기를 맞추기 위함)
        game_core.add_virtual_time(100)
        
        # 몬스터의 JobUpdate()를 호출하면, C++ 내부적으로 
        # RLPredictCallback 호출 -> ActionID 결정 -> ExecuteHighLevelAction -> ProcessMove 순으로 실행됩니다.
        env.monster.JobUpdate()
        
        # 정보 출력
        m_pos = env.monster.Getpos()
        p_pos = env.target_monster.Getpos()
        m_hp = env.monster.GetCurrentHp()
        p_hp = env.target_monster.GetCurrentHp()
        
        print(f"Step {step:02d} | Monster HP: {m_hp:3d} | Target HP: {p_hp:3d} | "
              f"Monster Pos: ({m_pos.x:5.1f}, {m_pos.z:5.1f}) | Target Pos: ({p_pos.x:5.1f}, {p_pos.z:5.1f})")
        
        # 에피소드 종료 조건 체크
        if p_hp <= 0:
            print("\n[성공] 몬스터가 플레이어를 사냥했습니다!")
            break
        if m_hp <= 0:
            print("\n[실패] 몬스터가 사망했습니다.")
            break
            
        time.sleep(0.05)

if __name__ == "__main__":
    test_callback()
