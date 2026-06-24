import torch
from stable_baselines3 import PPO
from env import ToyMonsterEnv

# 1. 기존 환경 및 학습된 모델 로드
env = ToyMonsterEnv(map_id=1, max_steps=500)
model = PPO.load("models/monster_ppo_model", env=env)

# 2. PyTorch 모델의 Actor(Policy) 신경망 추출 및 래핑
class SB3PolicyWrapper(torch.nn.Module):
    def __init__(self, policy):
        super().__init__()
        self.policy = policy
        
    def forward(self, observation):
        # 2-1. Observation을 입력받아 Feature Extractor 통과
        features = self.policy.features_extractor(observation)
        # 2-2. MLP Actor 레이어 통과하여 action logits 계산
        latent_pi = self.policy.mlp_extractor.forward_actor(features)
        logits = self.policy.action_net(latent_pi)
        # 2-3. deterministic=True 상태 추론을 위해 argmax로 가장 확률이 높은 Action ID(int64) 추출
        return torch.argmax(logits, dim=1)

# 평가 모드 설정
onnx_wrapper = SB3PolicyWrapper(model.policy)
onnx_wrapper.eval()

# 3. 더미 입력 데이터 생성 (Observation 크기인 5차원 맞춰 배치 사이즈 1 설정)
dummy_input = torch.randn(1, 5)

# 4. ONNX 모델 익스포트
torch.onnx.export(
    onnx_wrapper,
    dummy_input,
    "models/monster_ppo_model.onnx", #저장할 파일 경로
    export_params=True,
    opset_version=11,
    input_names=["input"],           #입력 텐서 이름
    output_names=["output"],         #출력 텐서 이름
    dynamic_axes={"input": {0: "batch_size"}, "output": {0: "batch_size"}}
)
print("ONNX 모델 파일로 익스포트가 완료되었습니다: models/monster_ppo_model.onnx")
