@echo off
setlocal

set "ROOT=%~dp0."

cmake -S "%ROOT%" -B "%ROOT%\build" %*
if errorlevel 1 exit /b %errorlevel%

cmake --build "%ROOT%\build" --config Release
exit /b %errorlevel%
