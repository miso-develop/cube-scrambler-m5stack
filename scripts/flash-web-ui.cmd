@echo off
setlocal

set "PORT=%~1"
if "%PORT%"=="" set "PORT=COM3"
set "IMAGE=.pio\build\m5stack-nanoc6\spiffs.bin"

if not exist ".venv\Scripts\python.exe" (
  echo ERROR: .venv not found. Run scripts\setup-windows.cmd first.
  exit /b 1
)

if not exist "%IMAGE%" (
  echo SPIFFS image not found. Building it first...
  pio run -e m5stack-nanoc6 -t buildfs
  if errorlevel 1 exit /b 1
)

echo Flashing Web UI SPIFFS image
echo   image : %IMAGE%
echo   port  : %PORT%
echo   baud  : 460800
echo   offset: 0x3C0000

".venv\Scripts\python.exe" -m esptool --chip esp32c6 -p %PORT% -b 460800 write-flash 0x3C0000 "%IMAGE%"
if errorlevel 1 exit /b 1

echo.
echo Web UI flash complete.
endlocal
