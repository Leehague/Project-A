@echo off
setlocal enabledelayedexpansion

:: 1. 경로 설정 (등호 앞뒤 공백 제거 및 상대 경로 명확화)
set "SOURCE_DIR=.\Common\Data\ForAll"
set "SOURCE_FORSERVER_DIR=.\Common\Data\OnlyForServer"
set "SOURCE_FORCLIENT_DIR=.\Common\Data\OnlyForClient"
set "SERVER_DIR=.\Project_TOY_server\Data"
set "CLIENT_DIR=.\Project_TOY_client_c_sharp_unity\Assets\Resources\Data"
set "MapData_Source_Dir=.\Project_TOY_client_c_sharp_unity\MapData"
set "MapData_Server_Dir=.\Project_TOY_server\Resource\Maps"

echo [Data Sync Start]

:: 2. 소스 폴더 존재 확인
if not exist "%SOURCE_DIR%" (
    echo Error: Source directory %SOURCE_DIR% not found.
    pause
    exit /b
)

:: 3. 서버 데이터 폴더로 복사
if not exist "%SERVER_DIR%" mkdir "%SERVER_DIR%"
if not exist "%MapData_Server_Dir%" mkdir "%MapData_Server_Dir%"

echo Copying JSON files to Server...
xcopy /Y /S /I "%SOURCE_DIR%\*.json" "%SERVER_DIR%"
xcopy /Y /S /I "%SOURCE_FORSERVER_DIR%\*.json" "%SERVER_DIR%"

echo Copying Map TXT files to Server...
:: 기존 .txt 파일 복사 명령어
xcopy /Y /S /I "%MapData_Source_Dir%\*.txt" "%MapData_Server_Dir%"
:: [새로 추가] 새로 생성한 .bin 파일도 서버 폴더로 복사합니다.
xcopy /Y /S /I "%MapData_Source_Dir%\*.bin" "%MapData_Server_Dir%"


:: 4. 클라이언트 데이터 폴더로 복사
if not exist "%CLIENT_DIR%" mkdir "%CLIENT_DIR%"
echo Copying JSON files to Client...
xcopy /Y /S /I "%SOURCE_DIR%\*.json" "%CLIENT_DIR%"

xcopy /Y /S /I "%SOURCE_FORCLIENT_DIR%\*.json" "%CLIENT_DIR%"

:: 5. 로그인 서버 데이터 폴더로 복사
set "LOGIN_SERVER_DIR=.\Project_TOY_Login_Web_server\Data"
if not exist "%LOGIN_SERVER_DIR%" mkdir "%LOGIN_SERVER_DIR%"
xcopy /Y /S /I "%SOURCE_DIR%\*.json" "%LOGIN_SERVER_DIR%"



echo.
echo [Data Sync Complete!]
pause