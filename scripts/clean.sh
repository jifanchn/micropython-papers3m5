#!/bin/bash

# Papers3 Clean Script
# 清理构建文件，恢复原仓库状态

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MICROPYTHON_DIR="$PROJECT_ROOT/micropython"

echo "=== Papers3 Clean ==="

# 清理MicroPython中的Papers3文件
if [ -d "$MICROPYTHON_DIR/ports/esp32/boards/PAPERS3" ]; then
    echo "Removing board config: $MICROPYTHON_DIR/ports/esp32/boards/PAPERS3"
    rm -rf "$MICROPYTHON_DIR/ports/esp32/boards/PAPERS3"
fi

# Papers3作为用户模块，无需清理额外的链接或组件

# 清理MicroPython构建文件
if [ -d "$MICROPYTHON_DIR/ports/esp32/build-PAPERS3" ]; then
    echo "Removing MicroPython build: $MICROPYTHON_DIR/ports/esp32/build-PAPERS3"
    rm -rf "$MICROPYTHON_DIR/ports/esp32/build-PAPERS3"
fi

# 清理项目构建目录
if [ -d "$PROJECT_ROOT/build" ]; then
    echo "Removing build directory: $PROJECT_ROOT/build"
    rm -rf "$PROJECT_ROOT/build"
fi

# 清理git状态（如果在git仓库中）
cd "$MICROPYTHON_DIR"
if [ -d ".git" ]; then
    echo "Restoring MicroPython git state..."
    git checkout . 2>/dev/null || true
    git clean -fd 2>/dev/null || true
fi

cd "$PROJECT_ROOT/epdiy"
if [ -d ".git" ]; then
    echo "Restoring EPDiy git state..."
    git checkout . 2>/dev/null || true
    git clean -fd 2>/dev/null || true
fi

echo "=== Clean Complete ==="
echo "All build artifacts removed, git repositories restored." 