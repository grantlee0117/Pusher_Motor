@echo off
setlocal
cd /d "%~dp0"

where py >nul 2>nul
if errorlevel 1 (
    echo [ERROR] Cannot find Python launcher "py".
    echo Install 64-bit Python from python.org, then run this script again.
    pause
    exit /b 1
)

py -m pip install -r requirements.txt
if errorlevel 1 goto build_failed

py -m PyInstaller --clean --noconfirm pusher_motor_host.spec
if errorlevel 1 goto build_failed

echo.
echo Build finished.
echo Output: dist\PusherMotorHost.exe
echo Send this single exe file to the after-sales engineer.
pause
exit /b 0

:build_failed
echo.
echo [ERROR] Build failed.
pause
exit /b 1
