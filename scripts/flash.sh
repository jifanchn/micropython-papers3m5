#!/bin/bash

# Papers3 MicroPython Flash Script
# 便捷的固件烧写脚本，支持自动检测端口和高速烧写

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
FIRMWARE_FILE="$BUILD_DIR/firmware.bin"
BOOTLOADER_FILE="$BUILD_DIR/bootloader.bin"
PARTITION_FILE="$BUILD_DIR/partition-table.bin"

# 默认参数
PORT=""
BAUD_RATE="921600"
ERASE_FLASH=false
VERBOSE=false
FULL_FLASH=false

# 显示帮助信息
show_help() {
    echo "Papers3 MicroPython Flash Script"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -p, --port PORT     指定串口 (如: /dev/ttyUSB0, COM3)"
    echo "  -b, --baud RATE     设置波特率 (默认: 921600)"
    echo "  -e, --erase         烧写前清空Flash"
    echo "  -f, --full          完整烧写 (bootloader + 分区表 + 固件)"
    echo "  -v, --verbose       显示详细信息"
    echo "  -h, --help          显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  $0                              # 自动检测端口，高速烧写"
    echo "  $0 -p /dev/ttyUSB0              # 指定端口烧写"
    echo "  $0 -e                           # 清空Flash后烧写"
    echo "  $0 -f                           # 完整烧写 (解决启动问题)"
    echo "  $0 -p COM3 -b 460800 -e -f     # Windows端口，中速，完整烧写"
    echo ""
    echo "常见端口:"
    echo "  macOS:   /dev/cu.usbserial-* 或 /dev/cu.SLAB_USBtoUART"
    echo "  Linux:   /dev/ttyUSB0 或 /dev/ttyACM0"
    echo "  Windows: COM3, COM4, COM5 等"
    echo ""
    echo "测试固件:"
    echo "  连接串口终端 (115200波特率) 后运行:"
    echo "    >>> import papers3"
    echo "    >>> test = papers3.test()"
    echo "    >>> test.help()        # 查看所有测试功能"
    echo "    >>> test.init()        # 初始化硬件"
    echo "    >>> test.basic()       # 运行基础测试"
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -b|--baud)
            BAUD_RATE="$2"
            shift 2
            ;;
        -e|--erase)
            ERASE_FLASH=true
            shift
            ;;
        -f|--full)
            FULL_FLASH=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo "未知选项: $1"
            echo "使用 -h 或 --help 查看帮助"
            exit 1
            ;;
    esac
done

# 检查固件文件
if [ ! -f "$FIRMWARE_FILE" ]; then
    echo "错误: 固件文件不存在: $FIRMWARE_FILE"
    echo "请先运行 ./scripts/build.sh 构建固件"
    exit 1
fi

# 检查esptool.py
if ! command -v esptool.py &> /dev/null; then
    echo "错误: esptool.py 未找到"
    echo "请安装 esptool: pip install esptool"
    exit 1
fi

# 构建esptool命令
ESPTOOL_CMD="esptool.py"

