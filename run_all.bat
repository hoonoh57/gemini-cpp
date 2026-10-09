@echo off
chcp 65001 > nul
title Gemini Modular HTS Full System Launcher
echo ========================================================
echo [LAUNCHER] Starting Gemini Modular HTS 3-Binary Engine
echo ========================================================

echo [1/3] Stopping previous running instances...
taskkill /f /im kiwoom_modular_app.exe >nul 2>&1
taskkill /f /im KiwoomBridge32.exe >nul 2>&1
taskkill /f /im CybosBridge32.exe >nul 2>&1

echo [2/3] Launching 32-bit Dedicated Bridges...
if exist KiwoomBridge32.exe (
    start "KiwoomBridge32" KiwoomBridge32.exe
    echo   - KiwoomBridge32.exe launched.
) else (
    echo   - [WARN] KiwoomBridge32.exe not found. Build required.
)

if exist CybosBridge32.exe (
    start "CybosBridge32" CybosBridge32.exe
    echo   - CybosBridge32.exe launched.
) else (
    echo   - [WARN] CybosBridge32.exe not found. Build required.
)

echo [WAIT] Waiting for IPC Named Pipes to establish...
timeout /t 2 /nobreak > nul

echo [3/3] Launching 64-bit Main D2D Engine...
if exist kiwoom_modular_app.exe (
    start "" kiwoom_modular_app.exe
    echo [SUCCESS] Full Modular Architecture Running Successfully.
) else (
    echo [ERROR] kiwoom_modular_app.exe not found. Build required.
)
