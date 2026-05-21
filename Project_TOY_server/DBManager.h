// DBManager.h
#pragma once
#include "DBConnection.h"
#include "JobQueue.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <thread>
#include "Types.h"

class DBManager {
public:
    static DBManager& GetInstance() { static DBManager instance; return instance; }

    // 초기화: 환경 핸들 생성 및 풀 생성
    bool Init(int poolSize, const std::wstring& connStr);

    // DB 전용 쓰레드 메인 루프
    void DBThreadMain();

    // 일감 추가
    void Push(JobFn&& job);

    // 풀에서 연결 하나를 빌려옴
    DBConnection* PopConnection();

    // 사용 후 연결을 다시 풀에 넣음
    void PushConnection(DBConnection* conn);

    //DB를 호출하는 기능별 함수들

    //플레이어 메모리에 DB의 플레이어가 가진 아이템 전부의 정보 가져옴
    bool LoadPlayerInventory(int32 playerDBId, PlayerPtr player);
    
    //해당 계정의 CharacterId와 TemplateId 를 DB에서 가져옴
    bool GetCharacterInfo(int32 accountId, int32& outCharacterId, int32& outTemplateId);



private:
    SQLHENV _hEnv = SQL_NULL_HENV;
    std::vector<DBConnection*> _pool; // 전체 연결 리스트
    std::queue<DBConnection*> _freePool; // 현재 사용 가능한 연결 큐

    std::queue<JobFn> _dbJobs; // DB 작업 대기열
    std::mutex _mutex;
    std::condition_variable _condVar;
    std::vector<std::thread> _threads; // DB 전용 쓰레드 풀
};
