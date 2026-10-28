# Script build API server cho Virtual Pet (MinGW.org GCC 6.3.0, khong can cmake).
# Chay tu thu muc goc repo:  .\backend\build.ps1
# Tham so -Run de build xong chay luon server.
param(
    [switch]$Run,
    [int]$Port = 8080,
    [string]$BuildDir = 'backend/build',
    [string]$DatabasePath = 'backend/build/virtual_pet.db'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
$resolvedBuildDir = if ([IO.Path]::IsPathRooted($BuildDir)) {
    [IO.Path]::GetFullPath($BuildDir)
} else {
    [IO.Path]::GetFullPath((Join-Path $root $BuildDir))
}

if (-not (Test-Path $resolvedBuildDir)) {
    New-Item -ItemType Directory -Force -Path $resolvedBuildDir | Out-Null
}

# 1) Bien dich SQLite mot lan (qua wrapper co shim GetTickCount64 cho MinGW cu).
$sqliteObj = Join-Path $resolvedBuildDir 'sqlite3.o'
if (-not (Test-Path $sqliteObj)) {
    Write-Host 'Bien dich SQLite (lan dau, hoi lau)...'
    gcc -c 'backend/third_party/sqlite/sqlite3_mingw.c' -o $sqliteObj `
        -O2 -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -D_WIN32_WINNT=0x0601
    if ($LASTEXITCODE -ne 0) { throw 'Bien dich SQLite that bai' }
}

# 2) Bien dich + link server.
Write-Host 'Bien dich server...'
g++ -std=c++17 -O2 -Wall -Wextra `
        -I backend/include/Server -I backend/include/Pet -I backend/include/Exercise -I backend/third_party/sqlite `
    backend/src/server_main.cpp `
    backend/src/Server/Database.cpp `
    backend/src/Server/HttpServer.cpp `
    backend/src/Server/Crypto.cpp `
    backend/src/Server/Catalog.cpp `
    backend/src/Exercise/Submission.cpp `
    $sqliteObj `
    -o (Join-Path $resolvedBuildDir 'server.exe') -lws2_32
if ($LASTEXITCODE -ne 0) { throw 'Bien dich server that bai' }

Write-Host "Xong: $(Join-Path $resolvedBuildDir 'server.exe')"

if ($Run) {
    $databasePath = if ([IO.Path]::IsPathRooted($DatabasePath)) {
        [IO.Path]::GetFullPath($DatabasePath)
    } else {
        [IO.Path]::GetFullPath((Join-Path $root $DatabasePath))
    }
    $serverPath = Join-Path $resolvedBuildDir 'server.exe'
    Push-Location (Join-Path $root 'backend/build')
    try {
        & $serverPath $Port $databasePath
    } finally {
        Pop-Location
    }
}
