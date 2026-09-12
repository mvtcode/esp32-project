@echo off
chcp 65001 > nul
title ESP32-S3 Volume Sync Companion

echo ========================================================
echo   ESP32-S3 Realtime Volume Sync Companion Starter
echo ========================================================
echo.

cd /d "%~dp0"

echo [1/2] Kiem tra thu vien phan mem can thiet...
python -m pip install -q -r requirements.txt
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Loi cai dat thu vien Python. Vui long kiem tra lai Python tren may tinh!
    pause
    exit /b %ERRORLEVEL%
)

echo [2/2] Khoi dong ung dung dong bo am luong 2 chieu...
echo.
python volume_sync.py

pause
