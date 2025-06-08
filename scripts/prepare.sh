#!/bin/bash

# Papers3 MicroPython Project Preparation Script
# Automatically clones required repositories if they don't exist

set -e  # Exit on any error

echo "=== Papers3 MicroPython Project Preparation ==="

# Get project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
echo "Project Root: $PROJECT_ROOT"

# Change to project root
cd "$PROJECT_ROOT"

echo "=== Checking Required Repositories ==="

# Check and clone MicroPython
if [ ! -d "micropython" ]; then
    echo "📥 Cloning MicroPython v1.25.0..."
    git clone --branch v1.25.0 --depth 1 https://github.com/micropython/micropython.git
    echo "✅ MicroPython cloned successfully"
else
    echo "✅ MicroPython directory already exists"
    # Check if it's the correct branch
    cd micropython
    CURRENT_BRANCH=$(git describe --tags --exact-match HEAD 2>/dev/null || echo "unknown")
    if [ "$CURRENT_BRANCH" != "v1.25.0" ]; then
        echo "⚠️  Warning: MicroPython is not on v1.25.0 (current: $CURRENT_BRANCH)"
        echo "   You may want to checkout the correct branch:"
        echo "   cd micropython && git checkout v1.25.0"
    else
        echo "✅ MicroPython is on correct branch: v1.25.0"
    fi
    cd ..
fi

# Check and clone EPDiy
if [ ! -d "epdiy" ]; then
    echo "📥 Cloning EPDiy v2.0.0..."
    git clone --branch v2.0.0 --depth 1 https://github.com/vroland/epdiy.git
    echo "✅ EPDiy cloned successfully"
else
    echo "✅ EPDiy directory already exists"
    # Check if it's the correct branch
    cd epdiy
    CURRENT_BRANCH=$(git describe --tags --exact-match HEAD 2>/dev/null || echo "unknown")
    if [ "$CURRENT_BRANCH" != "v2.0.0" ]; then
        echo "⚠️  Warning: EPDiy is not on v2.0.0 (current: $CURRENT_BRANCH)"
        echo "   You may want to checkout the correct branch:"
        echo "   cd epdiy && git checkout v2.0.0"
    else
        echo "✅ EPDiy is on correct branch: v2.0.0"
    fi
    cd ..
fi

echo "=== Initializing MicroPython Submodules ==="
cd micropython
if [ ! -f "lib/pico-sdk/README.md" ]; then
    echo "📥 Initializing MicroPython submodules..."
    git submodule update --init
    echo "✅ MicroPython submodules initialized"
else
    echo "✅ MicroPython submodules already initialized"
fi
cd ..

echo "=== Checking ESP-IDF Installation ==="
if command -v idf.py &> /dev/null; then
    echo "✅ ESP-IDF is installed and in PATH"
    IDF_VERSION=$(idf.py --version 2>/dev/null | grep -o 'v[0-9]\+\.[0-9]\+\.[0-9]\+' || echo "unknown")
    echo "   ESP-IDF Version: $IDF_VERSION"
    if [[ "$IDF_VERSION" == "v5.4."* ]]; then
        echo "✅ ESP-IDF version is compatible (5.4.x)"
    else
        echo "⚠️  Warning: Expected ESP-IDF 5.4.x, found: $IDF_VERSION"
    fi
else
    echo "❌ ESP-IDF not found in PATH"
    echo "   Please install ESP-IDF 5.4.1 from: https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/get-started/"
    echo "   Or source the export script: source /path/to/esp-idf/export.sh"
fi

echo "=== Checking Development Tools ==="

# Check Python
if command -v python3 &> /dev/null; then
    PYTHON_VERSION=$(python3 --version 2>&1 | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' || echo "unknown")
    echo "✅ Python3 found: $PYTHON_VERSION"
else
    echo "❌ Python3 not found"
fi

# Check Git
if command -v git &> /dev/null; then
    GIT_VERSION=$(git --version | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+' || echo "unknown")
    echo "✅ Git found: $GIT_VERSION"
else
    echo "❌ Git not found"
fi

# Check esptool
if command -v esptool.py &> /dev/null; then
    echo "✅ esptool.py found"
else
    echo "⚠️  esptool.py not found (install with: pip install esptool)"
fi

echo ""
echo "=== Preparation Summary ==="
echo "📁 Project structure:"
echo "   ./micropython/     - MicroPython v1.25.0"
echo "   ./epdiy/           - EPDiy v2.0.0"
echo "   ./papers3/         - Papers3 module source"
echo "   ./scripts/         - Build and utility scripts"
echo ""
echo "🚀 Next steps:"
echo "   1. Build firmware:      ./scripts/build.sh"
echo "   2. Clean build:         ./scripts/clean.sh"
echo "   3. Flash firmware:      esptool.py write_flash -z 0x0 ./build/firmware.bin"
echo ""
echo "✅ Preparation complete!" 