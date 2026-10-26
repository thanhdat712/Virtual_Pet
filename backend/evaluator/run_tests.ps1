$ErrorActionPreference = "Stop"
Write-Host "=== Virtual Pet verification ==="

if (Get-Command cmake -ErrorAction SilentlyContinue) {
    cmake -S . -B build
    cmake --build build --config Debug
    ctest --test-dir build -C Debug --output-on-failure
} else {
    Write-Warning "CMake not found; skip C++ build."
}

if (Get-Command python -ErrorAction SilentlyContinue) {
    python -m unittest discover -s backend/evaluator/tests -v
} else {
    Write-Warning "Python not found; skip evaluator tests."
}

Write-Host "Verification finished."
