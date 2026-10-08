@echo off
chcp 65001 > nul
setlocal

echo [BUILD-KIWOOM-32] Stopping running instances...
taskkill /F /IM KiwoomBridge32.exe > nul 2>&1

echo [BUILD-KIWOOM-32] Loading MSVC x86 (32-bit) Environment...
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars32.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
) else (
    echo [ERROR] vcvars32.bat not found.
    exit /b 1
)

cl.exe /nologo /std:c++17 /EHsc /O2 /utf-8 /DUNICODE /D_UNICODE KiwoomBridge32.cpp /link /OUT:KiwoomBridge32.exe /SUBSYSTEM:CONSOLE ole32.lib oleaut32.lib user32.lib
if %ERRORLEVEL% equ 0 (
    echo [SUCCESS] KiwoomBridge32.exe compiled successfully.
) else (
    echo [FAILED] KiwoomBridge32.exe build failed.
    exit /b %ERRORLEVEL%
)
endlocal