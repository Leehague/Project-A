@echo off
:: 배치 파일이 있는 폴더로 작업 디렉토리 고정
pushd %~dp0

:: 1. protoc.exe 경로 (본인 경로에 맞게 따옴표 유지)
set PROTOC_PATH="C:\Users\ha052\vcpkg\vcpkg-master\installed\x64-windows\tools\protobuf\protoc.exe"
set SUB_DIR=Protocol

if not exist %SUB_DIR% (
    mkdir %SUB_DIR%
)

echo [START] Generating Protocol files into ./%SUB_DIR%...

:: 2. 컴파일 실행
:: -I. 은 현재 디렉토리를 소스 경로로 지정
:: --cpp_out=. 은 현재 디렉토리에 결과물 생성
echo Generating Protocol files...
%PROTOC_PATH% -I=. --cpp_out=./%SUB_DIR% Protocol.proto

if %ERRORLEVEL% == 0 (
    echo [SUCCESS] Protocol.pb.h and Protocol.pb.cc generated.
) else (
    echo [ERROR] Protobuf compilation failed.
    pause
)

popd