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
    
    _colIndexMap.clear();
}

bool DBConnection::BindParam(int paramIndex, int32 value)
{
    if (_hStmt == SQL_NULL_HSTMT)
    {
        if (::SQLAllocHandle(SQL_HANDLE_STMT, _hDbc, &_hStmt) != SQL_SUCCESS)
            return false;
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

std::string DBConnection::GetString(const WCHAR* colName)
{
    if (_hStmt == SQL_NULL_HSTMT) return "";

    auto it = _colIndexMap.find(colName);
    if (it == _colIndexMap.end()) return "";

    // 1. 문자열을 받아올 안전한 고정 크기 버퍼를 준비합니다.
    char buffer[1024] = { 0 };
    SQLLEN indicator = 0;

    // 2. SQL_C_CHAR 타입을 사용하고, 버퍼의 포인터와 크기를 넘겨줍니다.
    SQLRETURN ret = ::SQLGetData(_hStmt, it->second, SQL_C_CHAR, buffer, sizeof(buffer), &indicator);

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        // 3. 데이터가 NULL 이면 빈 문자열 반환
        if (indicator == SQL_NULL_DATA) return "";

        // 4. 받아온 C스타일 문자열(buffer)을 std::string으로 안전하게 변환하여 반환
        return std::string(buffer);
    }

    return "";
}

float DBConnection::GetFloat(const WCHAR* colName)
{
    // 1. 구문 핸들이 유효하지 않으면 기본값 반환
    if (_hStmt == SQL_NULL_HSTMT) return 0.0f;
    // 2. 컬럼 이름으로 인덱스 매핑 검색
    auto it = _colIndexMap.find(colName);
    if (it == _colIndexMap.end()) return 0.0f;
    float value = 0.0f;
    SQLLEN indicator = 0;
    // 3. SQLGetData 호출 (float 형식을 위해 SQL_C_FLOAT 타입 지정)
    SQLRETURN ret = ::SQLGetData(
        _hStmt,
        it->second,      // 컬럼 인덱스
        SQL_C_FLOAT,     // C 데이터 타입 (float)
        &value,          // 데이터를 저장할 변수의 주소
        sizeof(value),   // 변수의 크기
        &indicator       // NULL 여부 및 데이터 크기 수신 지시자
    );
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        // 4. DB에 저장된 값이 NULL 인 경우 기본값인 0.0f 반환
        if (indicator == SQL_NULL_DATA) return 0.0f;

        return value;
    }
    return 0.0f;
}
