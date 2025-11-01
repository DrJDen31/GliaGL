# Repository cleanup script for Stage 6 (PowerShell)
# This script removes obsolete CLI-era files now that Python API is complete

Write-Host "=========================================" -ForegroundColor Cyan
Write-Host "GliaGL Repository Cleanup - Stage 6" -ForegroundColor Cyan
Write-Host "=========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "This script will:"
Write-Host "  1. Remove obsolete C++ CLI executables"
Write-Host "  2. Remove old evaluator C++ files"
Write-Host "  3. Remove old build scripts"
Write-Host "  4. Clear build directory contents"
Write-Host "  5. List files that would be affected"
Write-Host ""

# Check if we're in the right directory
if (-not (Test-Path "PYTHON_OVERHAUL_PLAN.md")) {
    Write-Host "Error: Must run from GliaGL root directory" -ForegroundColor Red
    exit 1
}

# Function to safely remove file/directory
function Safe-Remove {
    param($Path)
    if (Test-Path $Path) {
        Write-Host "Would remove: $Path" -ForegroundColor Yellow
        # Uncomment to actually delete:
        # Remove-Item -Recurse -Force $Path
        # Write-Host "Removed: $Path" -ForegroundColor Green
    }
}

Write-Host "[1/5] Checking obsolete C++ CLI executables..." -ForegroundColor Yellow
Write-Host ""

# Main training executables (replaced by Python)
Safe-Remove "src\train\eval_main.cpp"
Safe-Remove "src\train\mini_world_main.cpp"

Write-Host ""
Write-Host "[2/5] Checking old evaluator C++ files..." -ForegroundColor Yellow
Write-Host ""

# 3class evaluator files
if (Test-Path "examples\3class\evaluator") {
    Write-Host "  Found: examples\3class\evaluator\"
    Get-ChildItem "examples\3class\evaluator\*.cpp" -ErrorAction SilentlyContinue | ForEach-Object {
        Safe-Remove $_.FullName
    }
    Get-ChildItem "examples\3class\evaluator\*.h" -ErrorAction SilentlyContinue | ForEach-Object {
        Safe-Remove $_.FullName
    }
}

# mini-world evaluator files
if (Test-Path "examples\mini-world\evaluator") {
    Write-Host "  Found: examples\mini-world\evaluator\"
    Get-ChildItem "examples\mini-world\evaluator\*.cpp" -ErrorAction SilentlyContinue | ForEach-Object {
        Safe-Remove $_.FullName
    }
    Get-ChildItem "examples\mini-world\evaluator\*.h" -ErrorAction SilentlyContinue | ForEach-Object {
        Safe-Remove $_.FullName
    }
}

Write-Host ""
Write-Host "[3/5] Checking old build scripts..." -ForegroundColor Yellow
Write-Host ""

# Old build/run scripts in examples
Get-ChildItem "examples\3class\*.sh" -ErrorAction SilentlyContinue | ForEach-Object {
    Safe-Remove $_.FullName
}
Get-ChildItem "examples\3class\*.ps1" -ErrorAction SilentlyContinue | ForEach-Object {
    Safe-Remove $_.FullName
}
Get-ChildItem "examples\mini-world\*.ps1" -ErrorAction SilentlyContinue | ForEach-Object {
    Safe-Remove $_.FullName
}

Write-Host "  (Keeping scripts\build_local.* - these are new)"

Write-Host ""
Write-Host "[4/5] Checking build directory contents..." -ForegroundColor Yellow
Write-Host ""

# Clear build artifacts (keep directories)
if (Test-Path "build") {
    Write-Host "  build\ exists"
    Write-Host "  (Would clear contents)"
    # Uncomment to clear:
    # Get-ChildItem "build\*" -Recurse | Remove-Item -Recurse -Force
}

if (Test-Path "src\train\build") {
    Write-Host "  src\train\build\ exists"
    Write-Host "  (Would clear contents)"
    # Uncomment to clear:
    # Get-ChildItem "src\train\build\*" -Recurse | Remove-Item -Recurse -Force
}

if (Test-Path "src\vis\build") {
    Write-Host "  src\vis\build\ exists"
    Write-Host "  (Would clear contents)"
    # Uncomment to clear:
    # Get-ChildItem "src\vis\build\*" -Recurse | Remove-Item -Recurse -Force
}

Write-Host ""
Write-Host "[5/5] Summary..." -ForegroundColor Yellow
Write-Host ""

Write-Host "Files that would be removed:"
Write-Host "  - Obsolete C++ CLI main files"
Write-Host "  - Old evaluator C++ implementations"
Write-Host "  - Old build/run shell scripts"
Write-Host "  - Build directory contents (not dirs)"
Write-Host ""

Write-Host "Files that are KEPT:"
Write-Host "  ✓ All core C++ implementation (src\arch\, src\train\, src\evo\)"
Write-Host "  ✓ All network files (.net)"
Write-Host "  ✓ All sequence files (.seq)"
Write-Host "  ✓ All Python code (python\)"
Write-Host "  ✓ All new infrastructure (CI/CD, docs, examples)"
Write-Host "  ✓ Build directories (empty)"
Write-Host ""

Write-Host "=========================================" -ForegroundColor Green
Write-Host "DRY RUN COMPLETE" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor Green
Write-Host ""
Write-Host "This was a dry run. No files were deleted."
Write-Host ""
Write-Host "To actually perform cleanup:"
Write-Host "  1. Review CLEANUP_AUDIT.md"
Write-Host "  2. Edit this script and uncomment Remove-Item commands"
Write-Host "  3. Run again: .\scripts\cleanup_repository.ps1"
Write-Host ""
Write-Host "Or perform cleanup manually based on audit."
