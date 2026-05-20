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

py -m PyInstaller --clean --noconfirm pusher_motor_host_folder.spec
if errorlevel 1 goto build_failed

echo.
echo Build finished.
echo Output folder: dist\PusherMotorHost
echo Send the whole dist\PusherMotorHost folder, not only the exe inside it.
pause
exit /b 0

:build_failed
echo.
echo [ERROR] Build failed.
pause
exit /b 1
