# Papers3 Module Manifest
# This file defines which Python files should be frozen into the firmware

# VFS 文件系统支持模块（按照标准MicroPython ESP32结构）
freeze(".", "_boot.py")       # 启动时自动执行的VFS挂载脚本
freeze(".", "flashbdev.py")   # Flash块设备检测
freeze(".", "inisetup.py")    # 文件系统初始化
