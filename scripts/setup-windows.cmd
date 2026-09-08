@echo off

set "PYTHON312=%USERPROFILE%\scoop\apps\python312\current\python.exe"

if not exist "%PYTHON312%" (
  echo ERROR: Scoop Python 3.12 was not found:
  echo   %PYTHON312%
  echo Install it with: scoop install python312
  exit /b 1
)

echo [1/7] Python 3.12
"%PYTHON312%" --version || exit /b 1

if not exist ".venv\Scripts\python.exe" (
  echo [2/7] Creating .venv
  "%PYTHON312%" -m venv .venv || exit /b 1
) else (
  echo [2/7] .venv already exists
)

echo [3/7] Verifying venv Python
for /f "tokens=2" %%V in ('".venv\Scripts\python.exe" --version 2^>^&1') do set "VENV_PYTHON_VERSION=%%V"
echo Python %VENV_PYTHON_VERSION%
echo %VENV_PYTHON_VERSION% | findstr /b "3.12." >nul
if errorlevel 1 (
  echo ERROR: .venv was not created with Python 3.12.
  echo Delete .venv and run this script again.
  exit /b 1
)

echo [4/7] Installing PlatformIO Core 6.1.19
".venv\Scripts\python.exe" -m pip install --disable-pip-version-check platformio==6.1.19 || exit /b 1

echo [5/7] Installing esptool 5.1.0
".venv\Scripts\python.exe" -m pip install --disable-pip-version-check esptool==5.1.0 || exit /b 1

echo [6/7] Installing cryptography for HTTPS certificate generation
".venv\Scripts\python.exe" -m pip install --disable-pip-version-check "cryptography>=45,<47" || exit /b 1

echo [7/7] Activating project environment
call .venv\Scripts\activate.bat
set "PLATFORMIO_CORE_DIR=%CD%\.platformio-core"
pio --version || exit /b 1
python -m esptool version || exit /b 1

echo.
echo Setup complete. This cmd session is ready to use.
echo PLATFORMIO_CORE_DIR=%PLATFORMIO_CORE_DIR%
echo.
echo Build with:
echo   pio run -e m5stack-nanoc6
echo.
echo For each new cmd session run:
echo   call .venv\Scripts\activate.bat
echo   set PLATFORMIO_CORE_DIR=%%CD%%\.platformio-core
