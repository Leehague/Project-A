// DBManager.cpp
#include "DBManager.h"
#include <iostream>

bool DBManager::Init(int poolSize, const std::wstring& connStr) {
    // 1. ODBC 환경 핸들 생성
    if (SQL_SUCCESS != ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv))
        return false;

    if (SQL_SUCCESS != ::SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0))
        return false;

    // 2. 연결 풀 생성
    for (int i = 0; i < poolSize; i++) {
        DBConnection* conn = new DBConnection();
        if (conn->Connect(_hEnv, connStr)) {
            _pool.push_back(conn);
            _freePool.push(conn);
        }
        else {
            delete conn;
            return false;
        }
    }

    // 3. DB 전용 쓰레드 실행 (보통 1~2개면 충분합니다)
    for (int i = 0; i < 1; i++) {
        _threads.push_back(std::thread(&DBManager::DBThreadMain, this));
    }

    return true;
}

void DBManager::DBThreadMain() {
    while (true) {
        JobFn job = nullptr;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _condVar.wait(lock, [this] { return !_dbJobs.empty(); });

            job = std::move(_dbJobs.front());
            _dbJobs.pop();
        }

        if (job) job(); // Push된 람다/함수 실행
    }
}

void DBManager::Push(JobFn&& job) {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _dbJobs.push(std::move(job));
    }
    _condVar.notify_one();
}

DBConnection* DBManager::PopConnection() {
    std::lock_guard<std::mutex> lock(_mutex);
    if (_freePool.empty()) return nullptr;
    DBConnection* conn = _freePool.front();
    _freePool.pop();
    return conn;
}

void DBManager::PushConnection(DBConnection* conn) {
    std::lock_guard<std::mutex> lock(_mutex);
    _freePool.push(conn);
}