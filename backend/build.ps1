# Script build API server cho Virtual Pet (MinGW.org GCC 6.3.0, khong can cmake).
# Chay tu thu muc goc repo:  .\backend\build.ps1
# Tham so -Run de build xong chay luon server.
param(
    [switch]$Run,
    [int]$Port = 8080,
    [string]$BuildDir = 'backend/build'
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
}

# 1) Bien dich SQLite mot lan (qua wrapper co shim GetTickCount64 cho MinGW cu).
$sqliteObj = "$buildDir/sqlite3.o"
if (-not (Test-Path $sqliteObj)) {
    Write-Host 'Bien dich SQLite (lan dau, hoi lau)...'
    gcc -c 'backend/third_party/sqlite/sqlite3_mingw.c' -o $sqliteObj `
        -O2 -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -D_WIN32_WINNT=0x0601
    if ($LASTEXITCODE -ne 0) { throw 'Bien dich SQLite that bai' }
}

# 2) Bien dich + link server.
Write-Host 'Bien dich server...'
g++ -std=c++17 -O2 -Wall -Wextra `
    -I backend/include/Server -I backend/third_party/sqlite `
    backend/src/server_main.cpp `
    backend/src/Server/Database.cpp `
    backend/src/Server/HttpServer.cpp `
    backend/src/Server/Crypto.cpp `
    backend/src/Server/Catalog.cpp `
    $sqliteObj `
    -o "$buildDir/server.exe" -lws2_32
if ($LASTEXITCODE -ne 0) { throw 'Bien dich server that bai' }

Write-Host "Xong: $buildDir/server.exe"

if ($Run) {
    Push-Location $buildDir
    try {
        .\server.exe $Port
    } finally {
        Pop-Location
    }
}
