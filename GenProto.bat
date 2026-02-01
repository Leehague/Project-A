@echo off
SETLOCAL

:: 1. 기준 경로 설정 (배치 파일 위치를 절대 경로로 고정)
set BASE_PATH=%~dp0
set PROTOC_PATH=C:\Users\leehague\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe
set PROTO_NAME=Protocol.proto

:: 작업 디렉토리 설정
set COMMON_DIR=%BASE_PATH%Common
set COMMON_PROTO_OUT=%COMMON_DIR%\Protocol
set SERVER_DEST=%BASE_PATH%Project_TOY_server\Protocol
set CLIENT_DEST=%BASE_PATH%DummyClient\Protocol

echo [Info] Target Proto: %COMMON_DIR%\%PROTO_NAME%

:: 2. 출력 디렉토리 확인 및 생성
if not exist "%COMMON_PROTO_OUT%" mkdir "%COMMON_PROTO_OUT%"
if not exist "%SERVER_DEST%" mkdir "%SERVER_DEST%"
if not exist "%CLIENT_DEST%" mkdir "%CLIENT_DEST%"

echo [Info] Compiling .proto files...

:: 3. 핵심 수정 부분: 파일이 있는 곳으로 이동 후 컴파일
pushd "%COMMON_DIR%"

:: -I=. 은 현재 폴더(Common)를 기준으로 하겠다는 뜻입니다.
"%PROTOC_PATH%" -I=. --cpp_out=.\Protocol %PROTO_NAME%
if %errorlevel% neq 0 ( echo [Error] C++ Compilation Failed & pause & exit /b )

"%PROTOC_PATH%" -I=. --csharp_out=.\Protocol %PROTO_NAME%
if %errorlevel% neq 0 ( echo [Error] C# Compilation Failed & pause & exit /b )

popd

echo [Info] Compilation Successful.

:: 4. 파일 복사
echo [Info] Copying files...
copy /y "%COMMON_PROTO_OUT%\Protocol.pb.h" "%SERVER_DEST%\"
copy /y "%COMMON_PROTO_OUT%\Protocol.pb.cc" "%SERVER_DEST%\"
copy /y "%COMMON_PROTO_OUT%\Protocol.cs" "%CLIENT_DEST%\"

:: 5. 파이썬 핸들러 실행 (서버 프로젝트 폴더로 이동)
echo [Info] Running Python Packet Handler Generator...
pushd "%BASE_PATH%Project_TOY_server"
python GeneratePacketHandler.py
popd

echo [Success] All tasks completed!
pause