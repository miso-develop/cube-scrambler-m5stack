@echo off
setlocal

where node >nul 2>nul
if errorlevel 1 (
  echo ERROR: Node.js was not found in PATH.
  echo Install Node.js 18 or later, then run this script again.
  exit /b 1
)

echo Generating full Min2Phase table image...
node tools\generate_min2phase_tables.js --output .pio\min2phase-tables.bin
if errorlevel 1 exit /b 1

echo.
echo Generated: .pio\min2phase-tables.bin
echo Flash with:
echo   scripts\flash-solver-image.cmd .pio\min2phase-tables.bin COM3

endlocal
