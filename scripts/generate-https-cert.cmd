@echo off
setlocal

if not exist ".venv\Scripts\python.exe" (
  echo ERROR: .venv was not found. Run scripts\setup-windows.cmd first.
  exit /b 1
)

set "IP_ARG="
if not "%~1"=="" set "IP_ARG=--ip %~1"

".venv\Scripts\python.exe" tools\generate_https_cert.py --output-dir local\https --web-dir .pio\web-ui --hostname cube-scrambler.local %IP_ARG%
if errorlevel 1 exit /b 1

echo.
echo HTTPS certificate material is ready.
echo Install local\https\root-ca.crt ^(or root-ca.der.crt^) as a trusted root CA on the browser device.
endlocal
