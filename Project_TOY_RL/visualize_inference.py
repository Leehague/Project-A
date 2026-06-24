import os
import sys
import json

try:
    import matplotlib.pyplot as plt
    import numpy as np
except ImportError:
    print("[오류] 시각화를 위해 matplotlib, numpy 패키지가 필요합니다.")
    print("설치 명령: pip install matplotlib numpy")
    sys.exit(1)

def load_data(file_path):
    if not os.path.exists(file_path):
        print(f"[오류] 데이터 파일이 존재하지 않습니다: {file_path}")
        sys.exit(1)
        
    with open(file_path, "r", encoding="utf-8") as f:
        return json.load(f)

def analyze_and_plot(data_path="./logs/inference_results.json", save_img_path="./logs/inference_analysis.png", episode_idx=1):
    all_episodes = load_data(data_path)
    
    # 에피소드 인덱스 조정 (1-based index)
    if episode_idx < 1 or episode_idx > len(all_episodes):
        print(f"[경고] 에피소드 {episode_idx}가 유효하지 않습니다. 첫 번째 에피소드로 대체합니다. (총 에피소드 수: {len(all_episodes)})")
        episode_idx = 1
        
    ep_data = all_episodes[episode_idx - 1]
    steps = ep_data["steps"]
    
    if not steps:
        print(f"[오류] 에피소드 {episode_idx}에 저장된 스텝 데이터가 없습니다.")
        return
        
    print(f"\n[분석 중] 에피소드 {episode_idx} 분석 시각화 진행 중...")
    print(f"결과: {ep_data['outcome'].upper()} | 총 보상: {ep_data['total_reward']:.2f} | 총 스텝: {len(steps)}")
    
    # 데이터 추출
    step_seq = [s["step"] for s in steps]
    monster_x = [s["monster_pos_x"] for s in steps]
    monster_z = [s["monster_pos_z"] for s in steps]
    player_x = [s["player_pos_x"] for s in steps]
    player_z = [s["player_pos_z"] for s in steps]
    monster_hp = [s["monster_hp"] for s in steps]
    player_hp = [s["player_hp"] for s in steps]
    rewards = [s["reward"] for s in steps]
    cum_rewards = np.cumsum(rewards)
    actions = [s["action"] for s in steps]
    
    # 액션명 세팅
    action_names = {0: "Chase", 1: "Attack", 2: "Flee"}
    action_counts = {action_names[0]: 0, action_names[1]: 0, action_names[2]: 0}
    for a in actions:
        name = action_names.get(a, f"Action {a}")
        action_counts[name] = action_counts.get(name, 0) + 1
        
    # 시각화 설정 (2x2 서브플롯)
    fig, axs = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f"Monster AI Inference Analysis - Episode {episode_idx} (Outcome: {ep_data['outcome'].upper()})", fontsize=16, fontweight='bold')
    
    # 1. 2D 궤적 플롯 (Trajectory)
    ax_traj = axs[0, 0]
    ax_traj.plot(monster_x, monster_z, label="Monster Path", color="crimson", linewidth=2.0, alpha=0.8)
    ax_traj.plot(player_x, player_z, label="Player Path", color="royalblue", linewidth=1.5, linestyle="--", alpha=0.6)
    
    # 시작점/끝점 표시
    ax_traj.scatter(monster_x[0], monster_z[0], color="darkred", marker="o", s=80, label="Monster Start", zorder=5)
    ax_traj.scatter(monster_x[-1], monster_z[-1], color="darkred", marker="X", s=100, label="Monster End", zorder=5)
    ax_traj.scatter(player_x[0], player_z[0], color="darkblue", marker="o", s=80, label="Player Start", zorder=5)
    ax_traj.scatter(player_x[-1], player_z[-1], color="darkblue", marker="X", s=100, label="Player End", zorder=5)
    
    ax_traj.set_title("2D Space Trajectory (Chase Path)", fontsize=12, fontweight='bold')
    ax_traj.set_xlabel("X Coordinate", fontsize=10)
    ax_traj.set_ylabel("Z Coordinate", fontsize=10)
    ax_traj.grid(True, linestyle=":", alpha=0.6)
    ax_traj.legend(fontsize=9, loc="best")
    
    # 2. HP 추이 플롯 (HP Trends)
    ax_hp = axs[0, 1]
    ax_hp.plot(step_seq, monster_hp, label="Monster HP", color="firebrick", linewidth=2)
    ax_hp.plot(step_seq, player_hp, label="Player HP", color="forestgreen", linewidth=2)
    ax_hp.set_title("Monster & Player HP Over Steps", fontsize=12, fontweight='bold')
    ax_hp.set_xlabel("Steps", fontsize=10)
    ax_hp.set_ylabel("HP", fontsize=10)
    ax_hp.set_ylim(-5, 105)
    ax_hp.grid(True, linestyle=":", alpha=0.6)
    ax_hp.legend(fontsize=9)
    
    # 3. 행동 분포 플롯 (Action Distribution)
    ax_act = axs[1, 0]
    names = list(action_counts.keys())
    counts = list(action_counts.values())
    colors = ["salmon", "mediumpurple", "khaki"]
    bars = ax_act.bar(names, counts, color=colors, edgecolor="grey", width=0.5)
    
    # 막대 그래프 상단 수치 표기
    for bar in bars:
        height = bar.get_height()
        ax_act.annotate(f'{int(height)}',
                    xy=(bar.get_x() + bar.get_width() / 2, height),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=9)
                    
    ax_act.set_title("Action Selection Frequency", fontsize=12, fontweight='bold')
    ax_act.set_xlabel("Actions", fontsize=10)
    ax_act.set_ylabel("Count", fontsize=10)
    ax_act.grid(axis='y', linestyle=":", alpha=0.6)
    
    # 4. 누적 보상 플롯 (Cumulative Rewards)
    ax_rew = axs[1, 1]
    ax_rew.plot(step_seq, cum_rewards, color="darkorange", linewidth=2, label="Cumulative Reward")
    ax_rew.axhline(0, color="grey", linestyle="-.", linewidth=0.8)
    ax_rew.set_title("Cumulative Reward Over Steps", fontsize=12, fontweight='bold')
    ax_rew.set_xlabel("Steps", fontsize=10)
    ax_rew.set_ylabel("Reward Value", fontsize=10)
    ax_rew.grid(True, linestyle=":", alpha=0.6)
    ax_rew.legend(fontsize=9)
    
    plt.tight_layout()
    
    # 이미지 저장
    os.makedirs(os.path.dirname(save_img_path), exist_ok=True)
    plt.savefig(save_img_path, dpi=150)
    print(f"[저장 완료] 분석 결과 차트가 저장되었습니다: {save_img_path}")
    
    # GUI 화면에 그래프 출력 (지원되는 호스트 환경일 경우)
    try:
        plt.show()
    except Exception:
        pass

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="몬스터 강화학습 추론 결과 분석 및 시각화 스크립트")
    parser.add_argument("--input", type=str, default="./logs/inference_results.json", help="분석할 로그 파일 경로 (.json)")
    parser.add_argument("--output", type=str, default="./logs/inference_analysis.png", help="저장할 시각화 이미지 경로 (.png)")
    parser.add_argument("--episode", type=int, default=1, help="시각화할 에피소드 번호 (1-based)")
    
    args = parser.parse_args()
    
    analyze_and_plot(data_path=args.input, save_img_path=args.output, episode_idx=args.episode)
