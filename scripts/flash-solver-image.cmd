@echo off
setlocal

if "%~1"=="" (
  echo Usage: %~nx0 ^<solver-image.bin^> [COM-port]
  echo Example: %~nx0 .pio\min2phase-tables.bin COM3
  exit /b 2
)

set "IMAGE=%~1"
set "PORT=%~2"
if "%PORT%"=="" set "PORT=COM3"

if not exist "%IMAGE%" (
  echo ERROR: image not found: %IMAGE%
  exit /b 1
)

if not exist ".venv\Scripts\python.exe" (
  echo ERROR: .venv not found. Run scripts\setup-windows.cmd first.
  exit /b 1
)

echo Flashing solver image
echo   image : %IMAGE%
echo   port  : %PORT%
echo   baud  : 460800
echo   offset: 0x2B0000

".venv\Scripts\python.exe" -m esptool --chip esp32c6 -p %PORT% -b 460800 write-flash 0x2B0000 "%IMAGE%"
if errorlevel 1 exit /b 1

echo.
echo Solver image flash complete.
endlocal
