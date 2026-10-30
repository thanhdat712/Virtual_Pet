@echo off
setlocal

set "ROOT=%~dp0"
set "BUILD_DIR=%ROOT%build\full-run"
set "PORT=%~1"
if "%PORT%"=="" set "PORT=8080"

rem Reuse a server that is already responding instead of launching a duplicate.
powershell -NoProfile -Command "try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:%PORT%/' -UseBasicParsing -TimeoutSec 2; if ($r.StatusCode -eq 200) { exit 0 } } catch {}; exit 1" >nul 2>nul
if not errorlevel 1 (
    echo Virtual Pet is already running at http://127.0.0.1:%PORT%/
    start "" "http://127.0.0.1:%PORT%/"
    exit /b 0
)

where cmake >nul 2>nul
if errorlevel 1 (
    echo CMake was not found. Install CMake and a C++17 toolchain first.
    pause
    exit /b 1
)

if not exist "%BUILD_DIR%\build.ninja" (
    cmake -S "%ROOT%" -B "%BUILD_DIR%"
    if errorlevel 1 (
        echo CMake configuration failed.
        pause
        exit /b 1
    )
)

cmake --build "%BUILD_DIR%" --config Release --parallel
if errorlevel 1 (
    echo Project build failed.
    pause
    exit /b 1
)

rem Start the server in its own console and wait until the page is available.
start "Virtual Pet Server" "%BUILD_DIR%\Pet_Raising_Server.exe" "%PORT%" "%BUILD_DIR%\virtual_pet.db"

set "SERVER_READY="
for /l %%I in (1,1,30) do (
    powershell -NoProfile -Command "try { $r = Invoke-WebRequest -Uri 'http://127.0.0.1:%PORT%/' -UseBasicParsing -TimeoutSec 2; if ($r.StatusCode -eq 200) { exit 0 } } catch {}; exit 1" >nul 2>nul
    if not errorlevel 1 (
        set "SERVER_READY=1"
        goto :server_ready
    )
    timeout /t 1 /nobreak >nul
)

echo The server did not respond on port %PORT%.
echo Check the Virtual Pet Server window for an error, or try another port:
echo     run.bat 8083
pause
exit /b 1

:server_ready
start "" "http://127.0.0.1:%PORT%/"
exit /b 0
