#!/bin/bash
# Local build script for testing package build

set -e

echo "========================================="
echo "GliaGL Local Build Script"
echo "========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check Python
echo -e "${YELLOW}[1/6]${NC} Checking Python..."
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Error: python3 not found${NC}"
    exit 1
fi
PYTHON_VERSION=$(python3 --version)
echo -e "${GREEN}✓${NC} Found: $PYTHON_VERSION"
echo ""

# Check CMake
echo -e "${YELLOW}[2/6]${NC} Checking CMake..."
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: cmake not found${NC}"
    echo "Install with: sudo apt install cmake"
    exit 1
fi
CMAKE_VERSION=$(cmake --version | head -n1)
echo -e "${GREEN}✓${NC} Found: $CMAKE_VERSION"
echo ""

# Clean previous builds
echo -e "${YELLOW}[3/6]${NC} Cleaning previous builds..."
rm -rf build/ dist/ *.egg-info python/build/
echo -e "${GREEN}✓${NC} Cleaned"
echo ""

# Install build dependencies
echo -e "${YELLOW}[4/6]${NC} Installing build dependencies..."
python3 -m pip install --upgrade pip build wheel
python3 -m pip install numpy pybind11 scikit-build-core
echo -e "${GREEN}✓${NC} Dependencies installed"
echo ""

# Build package
echo -e "${YELLOW}[5/6]${NC} Building package..."
python3 -m build
echo -e "${GREEN}✓${NC} Package built"
echo ""

# Install and test
echo -e "${YELLOW}[6/6]${NC} Installing and testing..."
python3 -m pip install -e .
python3 python/test_import.py

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}=========================================${NC}"
    echo -e "${GREEN}Build successful!${NC}"
    echo -e "${GREEN}=========================================${NC}"
    echo ""
    echo "Package installed in development mode."
    echo "Run tests with:"
    echo "  python python/test_import.py"
    echo "  python python/test_api.py"
else
    echo -e "${RED}Build or tests failed${NC}"
    exit 1
fi
