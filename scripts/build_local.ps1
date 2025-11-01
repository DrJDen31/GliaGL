# PowerShell build script for Windows

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "GliaGL Local Build Script (Windows)" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""

# Check Python
Write-Host "[1/6] Checking Python..." -ForegroundColor Yellow
try {
    $pythonVersion = python --version 2>&1
    Write-Host "✓ Found: $pythonVersion" -ForegroundColor Green
} catch {
    Write-Host "Error: Python not found" -ForegroundColor Red
    exit 1
}
Write-Host ""

# Check CMake
Write-Host "[2/6] Checking CMake..." -ForegroundColor Yellow
try {
    $cmakeVersion = cmake --version 2>&1 | Select-Object -First 1
    Write-Host "✓ Found: $cmakeVersion" -ForegroundColor Green
} catch {
    Write-Host "Error: CMake not found" -ForegroundColor Red
    Write-Host "Download from: https://cmake.org/download/" -ForegroundColor Yellow
    exit 1
}
Write-Host ""

# Clean previous builds
Write-Host "[3/6] Cleaning previous builds..." -ForegroundColor Yellow
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue build, dist, *.egg-info, python\build
Write-Host "✓ Cleaned" -ForegroundColor Green
Write-Host ""

# Install build dependencies
Write-Host "[4/6] Installing build dependencies..." -ForegroundColor Yellow
python -m pip install --upgrade pip build wheel
python -m pip install numpy pybind11 scikit-build-core
Write-Host "✓ Dependencies installed" -ForegroundColor Green
Write-Host ""

# Build package
Write-Host "[5/6] Building package..." -ForegroundColor Yellow
python -m build
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed" -ForegroundColor Red
    exit 1
}
Write-Host "✓ Package built" -ForegroundColor Green
Write-Host ""

# Install and test
Write-Host "[6/6] Installing and testing..." -ForegroundColor Yellow
python -m pip install -e .
python python\test_import.py

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=========================================" -ForegroundColor Green
    Write-Host "Build successful!" -ForegroundColor Green
    Write-Host "=========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "Package installed in development mode."
    Write-Host "Run tests with:"
    Write-Host "  python python\test_import.py"
    Write-Host "  python python\test_api.py"
} else {
    Write-Host "Build or tests failed" -ForegroundColor Red
    exit 1
}
