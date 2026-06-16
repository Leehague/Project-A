import sys
# .pyd 파일이 있는 경로를 시스템 패스에 추가합니다.
sys.path.append(r"C:\Users\leehague\Desktop\Project A\Project_TOY_server\out\build\x64-Release")

# 이제 C++ 코어로 빌드된 모듈을 파이썬에서 불러올 수 있습니다!
import game_core

print("game_core 모듈 로드 성공!")
print(dir(game_core)) # 모듈 안에 어떤 클래스/함수가 있는지 확인해 봅니다.
