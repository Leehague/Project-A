import gymnasium as gym
from gymnasium import spaces
import numpy as np
import time

try:
    import game_core
except ImportError:
    # 빌드 전이거나 경로가 다를 경우를 위한 모킹/안내 처리
    game_core = None

class ToyMonsterEnv(gym.Env):
    """
    C++ CoreRoom과 Monster를 제어하여 플레이어를 추격/공격하도록 훈련시키는 Gymnasium 환경입니다.
    """
    def __init__(self, map_id=1, max_steps=500):
        super(ToyMonsterEnv, self).__init__()
        
        if game_core is None:
            raise ImportError(
                "game_core.pyd 모듈을 찾을 수 없습니다. C++ 프로젝트를 먼저 빌드해 주세요."
            )
            
        self.map_id = map_id
        self.max_steps = max_steps
        self.current_step = 0
        
        # 1. Action Space 정의: (몬스터의 ExecuteHighLevelAction 매핑)
        # 0: MoveTo (타겟 방향 추격)
        # 1: Basic Attack (스킬 ID 1 사용)
        # 2: FleeFrom (타겟 반대 방향 도망)
        self.action_space = spaces.Discrete(3)
        
        # 2. Observation Space 정의: (Monster::GatherContext 반환 데이터 규격)
        # [0] Monster HP 비율 (0.0 ~ 1.0)
        # [1] Monster MP 비율 (0.0 ~ 1.0)
        # [2] Target(Player) 상대 X 거리 (dx)
        # [3] Target(Player) 상대 Z 거리 (dz)
        # [4] Target(Player) HP 비율 (0.0 ~ 1.0)
        self.observation_space = spaces.Box(
            low=np.array([0.0, 0.0, -np.inf, -np.inf, 0.0], dtype=np.float32),
            high=np.array([1.0, 1.0, np.inf, np.inf, 1.0], dtype=np.float32),
            dtype=np.float32
        )
        
        self.room = None
        self.target_monster = None
        self.monster = None

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        self.current_step = 0
        
        # 가상 시간 초기화 및 활성화
        if game_core:
            game_core.set_use_virtual_time(True)
            game_core.set_virtual_time(0)
        
        # C++ 상대경로 파일(Data/ 및 Maps/) 로드 성공을 위해 작업 디렉토리 임시 변경
        import os
        original_cwd = os.getcwd()
        os.chdir(r"C:\Users\leehague\Desktop\Project A\Project_TOY_server")
        
        try:
            # 1. CoreRoom 초기화
            self.room = game_core.CoreRoom(self.map_id)
            
            # 2. 타겟 몬스터 생성 (원래의 Player 역할을 대신함)
            self.target_monster = game_core.Monster(1)
            self.target_monster.Init(1)  # 템플릿 ID 1 (Knight 스탯 적용)
            
            # 3. 몬스터 생성 (학습 에이전트)
            self.monster = game_core.Monster(2)
            self.monster.Init(2)  # 몬스터 templateId 설정 (StatData.json UnDead ID: 2)
            self.monster.SetRLControlled(True)
            
            # 4. 초기 스폰 위치 설정 (예: 몬스터는 (10, 0, 10), 타겟은 (15, 0, 15))
            monster_start_pos = game_core.Vector3(10.0, 0.0, 10.0)
            target_start_pos = game_core.Vector3(15.0, 0.0, 15.0)
            
            self.monster.Setpos(monster_start_pos)
            self.target_monster.Setpos(target_start_pos)
            
            # 5. CoreRoom에 오브젝트 등록 및 그리드 업데이트
            self.room.AddObject(self.target_monster)
            self.room.AddObject(self.monster)
            
            # 초기 그리드 갱신
            zero_pos = game_core.Vector3(0.0, 0.0, 0.0)
            self.room.UpdateObjectGrid(self.target_monster, zero_pos, target_start_pos)
            self.room.UpdateObjectGrid(self.monster, zero_pos, monster_start_pos)
        finally:
            os.chdir(original_cwd)
        
        observation = self._get_obs()
        info = {}
        return observation, info

    def step(self, action):
        self.current_step += 1
        
        # 매 스텝 가상 시간 100ms씩 진행
        if game_core:
            game_core.add_virtual_time(100)
        
        # 1. 에이전트 Action 적용
        target_pos = self.target_monster.Getpos() # Vector3
        self.monster.ExecuteHighLevelAction(action, target_pos)
        
        # 2. 물리/이동 시뮬레이션 프레임 진행
        # 몬스터의 JobUpdate()를 돌려서 몬스터가 100ms 의사결정 주기마다 위치를 갱신하게 만듭니다.
        # 고속 학습 시 실제 시간을 슬립할 경우 성능 저하가 크므로, 
        # C++의 GetTickCount64() 우회가 되어있지 않다면 여기서는 일단 실제 시뮬레이션 흐름을 반영합니다.
        self.monster.JobUpdate()
        
        # (옵션) 만약 투사체가 활성화되어 있다면 Room 단에서 UpdateProjectile 등을 처리해 줘야 합니다.
        # 현재는 몬스터의 단순 이동 및 타격 시뮬레이션 위주로 단순화합니다.

        # 3. 다음 Observation 획득
        observation = self._get_obs()
        
        # 4. 보상(Reward) 계산
        reward = self._calculate_reward(action)
        
        # 5. 종료 조건 판단 (Terminated)
        terminated = False
        if self.monster.GetCurrentHp() <= 0:
            # 몬스터가 죽으면 실패
            terminated = True
            reward -= 50.0
        elif self.target_monster.GetCurrentHp() <= 0:
            # 타겟 몬스터 사냥 성공
            terminated = True
            reward += 100.0
            
        # 6. 시간 초과 조건 판단 (Truncated)
        truncated = self.current_step >= self.max_steps
        
        info = {
            "monster_hp": self.monster.GetCurrentHp(),
            "player_hp": self.target_monster.GetCurrentHp(), # 하위 호환성을 위해 player 키 명칭 유지
            "monster_pos": (self.monster.Getpos().x, self.monster.Getpos().z),
            "player_pos": (self.target_monster.Getpos().x, self.target_monster.Getpos().z) # 하위 호환성 유지
        }
        
        return observation, reward, terminated, truncated, info

    def _get_obs(self):
        # C++ Monster::GatherContext가 반환한 std::vector<float> 수집
        context = self.monster.GatherContext()
        # [HpRatio, MpRatio, dx, dz, TargetHpRatio] -> numpy array로 캐스팅
        return np.array(context, dtype=np.float32)

    def _calculate_reward(self, action):
        reward = 0.0
        
        # 에이전트(Monster)와 타겟(Monster)의 위치 정보
        m_pos = self.monster.Getpos()
        p_pos = self.target_monster.Getpos()
        
        # 두 캐릭터 간의 거리 계산
        dist = np.sqrt((m_pos.x - p_pos.x)**2 + (m_pos.z - p_pos.z)**2)
        
        # 1. 거리 기반 보상 (가까워질수록 소폭 보상, 너무 멀어지면 페널티)
        if dist < 2.0:
            reward += 0.5  # 사거리 내 접근
        else:
            reward -= dist * 0.01  # 거리 페널티
            
        # 2. 공격 성공 보상 (피격 시 플레이어의 HP가 깎였는지 체크)
        # (이전 step의 HP와 비교하는 방식이 더 정확하나 여기서는 단순 감점을 보상으로 환산)
        # 예시: 플레이어가 대미지를 입었을 때
        # if 플레이어_HP_감소: reward += 10.0
        
        # 3. 시간 경과 페널티 (빠르게 학습 완료하도록 강제)
        reward -= 0.05
        
        return reward
