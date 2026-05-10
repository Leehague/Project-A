#include "DBConnection.h"
#include <iostream>

DBConnection::DBConnection()
{
}

DBConnection::~DBConnection()
{
    Clear();
}

bool DBConnection::Connect(SQLHENV hEnv, const std::wstring& connectionString)
{
    // 1. 연결 핸들 할당
    if (SQL_SUCCESS != ::SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &_hDbc))
        return false;

    // 2. 타임아웃 설정 (옵션)
    ::SQLSetConnectAttr(_hDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);

    // 3. 실제 연결 시도
    SQLWCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen;

    SQLRETURN ret = ::SQLDriverConnectW(
        _hDbc,
        NULL,
        (SQLWCHAR*)connectionString.c_str(),
        SQL_NTS,
        outConnStr,
        sizeof(outConnStr) / sizeof(SQLWCHAR),
        &outConnStrLen,
        SQL_DRIVER_NOPROMPT
    );

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        // 에러 발생 시 로그 출력 로직 추가 가능
        Clear();
        return false;
    }

    _isConnected = true;
    return true;
}

void DBConnection::Clear()
{
    if (_hDbc != SQL_NULL_HDBC)
    {
        ::SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
        _hDbc = SQL_NULL_HDBC;
    }
    _isConnected = false;
}