@echo off
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup.ps1" -Demo
if errorlevel 1 (
  echo Setup did not finish. Read the error above and docs\SETUP.md.
  pause
  exit /b 1
)
echo Ready. Open Launch.cmd to start Poisson Studio.
pause
