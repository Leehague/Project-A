#include "LoginManager.h"
#include <iostream>
#include "InfoSturct.h"
#include "DBManager.h"
#include "ObjectManager.h"
#include "Player.h"
#include "Protocol/Protocol.pb.h"
#include "ServerUtils.h"
#include "Session.h"

void LoginManager::CreateInstance(JobSerializer* jobSerializer)
{
    GetInstance() = std::make_shared<LoginManager>(jobSerializer);
}


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

void LoginManager::TryLogin(const std::string& token, SessionPtr& session, int characterId)
{
    std::weak_ptr<LoginManager> weakSelf = std::static_pointer_cast<LoginManager>(shared_from_this());
    this->Push([weakSelf, token, session, characterId]()
        {
            LoginResult loginresult = LoginManager::GetInstance()->VerifyLoginToken(token);


            // DB에서 위치 정보를 받아올 변수
            
            PlayerPtr player;

            PlayerDBData data;


            if (DBManager::GetInstance().GetCharacterInfo(loginresult.AccountId, characterId, data))
            {
                //참고 : characterId 는  DB에서의 Id임
                GameObjectPtr go = GObjcetManager.Create(GameObjectType::Player, session, data.TemplateId, nullptr, characterId);
                player = std::static_pointer_cast<Player>(go);


                player->Setpos(data.posInfo);

                if (DBManager::GetInstance().LoadPlayerInventory(characterId, player))
                {
                    loginresult.Success = true;
                }
                else
                {
                    GObjcetManager.Removeobjcet(go->GetObjectId());
                    loginresult.Success = false;
                    player = nullptr;
                }
            }
            else
            {
                //TODO: 여기서 새로운씬 (로비)로 유도해야함
                loginresult.Success = false;
            }



            // 응답 전송 
            Protocol::SC_LOGIN_OK resPkt;

            resPkt.set_success(loginresult.Success);

            if (player)
            {
                resPkt.set_player_id(player->GetObjectId()); //반드시 로그인 요청을 한 유저의 playerId(objectId)로 답을 해주어야함
            }

            resPkt.set_error_message(loginresult.ErrorMessage);
            //참고 SC_LOGIN_OK 에서의 player Id는 ObjectManager에서 관리하는 objectId와 동일함

            auto sendBuffer = ServerUtils::MakeSendBuffer(resPkt, Protocol::PKT_SC_LOGIN_OK);

            if (sendBuffer)
            {
                session->Send(sendBuffer);
            }




        }

    );

    
    
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

        std::string acountidstr = *val;
        if (val)
        {

            try {
                result.Success = true;
                result.AccountId = std::stoi(acountidstr);
            }
            catch (const std::out_of_range& e)
            {
                // int 범위를 벗어나는 너무 큰 숫자가 들어왔을 때의 예외 처리
                result.Success = false;
                result.ErrorMessage = "AccountId value is out of integer range: " + acountidstr;
            }
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
