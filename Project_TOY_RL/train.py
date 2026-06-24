import os
from env import ToyMonsterEnv

try:
    from stable_baselines3 import PPO
    from stable_baselines3.common.callbacks import EvalCallback
except ImportError:
    print("Stable-Baselines3 라이브러리가 필요합니다.")
    print("설치 명령: pip install stable-baselines3[extra] gymnasium numpy tensorboard")
    PPO = None

def train():
    if PPO is None:
        print("PPO is none")
        return

    # 1. 강화학습 환경 생성
    # map_id=1 맵 로드 시뮬레이션
    env = ToyMonsterEnv(map_id=1, max_steps=500)

    # 2. 텐서보드 로그 디렉토리 설정
    log_dir = "./logs/tensorboard/"
    os.makedirs(log_dir, exist_ok=True)

    # 3. PPO 알고리즘 모델 초기화
    # - MlpPolicy: 다층 신경망(MLP) 정책 모델 사용
    # - verbose=1: 학습 로그를 콘솔에 상세히 출력
    model = PPO(
        "MlpPolicy",
        env,
        learning_rate=3e-4,
        n_steps=2048,
        batch_size=64,
        n_epochs=10,
        gamma=0.99,
        gae_lambda=0.95,
        clip_range=0.2,
        tensorboard_log=log_dir,
        verbose=1
    )

    print("=" * 50)
    print("Project TOY Monster AI 강화학습을 시작합니다.")
    print("훈련 모니터링: tensorboard --logdir ./logs/tensorboard/")
    print("=" * 50)

    # 4. 학습 수행 (100,000 타임스텝)
    model.learn(total_timesteps=100000)

    # 5. 최종 훈련 모델 저장
    model_path = "./models/monster_ppo_model"
    os.makedirs("./models/", exist_ok=True)
    model.save(model_path)
    
    print(f"훈련 성공! 모델이 다음 경로에 저장되었습니다: {model_path}.zip")

    # 6. 간단한 테스트 시뮬레이션 실행
    print("\n--- 훈련된 모델로 테스트 플레이를 시작합니다 ---")
    obs, info = env.reset()
    for step in range(50):
        action, _states = model.predict(obs, deterministic=True)
        obs, reward, terminated, truncated, info = env.step(action)
        
        print(f"Step {step+1}: Action={action}, Reward={reward:.3f}, "
              f"Monster HP={info['monster_hp']}, Player HP={info['player_hp']}")
        
        if terminated or truncated:
            print("에피소드 종료!")
            break

if __name__ == "__main__":
    train()
