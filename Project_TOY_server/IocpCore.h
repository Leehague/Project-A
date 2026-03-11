#pragma once
#include "Types.h"
#include <winsock2.h>

class IocpCore
{
public:
    IocpCore();
    ~IocpCore();

    // IOCP 핸들 생성 및 관리
    HANDLE GetHandle() { return _iocpHandle; }

    // 소켓(세션)을 IOCP 핸들에 등록
    bool Register(SessionPtr session);

    // 완료 통보를 확인하는 핵심 함수 (Worker Thread가 호출)
    bool Dispatch(unsigned int timeoutMs = INFINITE);

private:
    HANDLE _iocpHandle;
};