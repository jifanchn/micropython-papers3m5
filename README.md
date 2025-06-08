# MicroPython Papers3M5 项目

为[M5Stack Papers3](https://docs.m5stack.com/zh_CN/core/papers3)硬件平台创建的MicroPython支持包，提供完整的嵌入式开发环境。

## 硬件平台

**目标设备**: [M5Stack Papers3 (SKU:C139)](https://docs.m5stack.com/zh_CN/core/papers3)
- **官方链接**: https://docs.m5stack.com/zh_CN/core/papers3
- **产品特性**: 4.7寸触控电子墨水屏主控设备
- **核心控制器**: ESP32-S3R8 SoC (Xtensa® 32位 LX7双核处理器，240MHz)

## 项目配置

- **MicroPython**: v1.25.0
- **EPDiy**: v2.0.0 (用于电子墨水屏驱动)
- **ESP-IDF**: 5.4.1
- **硬件**: M5Stack Papers3 ESP32S3-N16R8 (16MB Flash + 8MB PSRAM)

## 硬件规格

基于[M5Stack Papers3官方规格](https://docs.m5stack.com/zh_CN/core/papers3)：

### 核心规格
- **SoC**: ESP32-S3R8，Xtensa® 32位 LX7双核处理器，240MHz，2.4GHz Wi-Fi
- **PSRAM**: 8MB PSRAM  
- **Flash**: 16MB 外部闪存
- **显示屏**: 4.7" 触控电子墨水屏（全面屏）@EPD_ED047TC1，分辨率：960x540像素，16级灰度显示
- **触摸功能**: 支持两点触控与多种手势操作（GT911电容式触摸面板）
- **电池**: 3.7V@1800mAh 锂电池
- **尺寸**: 121.5 x 67.0 x 7.7mm
- **重量**: 92.5g

### 集成传感器和外设
- **传感器**: 内置陀螺仪传感器 BMI270@通讯地址：0x68
- **RTC**: 内置 BM8563 RTC 芯片（支持休眠与唤醒功能）@通讯地址：0x51  
- **蜂鸣器**: 板载无源蜂鸣器 (GPIO 21)
- **电池检测**: 板载电池检测电路 (GPIO 3, ADC1_CHANNEL_2)
- **按键**: 1x 物理按键（用于设备控制，开关机，复位，下载模式）
- **外设接口**: HY1.25-4P（3v3+GND+2xGPIO）外设接口（用于扩展传感器和设备）

### GPIO配置 (基于Papers3硬件)
- **Buzzer**: GPIO 21 (LEDC PWM输出，连接板载蜂鸣器)
- **Battery**: GPIO 3 (ADC1_CHANNEL_2, 电池电压检测电路)
- **Touch Panel**: GT911电容式触摸面板 (I2C接口)
- **Gyroscope**: BMI270陀螺仪传感器 (I2C地址: 0x68)
- **RTC**: BM8563实时时钟 (I2C地址: 0x51)

## 快速开始

### 1. 环境准备

首先运行准备脚本自动设置开发环境：

```bash
./scripts/prepare.sh
```

这个脚本会：
- 自动克隆MicroPython v1.25.0
- 自动克隆EPDiy v2.0.0  
- 初始化必要的git子模块
- 检查ESP-IDF安装状态
- 验证开发工具完整性

### 2. 编译固件

```bash
./scripts/build.sh
```

### 3. 烧写固件

```bash
# 清空Flash
esptool.py erase_flash

# 烧写固件
esptool.py write_flash -z 0x0 ./build/firmware.bin
```

## 目录结构

```
./micropython/       # MicroPython v1.25.0 (自动克隆)
./epdiy/             # EPDiy v2.0.0 (自动克隆)
./papers3-esp-demo/  # 原始参考工程 (手动提供)
./papers3/           # Papers3模块源代码 (核心实现)
./scripts/           # 构建和实用脚本
./build/             # 编译输出目录
```

## API文档 - 面向对象设计

Papers3模块采用纯面向对象设计，专为M5Stack Papers3硬件优化。

### 系统信息

```python
import papers3

# 显示M5Stack Papers3系统信息
papers3.info()

# Flash存储信息 (16MB)
flash_info = papers3.flash_info()
print(f"Flash: {flash_info['total_mb']} MB")

# RAM内存信息 (8MB PSRAM)
ram_info = papers3.ram_info()
print(f"Internal RAM: {ram_info['internal_free']} bytes free")
print(f"PSRAM: {ram_info['psram_free']} bytes free")
```

### 蜂鸣器控制 (板载无源蜂鸣器)

```python
# 创建蜂鸣器对象 (GPIO 21)
buzzer = papers3.Buzzer()

# 初始化PWM
buzzer.init()

# 播放蜂鸣声 (频率Hz, 持续时间ms)
buzzer.beep(1000, 200)  # 1kHz, 200ms
buzzer.beep(2000, 500)  # 2kHz, 500ms

# 清理资源
buzzer.deinit()
```

### 电池监控 (1800mAh锂电池)

```python
# 创建电池监控对象 (GPIO 3)
battery = papers3.Battery()

# 初始化ADC
battery.init()

# 读取电池信息
voltage = battery.voltage()      # 电压 (毫伏)
percentage = battery.percentage()  # 电量百分比
raw_adc = battery.adc_raw()       # 原始ADC值

print(f"电池电压: {voltage} mV")
print(f"电池电量: {percentage}%")

# 清理资源
battery.deinit()
```

## 实现特性

### ✅ 已实现功能

- **面向对象架构** - 清晰的类接口设计
- **M5Stack Papers3硬件抽象** - ESP32S3-N16R8完整支持
- **PWM蜂鸣器** - GPIO 21，板载无源蜂鸣器控制
- **ADC电池监控** - GPIO 3，1800mAh锂电池实时监控
- **系统信息** - Flash、RAM、芯片详细信息
- **构建系统** - 符号链接保持git仓库完整性
- **FreeRTOS集成** - 完整的实时操作系统支持

### 🚧 计划功能

- **EPDiy集成** - 4.7" 960x540电子墨水屏支持
- **触摸屏控制** - GT911电容式触摸面板
- **陀螺仪传感器** - BMI270运动检测
- **RTC时钟** - BM8563实时时钟和睡眠唤醒
- **WiFi连接** - 2.4GHz无线网络功能
- **示例程序** - 完整的M5Stack Papers3应用案例

## 电池监控公式 (基于Papers3硬件设计)

```
电池电压(mV) = (ADC原始值 * 3500 * 2) / 4096
电量百分比 = ((电压 - 3000) * 100) / (4200 - 3000)
```

*注: 电压计算公式基于M5Stack Papers3的板载电池检测电路设计*

## 存储规格

- **Flash**: 16MB (编译后使用率19%, 剩余12.9MB)
- **PSRAM**: 8MB (高速缓存用途)
- **固件大小**: 1.6MB
- **MicroSD**: 支持扩展存储 (硬件预留接口)

## 开发指南

### 模块架构

```