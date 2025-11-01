#!/bin/bash
# Run all tests

set -e

echo "Running GliaGL Test Suite"
echo "========================="
echo ""

# Test 1: Import test
echo "[1/3] Import and Basic Functionality Test"
echo "------------------------------------------"
python python/test_import.py
echo ""

# Test 2: API test
echo "[2/3] High-Level API Test"
echo "--------------------------"
python python/test_api.py
echo ""

# Test 3: Quick start example
echo "[3/3] Quick Start Example"
echo "-------------------------"
python python/examples/quick_start.py
echo ""

echo "========================="
echo "All tests passed! ✓"
echo "========================="
