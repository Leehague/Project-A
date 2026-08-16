#include "LoginManager.h"
#include <iostream>

bool LoginManager::Init(const std::string& host, int port, int poolSize)
{
    try
    {
        // 1. 연결 옵션 구성
        sw::redis::ConnectionOptions connectionOptions;
        connectionOptions.host = host;
        connectionOptions.port = port;
        connectionOptions.keep_alive = true; // 소켓 유지 옵션

        // 2. 커넥션 풀 구성 (IOCP 멀티스레드 환경 대비)
        sw::redis::ConnectionPoolOptions poolOptions;
        poolOptions.size = poolSize;

        // 3. Redis 인스턴스 생성
        _redis = std::make_unique<sw::redis::Redis>(connectionOptions, poolOptions);

        // 4. Ping 테스트
        if (_redis->ping() == "PONG")
        {
            std::cout << "[LoginManager] Redis Connection established successfully." << std::endl;
            return true;
        }
    }
    catch (const sw::redis::Error& e)
    {
        std::cerr << "[LoginManager] Redis Init Failed: " << e.what() << std::endl;
    }

    return false;
}

LoginResult LoginManager::VerifyLoginToken(const std::string& token)
{
    LoginResult result;

    if (_redis == nullptr)
    {
        result.Success = false;
        result.ErrorMessage = "Redis connection is not initialized.";
        return result;
    }

    if (token.empty())
    {
        result.Success = false;
        result.ErrorMessage = "Received empty token.";
        return result;
    }

    try
    {
        // 웹 서버의 로그인 Key 규격 맞춤: "Session:Token:{토큰}"
        std::string redisKey = "Session:Token:" + token;

        // Redis에서 데이터 가져오기
        auto val = _redis->get(redisKey);

        if (val)
        {
            // 성공: 토큰이 유효하며 해당 토큰에 바인딩된 계정명 반환
            result.Success = true;
            result.AccountName = *val;
        }
        else
        {
            // 실패: 토큰이 만료되었거나 존재하지 않음
            result.Success = false;
            result.ErrorMessage = "Invalid or expired token.";
        }
    }
    catch (const sw::redis::Error& e)
    {
        result.Success = false;
        result.ErrorMessage = std::string("Redis Exception: ") + e.what();
    }

    return result;
}

void LoginManager::ExpireToken(const std::string& token)
{
    if (_redis == nullptr || token.empty())
        return;

    try
    {
        std::string redisKey = "Session:Token:" + token;
        // 게임에 접속 성공했으므로 1회성 토큰을 Redis에서 즉시 삭제(소모) 처리
        _redis->del(redisKey);
    }
    catch (const sw::redis::Error& e)
    {
        std::cerr << "[LoginManager] Failed to delete token: " << e.what() << std::endl;
    }
}
