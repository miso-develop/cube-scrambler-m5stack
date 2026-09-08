@echo off
setlocal

if not exist ".venv\Scripts\python.exe" (
  echo ERROR: .venv was not found. Run scripts\setup-windows.cmd first.
  exit /b 1
)

if not exist ".pio\release\web-installer\index.html" (
  echo ERROR: Web installer bundle was not found.
  echo Run scripts\build-release-bundle.cmd first.
  exit /b 1
)

set "PORT=%~1"
if "%PORT%"=="" set "PORT=8000"

echo.
echo Cube Scrambler Web Serial installer
echo http://localhost:%PORT%/
echo.
echo Press Ctrl+C to stop the local server.
start "" "http://localhost:%PORT%/"
".venv\Scripts\python.exe" -m http.server %PORT% --bind 127.0.0.1 --directory ".pio\release\web-installer"

endlocal
