@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0tools\setup-windows.ps1"
if errorlevel 1 pause
endlocal
