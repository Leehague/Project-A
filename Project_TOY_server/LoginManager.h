#pragma once
#include <sw/redis++/redis++.h>
#include <string>
#include <memory>
#include <optional>
#include "Types.h"
#include "JobQueue.h"

class GameSession;


struct LoginResult
{
    bool Success = false;
    int32 AccountId = -1;
    std::string ErrorMessage = "";
};

class LoginManager : public JobQueue
{
public:
    
    static std::shared_ptr<LoginManager>& GetInstance()
    {
        static std::shared_ptr<LoginManager> instance = nullptr;
        return instance;
    }

    static void CreateInstance(JobSerializer* jobSerializer);
    

    //서버 시작 시 Redis 연결 초기화
    bool Init(const std::string& host, int port, int poolSize = 10);


    //로그인 처리
    void TryLogin(const std::string& token, SessionPtr& session , int characterId);

    
    //임의로 호출하지 말것!
    LoginManager(JobSerializer* jobSerializer) : JobQueue(jobSerializer)
    {

    }
    ~LoginManager() = default;


    // 복사 방지
    LoginManager(const LoginManager&) = delete;
    LoginManager& operator=(const LoginManager&) = delete;


private:
    // 클라이언트 세션의 토큰 검증 처리
    // 성공 시 true와 함께 AccountName을 채워서 반환합니다.
    LoginResult VerifyLoginToken(const std::string& token);

    // 만료된 토큰 정리 (만약 수동 삭제가 필요한 시나리오일 때)
    void ExpireToken(const std::string& token);
private:
    // redis++의 Redis 객체는 내부적으로 스레드 세이프한 커넥션 풀을 관리합니다.
    std::unique_ptr<sw::redis::Redis> _redis = nullptr;
};
