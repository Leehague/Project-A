#include "IocpCore.h"
#include "Session.h"
#include "Types.h"
#include <iostream>
#include <winerror.h>

void PrintErrorCode(int32_t errorCode)
{
    LPVOID errorMsg;

    // 시스템으로부터 에러 코드에 해당하는 메시지를 가져옵니다.
    ::FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&errorMsg,
        0,
        NULL
    );

    std::wcout << L"Error Code: " << errorCode << L" / Message: " << (wchar_t*)errorMsg << std::endl;

    // 사용된 메모리를 해제합니다.
    ::LocalFree(errorMsg);
}



IocpCore::IocpCore()
{
    // IOCP 핸들 생성 (0은 운영체제가 적절한 스레드 수를 결정하게 함)
    _iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
}

IocpCore::~IocpCore()
{
    if (_iocpHandle != INVALID_HANDLE_VALUE)
        ::CloseHandle(_iocpHandle);
}

//VALID_HANDLE_VALUE 이면 True 리턴
bool IocpCore::Register(SessionPtr session)
{
    // 2. 소켓과 IOCP 핸들을 연결 (CompletionKey로 세션 주소를 넘김)
    // 여기서 넘긴 session 주소는 나중에 Dispatch에서 그대로 돌아옴
    // [수정] session.get()을 사용하여 실제 주소(Session*)를 전달합니다.
    HANDLE h = ::CreateIoCompletionPort(
        (HANDLE)session->GetSocket(),
        _iocpHandle,
        (ULONG_PTR)session.get(), // shared_ptr이 아닌 실제 주소값 전달
        0
    );

    return (h != INVALID_HANDLE_VALUE);
}

bool IocpCore::Dispatch(unsigned int timeoutMs)
{
    DWORD bytesTransferred = 0;
    Session* session = nullptr;
    OverlappedEx* overlappedEx = nullptr;

    // GQCS 호출
    bool ret = ::GetQueuedCompletionStatus(_iocpHandle, &bytesTransferred,
        (ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, timeoutMs);

    if (ret == false)
    {
        int errCode = ::WSAGetLastError();
        if (errCode == WAIT_TIMEOUT) return true; // 타임아웃은 단순 대기 상태이므로 통과

        // [중요] 에러가 발생했더라도 overlappedEx가 있다면 세션 정리 후 계속 진행
        if (overlappedEx && overlappedEx->owner)
        {
            PrintErrorCode(errCode);
            std::cout << "GQCS Failure Log - Error: " << errCode << " Type: " << (int)overlappedEx->type << std::endl;
            overlappedEx->owner->OnDisconnected();
            delete overlappedEx;
        }
        return true; // 루프가 깨지지 않도록 true 반환
    }

    // 상대방이 접속을 끊은 경우 (Graceful Shutdown)
    if (bytesTransferred == 0)
    {
        if (overlappedEx && overlappedEx->owner) 
        {
            std::cout << "Graceful Disconnect detected, session: " << overlappedEx->owner->GetGuid() << std::endl;
            
            
            std::cout << "Disconnect detected during " << (overlappedEx->type == IO_TYPE::RECV ? "RECV" : "SEND") << std::endl;
            overlappedEx->owner->OnDisconnected();
            
        }
        if (overlappedEx) delete overlappedEx;
        return true;
    }

    // [매우 중요] 세션의 소켓이 유효한지 최종 확인 후 콜백 호출
    if (overlappedEx->owner->GetSocket() != INVALID_SOCKET)
    {
        if (overlappedEx->type == IO_TYPE::RECV)
            overlappedEx->owner->OnRecv(bytesTransferred);
        else
            overlappedEx->owner->OnSend(bytesTransferred);
    }
    delete overlappedEx;
    return true; // 성공적으로 처리 완료
}
