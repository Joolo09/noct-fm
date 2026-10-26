@echo off
cd /d "%~dp0.."
set "NOCT_PY=%USERPROFILE%\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
echo NOCT Firmware Lab: http://127.0.0.1:8766/
if exist "%NOCT_PY%" (
  "%NOCT_PY%" tools\emulator.py --port 8766
) else (
  py -3 tools\emulator.py --port 8766
)
pause
