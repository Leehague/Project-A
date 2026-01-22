@echo off
pushd %~dp0



set PROTOC_PATH="C\vcpkg-master\installedx64-windows\tools\protobuf\protoc.exe"

set OUT_PATH=.C:\Users\ha052\vcpkg\vcpkg-master\buildtrees\protobuf\x64-windows-rel\bin\protoc.exe
set SOURCE_PROTO=.Protocol.proto

echo Generating Protocol files...
%PROTOC_PATH% -I=. --cpp_out=%OUT_PATH% %SOURCE_PROTO%

if %ERRORLEVEL% == 0 (
    echo [SUCCESS] Protocol.pb.h and Protocol.pb.cc generated.
) else (
    echo [ERROR] Protobuf compilation failed.
    pause
)

popd