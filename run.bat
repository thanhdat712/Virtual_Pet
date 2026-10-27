@echo off
setlocal

set "ROOT=%~dp0"
set "BUILD_DIR=%ROOT%build\full-run"
set "PORT=%~1"
if "%PORT%"=="" set "PORT=8080"

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

start "" "http://127.0.0.1:%PORT%/"
pushd "%ROOT%"
"%BUILD_DIR%\Pet_Raising_Server.exe" "%PORT%" "%BUILD_DIR%\virtual_pet.db"
set "SERVER_EXIT=%ERRORLEVEL%"
popd

if not "%SERVER_EXIT%"=="0" pause
exit /b %SERVER_EXIT%
