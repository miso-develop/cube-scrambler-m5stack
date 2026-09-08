@echo off
setlocal

where node >nul 2>nul
if errorlevel 1 (
  echo Node.js 18+ is required to generate Web UI assets.
  exit /b 1
)

node --require .\tools\windows_spawn_sync_shim.js tools\generate_web_ui.js --output .pio\web-ui
if errorlevel 1 exit /b 1

node tools\finalize_web_bootstrap.js --web-dir .pio\web-ui
if errorlevel 1 exit /b 1

echo.
echo Web UI generated under .pio\web-ui
echo Build filesystem image with:
echo   pio run -e m5stack-nanoc6 -t buildfs
echo Flash filesystem image with the stable NanoC6 path:
echo   scripts\flash-web-ui.cmd COM3
