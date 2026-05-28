#pragma once
#include <windows.h>
#include <sqlext.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "Types.h"

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


    // 쿼리 실행 및 결과 패치
    bool BindParam(int paramIndex, int32 value);
    bool Execute(const WCHAR* query);
    bool Fetch();
    int32 GetInt(const WCHAR* colName);
    std::string GetString(const WCHAR* colName);


    // 구문(Statement) 초기화
    void FreeStmt();
private:
    SQLHDBC     _hDbc = SQL_NULL_HDBC;      // 연결 핸들
    bool        _isConnected = false;

    SQLHSTMT    _hStmt = SQL_NULL_HSTMT;    // 구문 핸들

    // 파라미터 바인딩 생명주기를 위한 메모리 유지
    std::vector<int32> _boundInts;
    std::vector<SQLLEN> _boundIndicators;

    // Fetch시 컬럼 이름을 인덱스로 매핑
    std::unordered_map<std::wstring, SQLUSMALLINT> _colIndexMap;
};