# 端口检测和选择 (优化版本，来自download.sh)
detect_and_select_port() {
    local ports=()
    
    # 检测各平台的串口设备
    echo "正在扫描可用串口..."
    
    # macOS 端口 (排在前面)
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # USB转串口设备 (优先级最高)
        while IFS= read -r -d '' port; do
            ports+=("$port")
        done < <(find /dev -name "cu.usbserial*" -print0 2>/dev/null | sort -z)
        
        while IFS= read -r -d '' port; do
            ports+=("$port")
        done < <(find /dev -name "cu.SLAB_USBtoUART*" -print0 2>/dev/null | sort -z)
        
        while IFS= read -r -d '' port; do
            ports+=("$port")
        done < <(find /dev -name "cu.wchusbserial*" -print0 2>/dev/null | sort -z)
        
        # 其他USB设备
        while IFS= read -r -d '' port; do
            ports+=("$port")
        done < <(find /dev -name "cu.usb*" -print0 2>/dev/null | sort -z)
    fi
    
    # Linux 端口
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        # USB转串口设备 (优先级最高)
        for port in /dev/ttyUSB* /dev/ttyACM*; do
            [ -e "$port" ] && ports+=("$port")
        done
        
        # 其他串口设备
        for port in /dev/ttyS*; do
            [ -e "$port" ] && ports+=("$port")
        done
    fi
    
    # Windows 端口 (通过模式匹配)
    if [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        # COM端口 (Windows)
        for i in {1..20}; do
            port="COM$i"
            # 简单测试端口是否存在
            if [ -e "$port" ] 2>/dev/null; then
                ports+=("$port")
            fi
        done
    fi
    
    # 如果没有检测到端口
    if [ ${#ports[@]} -eq 0 ]; then
        echo "未检测到可用串口设备"
        echo "请手动指定端口: $0 -p PORT"
        echo ""
        echo "常见端口:"
        echo "  macOS:   /dev/cu.usbserial-* 或 /dev/cu.SLAB_USBtoUART"
        echo "  Linux:   /dev/ttyUSB0 或 /dev/ttyACM0"
        echo "  Windows: COM3, COM4, COM5 等"
        exit 1
    fi
    
    # 如果只有一个端口，直接使用
    if [ ${#ports[@]} -eq 1 ]; then
        PORT="${ports[0]}"
        echo "自动选择唯一端口: $PORT"
        return
    fi
    
    # 多个端口，让用户选择
    echo ""
    echo "检测到多个串口设备，请选择:"
    echo ""
    for i in "${!ports[@]}"; do
        printf "%2d) %s\n" $((i+1)) "${ports[$i]}"
    done
    echo ""
    
    while true; do
        read -p "请选择端口 (1-${#ports[@]}): " choice
        
        # 检查输入是否为数字
        if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le ${#ports[@]} ]; then
            PORT="${ports[$((choice-1))]}"
            echo "已选择: $PORT"
            break
        else
            echo "无效选择，请输入 1-${#ports[@]} 之间的数字"
        fi
    done
}

# 添加端口参数
if [ -n "$PORT" ]; then
    echo "使用指定端口: $PORT"
else
    detect_and_select_port
fi

ESPTOOL_CMD="$ESPTOOL_CMD --port $PORT"

# 显示固件信息
FIRMWARE_SIZE=$(ls -lh "$FIRMWARE_FILE" | awk '{print $5}')
echo "固件文件: $FIRMWARE_FILE"
echo "固件大小: $FIRMWARE_SIZE"
echo "波特率: $BAUD_RATE"

# 清空Flash (如果需要)
if [ "$ERASE_FLASH" = true ]; then
    echo ""
    echo "=== 清空Flash ==="
    if [ "$VERBOSE" = true ]; then
        $ESPTOOL_CMD erase_flash
    else
        $ESPTOOL_CMD erase_flash > /dev/null 2>&1
    fi
    echo "Flash已清空"
    echo "等待设备稳定..."
    sleep 1
fi

# 烧写固件
echo ""
echo "=== 烧写固件 ==="
echo "开始烧写..."

if [ "$FULL_FLASH" = true ]; then
    # 完整烧写 (bootloader + 分区表 + 固件)
    if [ ! -f "$BOOTLOADER_FILE" ] || [ ! -f "$PARTITION_FILE" ]; then
        echo "警告: 缺少bootloader或分区表文件，降级为仅烧写固件"
        FLASH_CMD="$ESPTOOL_CMD --baud $BAUD_RATE write_flash --compress 0x0 $FIRMWARE_FILE"
    else
        echo "使用完整烧写模式 (bootloader + 分区表 + 固件)"
        FLASH_CMD="$ESPTOOL_CMD --baud $BAUD_RATE write_flash 0x0 $BOOTLOADER_FILE 0x8000 $PARTITION_FILE 0x10000 $FIRMWARE_FILE"
    fi
else
    # 仅烧写固件
    FLASH_CMD="$ESPTOOL_CMD --baud $BAUD_RATE write_flash --compress 0x0 $FIRMWARE_FILE"
fi

if [ "$VERBOSE" = true ]; then
    echo "执行命令: $FLASH_CMD"
    $FLASH_CMD
else
    $FLASH_CMD > /dev/null 2>&1
fi

echo "固件烧写完成！"
echo ""
echo "=== 下一步 ==="
echo "1. 重启设备"
echo "2. 连接串口终端 (115200波特率)"
echo "3. 运行测试代码:"
echo "   >>> import papers3"
echo "   >>> test = papers3.test()"
echo "   >>> test.help()        # 查看所有测试功能"