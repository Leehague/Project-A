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
    FreeStmt(); // 구문 핸들 먼저 해제

    if (_hDbc != SQL_NULL_HDBC)
    {
        ::SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
        _hDbc = SQL_NULL_HDBC;
    }
    _isConnected = false;
}


void DBConnection::FreeStmt()
{
    if (_hStmt != SQL_NULL_HSTMT)
    {
        ::SQLFreeHandle(SQL_HANDLE_STMT, _hStmt);
        _hStmt = SQL_NULL_HSTMT;
    }
    _boundInts.clear();
    _boundIndicators.clear();
    _colIndexMap.clear();
}

bool DBConnection::BindParam(int paramIndex, int32 value)
{
    if (_hStmt == SQL_NULL_HSTMT)
    {
        if (::SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &_hStmt) != SQL_SUCCESS)
            return false;
    }

    // 인덱스는 1부터 시작하므로 공간 확보
    if ((size_t)paramIndex > _boundInts.size())
    {
        _boundInts.resize(paramIndex, 0);
        _boundIndicators.resize(paramIndex, 0);
    }

    _boundInts[paramIndex - 1] = value;
    _boundIndicators[paramIndex - 1] = 0;

    SQLRETURN ret = ::SQLBindParameter(
        _hStmt, paramIndex, SQL_PARAM_INPUT, SQL_C_SLONG, SQL_INTEGER, 0, 0,
        &_boundInts[paramIndex - 1], 0, &_boundIndicators[paramIndex - 1]);

    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

bool DBConnection::Execute(const WCHAR* query)
{
    if (_hStmt == SQL_NULL_HSTMT)
    {
        if (::SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &_hStmt) != SQL_SUCCESS)
            return false;
    }
    else
    {
        ::SQLCloseCursor(_hStmt); // 재사용 시 이전 결과셋 닫기
    }

    SQLRETURN ret = ::SQLExecDirectW(_hStmt, (SQLWCHAR*)query, SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        return false;

    // 컬럼 메타데이터 캐싱 (이름 -> 인덱스)
    _colIndexMap.clear();
    SQLSMALLINT numCols = 0;
    ::SQLNumResultCols(_hStmt, &numCols);

    for (SQLUSMALLINT i = 1; i <= numCols; ++i)
    {
        SQLWCHAR colName[256];
        SQLSMALLINT nameLen;
        ::SQLDescribeColW(_hStmt, i, colName, sizeof(colName) / sizeof(WCHAR), &nameLen, nullptr, nullptr, nullptr, nullptr);
        _colIndexMap[std::wstring(colName)] = i;
    }
    return true;
}

bool DBConnection::Fetch()
{
    if (_hStmt == SQL_NULL_HSTMT) return false;
    SQLRETURN ret = ::SQLFetch(_hStmt);
    return (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO);
}

int32 DBConnection::GetInt(const WCHAR* colName)
{
    if (_hStmt == SQL_NULL_HSTMT) return 0;

    auto it = _colIndexMap.find(colName);
    if (it == _colIndexMap.end()) return 0;

    int32 value = 0;
    SQLLEN indicator = 0;
    SQLRETURN ret = ::SQLGetData(_hStmt, it->second, SQL_C_SLONG, &value, sizeof(value), &indicator);

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        if (indicator == SQL_NULL_DATA) return 0;
        return value;
    }
    return 0;
}
