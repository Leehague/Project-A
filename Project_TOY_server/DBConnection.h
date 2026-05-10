#pragma once
#include <windows.h>
#include <sqlext.h>
#include <string>

// ODBC 라이브러리 링크
#pragma comment(lib, "odbc32.lib")

class DBConnection
{
public:
    DBConnection();
    ~DBConnection();

    // 연결 시도 (LocalDB용 커넥션 스트링 포함)
    bool Connect(SQLHENV hEnv, const std::wstring& connectionString);

    // 연결 해제
    void Clear();

    // 쿼리 실행용 핸들 반환
    SQLHDBC GetConnectionHandle() { return _hDbc; }

private:
    SQLHDBC     _hDbc = SQL_NULL_HDBC;      // 연결 핸들
    bool        _isConnected = false;
};