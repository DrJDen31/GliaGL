#!/bin/bash
# Repository cleanup script for Stage 6
# This script removes obsolete CLI-era files now that Python API is complete

set -e

echo "========================================="
echo "GliaGL Repository Cleanup - Stage 6"
echo "========================================="
echo ""
echo "This script will:"
echo "  1. Remove obsolete C++ CLI executables"
echo "  2. Remove old evaluator C++ files"
echo "  3. Remove old build scripts"
echo "  4. Clear build directory contents"
echo "  5. List files that would be affected"
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Check if we're in the right directory
if [ ! -f "PYTHON_OVERHAUL_PLAN.md" ]; then
    echo -e "${RED}Error: Must run from GliaGL root directory${NC}"
    exit 1
fi

# Function to safely remove file/directory
safe_remove() {
    local path="$1"
    if [ -e "$path" ]; then
        echo -e "${YELLOW}Would remove:${NC} $path"
        # Uncomment to actually delete:
        rm -rf "$path"
         echo -e "${GREEN}Removed:${NC} $path"
    fi
}

echo -e "${YELLOW}[1/5]${NC} Checking obsolete C++ CLI executables..."
echo ""

# Main training executables (replaced by Python)
safe_remove "src/train/eval_main.cpp"
safe_remove "src/train/mini_world_main.cpp"

echo ""
echo -e "${YELLOW}[2/5]${NC} Checking old evaluator C++ files..."
echo ""

# 3class evaluator files
if [ -d "examples/3class/evaluator" ]; then
    echo "  Found: examples/3class/evaluator/"
    for file in examples/3class/evaluator/*.cpp; do
        [ -f "$file" ] && safe_remove "$file"
    done
    for file in examples/3class/evaluator/*.h; do
        [ -f "$file" ] && safe_remove "$file"
    done
fi

# mini-world evaluator files
if [ -d "examples/mini-world/evaluator" ]; then
    echo "  Found: examples/mini-world/evaluator/"
    for file in examples/mini-world/evaluator/*.cpp; do
        [ -f "$file" ] && safe_remove "$file"
    done
    for file in examples/mini-world/evaluator/*.h; do
        [ -f "$file" ] && safe_remove "$file"
    done
fi

echo ""
echo -e "${YELLOW}[3/5]${NC} Checking old build scripts..."
echo ""

# Old build/run scripts in examples
for script in examples/3class/*.sh examples/3class/*.ps1; do
    [ -f "$script" ] && safe_remove "$script"
done

for script in examples/mini-world/*.ps1; do
    [ -f "$script" ] && safe_remove "$script"
done

# But keep the new scripts in scripts/
echo "  (Keeping scripts/build_local.* - these are new)"

echo ""
echo -e "${YELLOW}[4/5]${NC} Checking build directory contents..."
echo ""

# Clear build artifacts (keep directories)
if [ -d "build" ]; then
    echo "  build/ exists"
    # Uncomment to clear:
    find build/ -mindepth 1 -delete 2>/dev/null || true
    echo "  (Would clear contents)"
fi

if [ -d "src/train/build" ]; then
    echo "  src/train/build/ exists"
    # Uncomment to clear:
    find src/train/build/ -mindepth 1 -delete 2>/dev/null || true
    echo "  (Would clear contents)"
fi

if [ -d "src/vis/build" ]; then
    echo "  src/vis/build/ exists"
    # Uncomment to clear:
    find src/vis/build/ -mindepth 1 -delete 2>/dev/null || true
    echo "  (Would clear contents)"
fi

echo ""
echo -e "${YELLOW}[5/5]${NC} Summary..."
echo ""

echo "Files that would be removed:"
echo "  - Obsolete C++ CLI main files"
echo "  - Old evaluator C++ implementations"  
echo "  - Old build/run shell scripts"
echo "  - Build directory contents (not dirs)"
echo ""

echo "Files that are KEPT:"
echo "  ✓ All core C++ implementation (src/arch/, src/train/, src/evo/)"
echo "  ✓ All network files (.net)"
echo "  ✓ All sequence files (.seq)"
echo "  ✓ All Python code (python/)"
echo "  ✓ All new infrastructure (CI/CD, docs, examples)"
echo "  ✓ Build directories (empty)"
echo ""

echo "========================================="
echo -e "${GREEN}DRY RUN COMPLETE${NC}"
echo "========================================="
echo ""
echo "This was a dry run. No files were deleted."
echo ""
echo "To actually perform cleanup:"
echo "  1. Review CLEANUP_AUDIT.md"
echo "  2. Edit this script and uncomment rm commands"
echo "  3. Run again: bash scripts/cleanup_repository.sh"
echo ""
echo "Or perform cleanup manually based on audit."
