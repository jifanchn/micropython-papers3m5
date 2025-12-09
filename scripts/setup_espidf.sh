#!/bin/bash

# ESP-IDF Setup Script for Papers3 MicroPython Project
# Downloads and installs ESP-IDF v5.4.1 for ESP32S3

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ESPIDF_DIR="$PROJECT_ROOT/esp-idf"
ESPIDF_VERSION="v5.4.1"

echo "=== ESP-IDF Setup Script ==="
echo "Project Root: $PROJECT_ROOT"
echo "ESP-IDF Target: $ESPIDF_DIR"
echo "ESP-IDF Version: $ESPIDF_VERSION"
echo ""

# Check if ESP-IDF already exists
if [ -d "$ESPIDF_DIR" ]; then
    echo "⚠️  ESP-IDF directory already exists at: $ESPIDF_DIR"
    echo "To reinstall, delete the directory first:"
    echo "  rm -rf $ESPIDF_DIR"
    echo ""
    echo "To use existing installation, source the environment:"
    echo "  source $ESPIDF_DIR/export.sh"
    exit 0
fi

echo "📥 Cloning ESP-IDF $ESPIDF_VERSION..."
echo "This may take 10-20 minutes depending on your network speed."
echo ""

# Clone ESP-IDF
git clone -b "$ESPIDF_VERSION" --depth 1 https://github.com/espressif/esp-idf.git "$ESPIDF_DIR"

echo ""
echo "📥 Initializing ESP-IDF submodules..."
cd "$ESPIDF_DIR"
git submodule update --init --recursive --depth 1

echo ""
echo "🔧 Installing ESP-IDF tools for ESP32S3..."
./install.sh esp32s3

echo ""
echo "=== ESP-IDF Installation Complete ==="
echo ""
echo "✅ ESP-IDF $ESPIDF_VERSION installed to: $ESPIDF_DIR"
echo ""
echo "🚀 Next steps:"
echo "1. Source the ESP-IDF environment:"
echo "   source $ESPIDF_DIR/export.sh"
echo ""
echo "2. Build the firmware:"
echo "   cd $PROJECT_ROOT"
echo "   ./scripts/build.sh"
echo ""
echo "3. Flash to device:"
echo "   ./scripts/flash.sh -e"
echo ""
echo "💡 Tip: Add this to your shell profile (.bashrc or .zshrc) for convenience:"
echo "   alias idf-papers3='source $ESPIDF_DIR/export.sh && cd $PROJECT_ROOT'"
