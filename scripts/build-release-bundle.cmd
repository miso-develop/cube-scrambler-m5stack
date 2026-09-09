@echo off
setlocal

if not exist ".venv\Scripts\python.exe" (
  echo ERROR: .venv was not found. Run scripts\setup-windows.cmd first.
  exit /b 1
)

if "%PLATFORMIO_CORE_DIR%"=="" set "PLATFORMIO_CORE_DIR=%CD%\.platformio-core"
set "VERSION=%~1"
if "%VERSION%"=="" set "VERSION=dev"
set "DEVICE=%~2"
if "%DEVICE%"=="" (
  echo ERROR: device id is required.
  echo Usage: scripts\build-release-bundle.cmd [version] ^<device-id^>
  echo Example: scripts\build-release-bundle.cmd dev m5stack-nanoc6
  exit /b 1
)

set "PIO_ENV="
for /f "usebackq delims=" %%I in (`".venv\Scripts\python.exe" -c "import sys; sys.path.insert(0, 'tools'); from device_profiles import load_device_profile; print(load_device_profile(device_id=sys.argv[1]).platformio_release_env)" "%DEVICE%"`) do set "PIO_ENV=%%I"
if "%PIO_ENV%"=="" (
  echo ERROR: failed to resolve PlatformIO release environment for %DEVICE%.
  exit /b 1
)

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
pio run -e "%PIO_ENV%"
if errorlevel 1 exit /b 1

pio run -e "%PIO_ENV%" -t buildfs
if errorlevel 1 exit /b 1

python tools\check_flash_layout.py --device "%DEVICE%"
if errorlevel 1 exit /b 1

python tools\build_release_bundle.py --device "%DEVICE%" --version "%VERSION%"
if errorlevel 1 exit /b 1

echo.
echo Release bundle is ready under .pio\release
echo Device: %DEVICE%
echo Publishable Web Serial installer: .pio\release\web-installer
echo Local verification: scripts\serve-web-installer.cmd
endlocal
