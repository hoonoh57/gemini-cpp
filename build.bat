@echo off
cl.exe /O2 /EHsc /utf-8 /DUNICODE /D_UNICODE /std:c++17 main.cpp /Fe:kiwoom_modular_app.exe /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib comctl32.lib comdlg32.lib d2d1.lib dwrite.lib /nologo
if %ERRORLEVEL% EQU 0 (
    echo [BUILD SUCCESS] Starting kiwoom_modular_app.exe...
    start kiwoom_modular_app.exe
)