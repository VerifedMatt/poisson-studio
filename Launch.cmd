@echo off
cd /d "%~dp0"
if not exist "%~dp0PoissonStudio.exe" (
  echo Run Setup.cmd first to download dependencies and build Poisson Studio.
  pause
  exit /b 1
)
start "Poisson Studio" "%~dp0PoissonStudio.exe"
