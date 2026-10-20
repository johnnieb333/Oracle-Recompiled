@echo off
setlocal
if exist "%~dp0build-launcher\bin\oracle-launcher.exe" (
  start "" "%~dp0build-launcher\bin\oracle-launcher.exe"
) else (
  call "%~dp0Setup-Oracle.bat"
)
endlocal
