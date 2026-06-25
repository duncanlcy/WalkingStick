@echo off
setlocal

cd /d "%~dp0.."

if not exist "demo\index.html" (
  echo ERROR: demo folder not found in %CD%
  echo Run: git pull
  exit /b 1
)

set "PORT=%PORT%"
if "%PORT%"=="" set "PORT=8080"

where python >nul 2>nul
if %errorlevel%==0 (
  echo Starting WalkingStick demo on http://localhost:%PORT%
  python demo\server.py --port %PORT% %*
  exit /b %errorlevel%
)

where py >nul 2>nul
if %errorlevel%==0 (
  echo Starting WalkingStick demo on http://localhost:%PORT%
  py -3 demo\server.py --port %PORT% %*
  exit /b %errorlevel%
)

echo ERROR: Python 3 not found. Install from https://www.python.org/downloads/
exit /b 1
