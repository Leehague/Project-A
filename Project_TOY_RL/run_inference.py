import sys
import os
import time
import json

# 1. 윈도우 파이썬 3.8+ 에서 vcpkg 및 C++ 빌드 DLL들을 정상적으로 로드할 수 있도록 설정
if os.name == 'nt':
    vcpkg_bin = r"C:\Users\leehague\vcpkg\installed\x64-windows\bin"
    build_bin = r"C:\Users\leehague\Desktop\Project A\Project_TOY_server\out\build\x64-Release"
    if os.path.exists(vcpkg_bin):
        os.add_dll_directory(vcpkg_bin)
    if os.path.exists(build_bin):
        os.add_dll_directory(build_bin)

# .pyd 파일 경로 추가
sys.path.append(build_bin)

try:
    import game_core
except ImportError as e:
    print(f"[오류] game_core 모듈을 가져올 수 없습니다: {e}")
    sys.exit(1)

try:
    from stable_baselines3 import PPO
except ImportError:
    print("[오류] stable_baselines3 라이브러리가 필요합니다. 'pip install stable-baselines3'를 실행해주세요.")
    sys.exit(1)

from env import ToyMonsterEnv

def run_inference(model_path="./models/monster_ppo_model.zip", num_episodes=5, max_steps=500, delay=0.1, save_path=None):
    # 2. 모델 로드 확인
    if not os.path.exists(model_path):
        print(f"[오류] 저장된 모델 파일을 찾을 수 없습니다: {model_path}")
        print("먼저 train.py를 실행하여 모델을 학습하고 저장해주세요.")
        return

    print("=" * 60)
    print(f"저장된 모델 [{model_path}] 로드 중...")
    model = PPO.load(model_path)
    print("모델 로드 성공!")
    print("=" * 60)

    # 3. 환경 생성
    env = ToyMonsterEnv(map_id=1, max_steps=max_steps)

    all_episodes_data = []

    for episode in range(num_episodes):
        print(f"\n--- 에피소드 {episode + 1} 시작 ---")
        obs, info = env.reset()
        
        episode_reward = 0.0
        step_count = 0
        
        episode_data = {
            "episode": episode + 1,
            "total_reward": 0.0,
            "outcome": "timeout",
            "steps": []
        }
        
        while True:
            step_count += 1
            # 모델 예측 (결정론적 추론 수행)
            action, _states = model.predict(obs, deterministic=True)
            
            # 환경 진행
            obs, reward, terminated, truncated, info = env.step(action)
            episode_reward += reward

            # 콘솔에 상세 상태 로그 출력
            action_name = "Chase (MoveTo)" if action == 0 else "Attack" if action == 1 else "Flee"
            print(f"Step {step_count:03d} | Action: {action_name:<14} | Reward: {reward:6.3f} | "
                  f"Monster HP: {info['monster_hp']:3d} | Player HP: {info['player_hp']:3d} | "
                  f"Monster Pos: ({info['monster_pos'][0]:5.1f}, {info['monster_pos'][1]:5.1f}) | "
                  f"Player Pos: ({info['player_pos'][0]:5.1f}, {info['player_pos'][1]:5.1f})")

            # 데이터 로깅
            episode_data["steps"].append({
                "step": step_count,
                "action": int(action),
                "action_name": action_name,
                "reward": float(reward),
                "monster_hp": int(info['monster_hp']),
                "player_hp": int(info['player_hp']),
                "monster_pos_x": float(info['monster_pos'][0]),
                "monster_pos_z": float(info['monster_pos'][1]),
                "player_pos_x": float(info['player_pos'][0]),
                "player_pos_z": float(info['player_pos'][1]),
            })

            # 딜레이를 주어 사람이 시뮬레이션 흐름을 보기 쉽게 처리
            if delay > 0:
                time.sleep(delay)

            if terminated:
                if info['player_hp'] <= 0:
                    outcome = "success"
                    print(f"\n[성공] 몬스터가 플레이어를 성공적으로 사냥했습니다! (소요 스텝: {step_count})")
                elif info['monster_hp'] <= 0:
                    outcome = "fail"
                    print(f"\n[실패] 몬스터가 플레이어에게 공격당해 사망했습니다. (소요 스텝: {step_count})")
                episode_data["outcome"] = outcome
                break
                
            if truncated:
                print(f"\n[타임아웃] 최대 스텝 {max_steps}에 도달하여 에피소드가 중단되었습니다.")
                episode_data["outcome"] = "timeout"
                break

        episode_data["total_reward"] = episode_reward
        all_episodes_data.append(episode_data)
        print(f"에피소드 {episode + 1} 종료! 총 획득 보상: {episode_reward:.2f}")
        print("-" * 60)

    # 4. 파일 저장 처리
    if save_path:
        # 디렉토리 자동 생성
        log_dir = os.path.dirname(save_path)
        if log_dir:
            os.makedirs(log_dir, exist_ok=True)
            
        with open(save_path, "w", encoding="utf-8") as f:
            json.dump(all_episodes_data, f, indent=4, ensure_ascii=False)
        print(f"\n[저장 완료] 추론 테스트 결과가 파일에 저장되었습니다: {save_path}")

    print("\n모든 추론 테스트가 완료되었습니다.")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="몬스터 강화학습 모델 추론 테스트 스크립트")
    parser.add_argument("--model", type=str, default="./models/monster_ppo_model.zip", help="불러올 모델 파일 경로")
    parser.add_argument("--episodes", type=int, default=3, help="테스트할 에피소드 개수")
    parser.add_argument("--steps", type=int, default=500, help="에피소드당 최대 스텝 수")
    parser.add_argument("--delay", type=float, default=0.05, help="스텝 간 시간 간격 (초 단위, 콘솔 출력 가독성용)")
    parser.add_argument("--output", type=str, default="./logs/inference_results.json", help="추론 결과를 저장할 파일 경로 (.json)")
    
    args = parser.parse_args()
    
    run_inference(
        model_path=args.model, 
        num_episodes=args.episodes, 
        max_steps=args.steps, 
        delay=args.delay,
        save_path=args.output
    )
