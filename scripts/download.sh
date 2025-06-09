#!/bin/bash

# Papers3 Demo Files Download Script
# 自动下载demo目录中的Python文件到ESP32设备

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEMO_DIR="$PROJECT_ROOT/demo"

# 默认参数
PORT=""
BAUD_RATE="115200"
VERBOSE=false

# 显示帮助信息
show_help() {
    echo "Papers3 Demo Files Download Script"
    echo ""
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -p, --port PORT     指定串口 (如: /dev/ttyUSB0, COM3)"
    echo "  -b, --baud RATE     设置波特率 (默认: 115200)"
    echo "  -v, --verbose       显示详细信息"
    echo "  -h, --help          显示此帮助信息"
    echo ""
    echo "功能:"
    echo "  - 自动检测可用串口设备"
    echo "  - 扫描demo目录中的.py文件"
    echo "  - 支持选择单个文件或全部下载"
    echo "  - 自动检测设备连接状态"
    echo ""
    echo "示例:"
    echo "  $0                              # 自动检测端口和文件"
    echo "  $0 -p /dev/ttyUSB0              # 指定端口下载"
    echo "  $0 -v                           # 显示详细下载信息"
    echo ""
    echo "常见端口:"
    echo "  macOS:   /dev/cu.usbserial-* 或 /dev/cu.SLAB_USBtoUART"
    echo "  Linux:   /dev/ttyUSB0 或 /dev/ttyACM0"
    echo "  Windows: COM3, COM4, COM5 等"
    echo ""
    echo "下载后使用:"
    echo "  连接到ESP32 REPL后运行:"
    echo "    >>> import screen_range_test as srt"
    echo "    >>> srt.help()                     # 查看帮助"
    echo "    >>> srt.demo_quick()               # 快速演示"
    echo "    >>> srt.demo_interactive()         # 交互式演示"
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

# 检查demo目录
if [ ! -d "$DEMO_DIR" ]; then
    echo "错误: demo目录不存在: $DEMO_DIR"
    exit 1
fi

# 检查ampy工具
if ! command -v ampy &> /dev/null; then
    echo "错误: ampy 工具未找到"
    echo "请安装 ampy: pip install adafruit-ampy"
    echo ""
    echo "安装后重新运行此脚本"
    exit 1
fi

# 端口检测和选择 (复用flash.sh的逻辑)
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



# 测试ESP32连接
test_esp32_connection() {
    echo "正在测试ESP32连接..."
    
    if [ "$VERBOSE" = true ]; then
        ampy -p "$PORT" -b "$BAUD_RATE" ls
    else
        ampy -p "$PORT" -b "$BAUD_RATE" ls > /dev/null 2>&1
    fi
    
    if [ $? -eq 0 ]; then
        echo "✅ ESP32连接测试成功"
        return 0
    else
        echo "❌ ESP32连接测试失败"
        echo "请检查:"
        echo "  1. ESP32是否正确连接到电脑"
        echo "  2. 端口 $PORT 是否正确"
        echo "  3. ESP32是否已烧录MicroPython固件"
        echo "  4. 没有其他程序占用串口"
        return 1
    fi
}

# 下载文件到ESP32
download_file() {
    local filename="$1"
    local file_path="$DEMO_DIR/$filename"
    
    if [ ! -f "$file_path" ]; then
        echo "❌ 文件不存在: $file_path"
        return 1
    fi
    
    echo "📤 下载文件: $filename"
    
    if [ "$VERBOSE" = true ]; then
        echo "执行命令: ampy -p $PORT -b $BAUD_RATE put $file_path $filename"
        ampy -p "$PORT" -b "$BAUD_RATE" put "$file_path" "$filename"
    else
        ampy -p "$PORT" -b "$BAUD_RATE" put "$file_path" "$filename" > /dev/null 2>&1
    fi
    
    if [ $? -eq 0 ]; then
        echo "✅ $filename 下载成功"
        return 0
    else
        echo "❌ $filename 下载失败"
        return 1
    fi
}

# 验证下载结果
verify_downloads() {
    # 将传入的字符串转换为数组
    local files_str="$1"
    local files=()
    IFS=' ' read -ra files <<< "$files_str"
    
    echo ""
    echo "=== 验证下载结果 ==="
    
    echo "ESP32上的文件列表:"
    if [ "$VERBOSE" = true ]; then
        ampy -p "$PORT" -b "$BAUD_RATE" ls
    else
        ampy -p "$PORT" -b "$BAUD_RATE" ls 2>/dev/null
    fi
    
    echo ""
    echo "验证下载的文件:"
    local success_count=0
    for file in "${files[@]}"; do
        if ampy -p "$PORT" -b "$BAUD_RATE" ls | grep -q "^/$file$" 2>/dev/null; then
            echo "✅ $file - 存在"
            ((success_count++))
        else
            echo "❌ $file - 未找到"
        fi
    done
    
    echo ""
    echo "下载完成: $success_count/${#files[@]} 个文件成功"
}

# 主程序流程
main() {
    echo "Papers3 Demo Files Download Script"
    echo "=================================="
    
    # 1. 端口选择
    if [ -n "$PORT" ]; then
        echo "使用指定端口: $PORT"
    else
        detect_and_select_port
    fi
    
    # 2. 扫描文件
    echo "正在扫描demo目录中的Python文件..."
    
    local files_array=()
    while IFS= read -r -d '' file; do
        files_array+=("$(basename "$file")")
    done < <(find "$DEMO_DIR" -name "*.py" -type f -print0 | sort -z)
    
    if [ ${#files_array[@]} -eq 0 ]; then
        echo "demo目录中没有找到Python文件"
        exit 1
    fi
    
    echo "找到 ${#files_array[@]} 个Python文件:"
    for file in "${files_array[@]}"; do
        echo "  - $file"
    done
    
    # 3. 选择文件
    echo ""
    echo "请选择要下载的文件:"
    echo ""
    printf "%2d) %s\n" 0 "全部下载"
    for i in "${!files_array[@]}"; do
        file="${files_array[$i]}"
        file_path="$DEMO_DIR/$file"
        if [ -f "$file_path" ]; then
            file_size=$(ls -lh "$file_path" 2>/dev/null | awk '{print $5}')
            printf "%2d) %-25s (%s)\n" $((i+1)) "$file" "$file_size"
        else
            printf "%2d) %-25s\n" $((i+1)) "$file"
        fi
    done
    echo ""
    
    local selected_files_array=()
    while true; do
        read -p "请选择 (0=全部, 1-${#files_array[@]}=单个文件): " choice
        
        if [[ "$choice" =~ ^[0-9]+$ ]]; then
            if [ "$choice" -eq 0 ]; then
                # 全部下载
                selected_files_array=("${files_array[@]}")
                echo "已选择: 全部文件 (${#files_array[@]}个)"
                break
            elif [ "$choice" -ge 1 ] && [ "$choice" -le ${#files_array[@]} ]; then
                # 单个文件
                selected_files_array=("${files_array[$((choice-1))]}")
                echo "已选择: ${selected_files_array[0]}"
                break
            else
                echo "无效选择，请输入 0-${#files_array[@]} 之间的数字"
            fi
        else
            echo "无效输入，请输入数字"
        fi
    done
    
    # 4. 测试连接
    if ! test_esp32_connection; then
        echo ""
        echo "连接测试失败，是否继续尝试下载? (y/N)"
        read -p "> " continue_download
        if [[ ! "$continue_download" =~ ^[Yy]$ ]]; then
            echo "下载已取消"
            exit 1
        fi
    fi
    
    # 5. 下载文件
    echo ""
    echo "=== 开始下载文件 ==="
    echo "端口: $PORT"
    echo "波特率: $BAUD_RATE"
    echo "文件数量: ${#selected_files_array[@]}"
    echo ""
    
    local success_count=0
    for file in "${selected_files_array[@]}"; do
        if download_file "$file"; then
            ((success_count++))
        fi
        echo ""
    done
    
    # 6. 验证结果
    verify_downloads "${selected_files_array[*]}"
    
    # 7. 完成提示
    echo ""
    echo "=== 下载完成 ==="
    if [ $success_count -eq ${#selected_files_array[@]} ]; then
        echo "🎉 所有文件下载成功！"
    else
        echo "⚠️  部分文件下载失败 ($success_count/${#selected_files_array[@]})"
    fi
    
    echo ""
    echo "下一步:"
    echo "1. 连接到ESP32 REPL:"
    echo "   picocom $PORT -b 115200"
    echo ""
    echo "2. 运行测试代码:"
    echo "   >>> import screen_range_test as srt"
    echo "   >>> srt.help()                     # 查看帮助"
    echo "   >>> srt.demo_quick()               # 快速演示"
    echo "   >>> srt.demo_interactive()         # 交互式演示"
}

# 运行主程序 (仅在直接执行时运行)
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main
fi 