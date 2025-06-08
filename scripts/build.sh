#!/bin/bash

# Papers3 MicroPython Build Script
# 保持原仓库干净，仅在构建时复制必要文件

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MICROPYTHON_DIR="$PROJECT_ROOT/micropython"
EPDIY_DIR="$PROJECT_ROOT/epdiy"
PAPERS3_DIR="$PROJECT_ROOT/papers3"
BUILD_DIR="$PROJECT_ROOT/build"

echo "=== Papers3 MicroPython Build ==="
echo "Project Root: $PROJECT_ROOT"
echo "MicroPython:  $MICROPYTHON_DIR"
echo "EPDiy:        $EPDIY_DIR"
echo "Papers3:      $PAPERS3_DIR"
echo "Build:        $BUILD_DIR"

# 检查并设置ESP-IDF环境
if [ -z "$IDF_PATH" ]; then
    echo "=== Setting up ESP-IDF Environment ==="
    
    # 常见的ESP-IDF安装路径
    ESP_IDF_PATHS=(
        "$HOME/esp/esp-idf"
        "/opt/esp-idf"
        "$HOME/.espressif/esp-idf"
        "$HOME/Desktop/esp-idf"
    )
    
    for path in "${ESP_IDF_PATHS[@]}"; do
        if [ -f "$path/export.sh" ]; then
            echo "Found ESP-IDF at: $path"
            source "$path/export.sh"
            break
        fi
    done
    
    if [ -z "$IDF_PATH" ]; then
        echo "ERROR: ESP-IDF not found. Please install ESP-IDF 5.4.1 and set IDF_PATH"
        echo "Visit: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/"
        exit 1
    fi
fi

echo "ESP-IDF Path: $IDF_PATH"
echo "ESP-IDF Version: $(idf.py --version 2>/dev/null || echo 'Unknown')"

# 检查目录
if [ ! -d "$MICROPYTHON_DIR" ]; then
    echo "Error: MicroPython directory not found: $MICROPYTHON_DIR"
    exit 1
fi

if [ ! -d "$EPDIY_DIR" ]; then
    echo "Error: EPDiy directory not found: $EPDIY_DIR"
    exit 1
fi

if [ ! -d "$PAPERS3_DIR" ]; then
    echo "Error: Papers3 directory not found: $PAPERS3_DIR"
    exit 1
fi

# 清理并创建构建目录
echo "=== Preparing Build Environment ==="
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

# 复制board配置到MicroPython
BOARD_TARGET="$MICROPYTHON_DIR/ports/esp32/boards/PAPERS3"
echo "Copying Papers3 board config to: $BOARD_TARGET"

mkdir -p "$BOARD_TARGET"

# 复制所有board配置文件
for file in mpconfigboard.h mpconfigboard.cmake sdkconfig.board partitions.csv board.json; do
    if [ -f "$PAPERS3_DIR/$file" ]; then
        cp "$PAPERS3_DIR/$file" "$BOARD_TARGET/"
        echo "Copied: $file"
    else
        echo "Warning: $file not found"
    fi
done

# Papers3作为用户模块，不需要额外的复制或链接
echo "Papers3 will be built as user module from: $PAPERS3_DIR"

# 进入MicroPython ESP32目录
cd "$MICROPYTHON_DIR/ports/esp32"

echo "=== Building MicroPython with Papers3 Support ==="

# 清理之前的构建
# make BOARD=PAPERS3 clean

# 开始构建
echo "Starting build process..."
# 检测CPU核心数 (macOS使用sysctl, Linux使用nproc)
if command -v nproc >/dev/null 2>&1; then
    CORES=$(nproc)
elif command -v sysctl >/dev/null 2>&1; then
    CORES=$(sysctl -n hw.ncpu)
else
    CORES=4
fi
make BOARD=PAPERS3 -j$CORES

# 检查构建结果
if [ -f "build-PAPERS3/firmware.bin" ]; then
    # 复制固件到build目录
    cp "build-PAPERS3/firmware.bin" "$BUILD_DIR/"
    cp "build-PAPERS3/bootloader/bootloader.bin" "$BUILD_DIR/" 2>/dev/null || true
    cp "build-PAPERS3/partition_table/partition-table.bin" "$BUILD_DIR/" 2>/dev/null || true
    
    # 显示固件信息
    FIRMWARE_SIZE=$(ls -lh "build-PAPERS3/firmware.bin" | awk '{print $5}')
    echo ""
    echo "=== Build Successful ==="
    echo "Firmware size: $FIRMWARE_SIZE"
    echo "Firmware location: $BUILD_DIR/firmware.bin"
    echo ""
    
    # 显示烧写命令
    echo "=== Flash Commands ==="
    echo "清空Flash (自动检测端口):"
    echo "esptool.py erase_flash"
    echo ""
    echo "清空Flash (指定端口):"
    echo "esptool.py --port /dev/ttyUSB0 erase_flash"
    echo ""
    echo "烧写固件 (高速921600波特率，自动检测端口):"
    echo "esptool.py --baud 921600 write_flash --compress 0x0 $BUILD_DIR/firmware.bin"
    echo ""
    echo "烧写固件 (指定端口):"
    echo "esptool.py --port /dev/ttyUSB0 --baud 921600 write_flash --compress 0x0 $BUILD_DIR/firmware.bin"
    echo ""
    echo "或者使用完整烧写 (高速):"
    if [ -f "$BUILD_DIR/bootloader.bin" ] && [ -f "$BUILD_DIR/partition-table.bin" ]; then
        echo "esptool.py --baud 921600 write_flash 0x0 $BUILD_DIR/bootloader.bin 0x8000 $BUILD_DIR/partition-table.bin 0x10000 $BUILD_DIR/firmware.bin"
    fi
    echo ""
    echo "常见端口示例:"
    echo "  macOS:   /dev/cu.usbserial-* 或 /dev/cu.SLAB_USBtoUART"
    echo "  Linux:   /dev/ttyUSB0 或 /dev/ttyACM0"
    echo "  Windows: COM3, COM4, COM5 等"
    echo ""
    

    
else
    echo "=== Build Failed ==="
    echo "Firmware not found at: build-PAPERS3/firmware.bin"
    exit 1
fi

echo ""
echo "=== Build Complete ===" 