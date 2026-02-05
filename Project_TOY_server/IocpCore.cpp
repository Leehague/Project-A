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
    // 1. IOCP 핸들 생성 (0은 운영체제가 적절한 스레드 수를 결정하게 함)
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

    bool ret = ::GetQueuedCompletionStatus(_iocpHandle, &bytesTransferred,
        (ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, timeoutMs);

    //완료된 I/O 작업이 있는지 확인 (여기서 스레드가 잠시 멈춤)
    if (ret==false)
    {
        int errCode = ::WSAGetLastError();
        if (errCode != WAIT_TIMEOUT && session)
        {
            //TODO 세션상태 체크 로직 추가

            //에러 종류 판별 및 디버깅 로그 출력
            PrintErrorCode(errCode);

            session->OnDisconnected();
            if (overlappedEx) delete overlappedEx;
            return false;
        }
    }
    
    // 전송된 바이트가 0이면 상대방이 접속을 정상 종료(Graceful Shutdown)한 것
    if (bytesTransferred == 0 && session)
    {
        session->OnDisconnected();
        if (overlappedEx) { delete overlappedEx; }
       

        return true;
    }

    // 성공적으로 작업을 꺼내왔다면 타입에 따라 세션의 콜백 호출
    if (overlappedEx->type == IO_TYPE::RECV)
        session->OnRecv(bytesTransferred);
    else
        session->OnSend(bytesTransferred);

    // 사용한 OverlappedEx 메모리 해제 (풀링 추가 필요)
    delete overlappedEx;
}