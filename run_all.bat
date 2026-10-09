@echo off
chcp 65001 > nul
echo ========================================================
echo [1/3] Step 1: Checking Cybos Plus (대신증권 32-bit COM)...
echo ========================================================
if exist CybosBridge32.exe (
    start "CybosBridge32" CybosBridge32.exe
    timeout /t 2 /nobreak > nul
)

echo ========================================================
echo [2/3] Step 2: Starting Kiwoom OpenAPI (키움증권 32-bit COM)...
echo ========================================================
if exist KiwoomBridge32.exe (
    start "KiwoomBridge32" KiwoomBridge32.exe
    timeout /t 2 /nobreak > nul
)

echo ========================================================
echo [3/3] Step 3: Launching Main 64-bit Renderer...
echo ========================================================
start "" kiwoom_modular_app.exe