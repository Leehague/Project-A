#include "IocpCore.h"
#include "Session.h"
#include <iostream>

IocpCore::IocpCore()
{
    // 1. IOCP 핸들 생성 (0은 운영체제가 적절한 스레드 수를 결정하게 함)
    _iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}

IocpCore::~IocpCore()
{
    if (_iocpHandle != INVALID_HANDLE_VALUE)
        ::CloseHandle(_iocpHandle);
}

bool IocpCore::Register(Session* session)
{
    // 2. 소켓과 IOCP 핸들을 연결 (CompletionKey로 세션 주소를 넘김)
    // 여기서 넘긴 session 주소는 나중에 Dispatch에서 그대로 돌아옴
    HANDLE h = ::CreateIoCompletionPort((HANDLE)session->GetSocket(), _iocpHandle, (ULONG_PTR)session, 0);
    return (h != INVALID_HANDLE_VALUE);
}

bool IocpCore::Dispatch(unsigned int timeoutMs)
{
    DWORD bytesTransferred = 0;
    Session* session = nullptr;
    OverlappedEx* overlappedEx = nullptr;

    // 3. 완료된 I/O 작업이 있는지 확인 (여기서 스레드가 잠시 멈춤)
    if (::GetQueuedCompletionStatus(_iocpHandle, &bytesTransferred,
        (ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, timeoutMs))
    {
        // 4. 성공적으로 작업을 꺼내왔다면 타입에 따라 세션의 콜백 호출
        if (overlappedEx->type == IO_TYPE::RECV)
            session->OnRecv(bytesTransferred);
        else
            session->OnSend(bytesTransferred);

        // 사용한 OverlappedEx 메모리 해제 (풀링 추가 필요)
        delete overlappedEx;
    }
    else
    {
        int errCode = ::WSAGetLastError();
        if (errCode != WAIT_TIMEOUT)
        {
            // 에러 처리 로직
            return false;
        }
    }

    return true;
}