@echo off
setlocal

if not exist ".venv\Scripts\python.exe" (
  echo ERROR: .venv was not found. Run scripts\setup-windows.cmd first.
  exit /b 1
)

if "%PLATFORMIO_CORE_DIR%"=="" set "PLATFORMIO_CORE_DIR=%CD%\.platformio-core"
set "VERSION=%~1"
if "%VERSION%"=="" set "VERSION=dev"

node --check tools\web_installer\installer.js
if errorlevel 1 exit /b 1

call scripts\generate-solver-tables.cmd
if errorlevel 1 exit /b 1

call scripts\generate-web-ui.cmd
if errorlevel 1 exit /b 1

rem Distribution certificates always include cube-scrambler.local and the
rem default AP address 192.168.4.1. Do not pass a developer LAN IP here.
call scripts\generate-https-cert.cmd
if errorlevel 1 exit /b 1

call .venv\Scripts\activate.bat
pio run -e m5stack-nanoc6-release
if errorlevel 1 exit /b 1

pio run -e m5stack-nanoc6-release -t buildfs
if errorlevel 1 exit /b 1

python tools\check_flash_layout.py ^
  --firmware .pio\build\m5stack-nanoc6-release\firmware.bin ^
  --web .pio\build\m5stack-nanoc6-release\spiffs.bin
if errorlevel 1 exit /b 1

python tools\build_release_bundle.py --version "%VERSION%"
if errorlevel 1 exit /b 1

echo.
echo Release bundle is ready under .pio\release
echo Publishable Web Serial installer: .pio\release\web-installer
echo Local verification: scripts\serve-web-installer.cmd
endlocal
