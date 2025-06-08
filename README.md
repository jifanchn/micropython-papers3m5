# MicroPython Papers3M5 项目

[English](README.en.md) | 中文

## 🎯 项目概述

本项目为 M5Stack Papers3 开发板提供完整的 MicroPython 支持，包括电子墨水屏、触摸屏、传感器等硬件功能。

## 📋 硬件规格

- **主控**: ESP32-S3R8 (16MB Flash + 8MB PSRAM)
- **显示屏**: 4.7" 电子墨水屏 960×540，16级灰度
- **触摸屏**: GT911电容式触摸面板 (I2C)
- **传感器**: BMI270 陀螺仪 (I2C地址: 0x68)
- **RTC**: BM8563实时时钟 (I2C地址: 0x51)
- **音响**: GPIO 21 无源蜂鸣器 (PWM控制)
- **电池**: GPIO 3 电池检测 (ADC1_CHANNEL_2, 1800mAh)
- **LED**: 板载状态LED

## 🚀 快速开始

### 1. 环境准备

```bash
# 克隆项目
git clone <repository-url>
cd micropython-papers3m5

# 准备开发环境
./scripts/prepare.sh
```

### 2. 编译固件

```bash
# 编译 MicroPython 固件
./scripts/build.sh
```

### 3. 烧写固件

```bash
# 使用智能烧写脚本
./scripts/flash.sh -e
```

### 4. 基础测试

```python
import papers3

# 创建测试实例
test = papers3.test()

# 查看帮助
test.help()

# 初始化硬件
test.init()

# 运行基础测试
test.basic()
```

## 🖼️ 显示功能详解

### 多字体大小支持

Papers3 支持中文字体显示：

- **70px行高** - 中文字体(生成参数24px)，支持7000+常用汉字
  - 字体度量：ascender=53px, descender=-17px, advance_y=70px  
  - 实际显示高度：70px，推荐行间距：70px

### 文字绘制API

```python
import papers3

# 初始化显示器
epdiy = papers3.EPDiy()
epdiy.init()

# 绘制中文文字
epdiy.draw_text("中文文本", 10, 50, 0)              # 中文字体(行高70px)
epdiy.draw_text("English Text", 10, 100, 0)        # 同样支持英文

# 更新显示
epdiy.update()
```

### 完整显示API

```python
# 基础图形绘制
epdiy.clear()                                    # 清屏
epdiy.draw_rect(x, y, width, height, color)      # 绘制矩形
epdiy.fill_rect(x, y, width, height, color)      # 填充矩形
epdiy.draw_circle(x, y, radius, color)           # 绘制圆形
epdiy.fill_circle(x, y, radius, color)           # 填充圆形
epdiy.draw_line(x1, y1, x2, y2, color)           # 绘制直线
epdiy.draw_triangle(x1, y1, x2, y2, x3, y3, color)  # 绘制三角形
epdiy.fill_triangle(x1, y1, x2, y2, x3, y3, color)  # 填充三角形

# 文字绘制 (支持中文)
epdiy.draw_text(text, x, y, color)               # 中文字体 (实际高度70px，行间距70px)

# 显示更新
epdiy.update()                                   # 更新屏幕显示
epdiy.clear_screen()                             # 清除并更新
```

## 🔧 硬件模块API

### 蜂鸣器控制

```python
# 初始化蜂鸣器
buzzer = papers3.Buzzer()
buzzer.init()

# 播放声音
buzzer.beep(frequency, duration)  # 频率(Hz), 持续时间(ms)
buzzer.beep(1000, 500)           # 1kHz音调持续500ms

# 清理资源
buzzer.deinit()
```

### 电池监控

```python
# 初始化电池监控
battery = papers3.Battery()
battery.init()

# 读取电池信息
voltage = battery.voltage()      # 电压(mV)
percentage = battery.percentage() # 电量百分比

print(f"电池电压: {voltage} mV")
print(f"电池电量: {percentage}%")

# 清理资源
battery.deinit()
```

### 陀螺仪传感器

```python
# 初始化陀螺仪
gyro = papers3.Gyro()
gyro.init()

# 读取传感器数据
accel = gyro.read_accel()  # 加速度计 [x, y, z]
gyro_data = gyro.read_gyro()  # 陀螺仪 [x, y, z]

print(f"加速度: {accel}")
print(f"陀螺仪: {gyro_data}")

# 清理资源
gyro.deinit()
```

### 实时时钟 (RTC)

```python
# 初始化RTC
rtc = papers3.RTC()
rtc.init()

# 读取时间
current_time = rtc.datetime()
print(f"当前时间: {current_time}")

# 设置时间 (年, 月, 日, 星期, 时, 分, 秒)
rtc.set_datetime(2024, 12, 25, 3, 10, 30, 0)

# 清理资源
rtc.deinit()
```

### 触摸屏

```python
# 初始化触摸屏
touch = papers3.Touch()
touch.init()

# 读取触摸数据
touch.update()                    # 更新触摸数据
num_touches = touch.get_touches() # 获取触摸点数量

for i in range(num_touches):
    point = touch.get_point(i)    # 获取触摸点 [x, y, size]
    x, y, size = point
    print(f"触摸点 {i}: X={x}, Y={y}, Size={size}")

# 清理资源
touch.deinit()
```

### LED控制

```python
# 初始化LED
led = papers3.LED()
led.init()

# 控制LED
led.on()   # 点亮
led.off()  # 熄灭

# 清理资源
led.deinit()
```

## 🎮 完整测试框架

### 测试命令概览

```python
import papers3
test = papers3.test()

# 基础功能
test.help()           # 显示帮助信息
test.init()           # 初始化所有硬件
test.basic()          # 运行基础硬件测试
test.system_info()    # 显示系统信息

# 硬件模块测试
test.buzzer_test()    # 蜂鸣器测试
test.battery_test()   # 电池监控测试
test.led_test()       # LED测试
test.gyro_test()      # 陀螺仪测试
test.rtc_test()       # RTC测试
test.touch_test()     # 触摸屏测试

# 显示功能测试
test.simple_chinese() # 简单中文显示测试
test.display_test()   # 基础显示测试
test.chinese()        # 中文字体测试
test.complex_display() # 复杂显示测试（系统状态）
test.touch_paint()    # 触摸绘图测试

# 演示和清理
test.demo()           # 演示模式
test.cleanup()        # 清理所有资源
```

### 字体显示测试

```python
# 简单中文测试 - 使用小字体
test.simple_chinese()

# 复杂显示测试 - 展示系统状态信息
test.complex_display()

# 手动测试中文字体
epdiy = papers3.EPDiy()
epdiy.init()
epdiy.clear()

# 显示中英文对比 (注意70px行间距)
epdiy.draw_text("中文字体 Chinese Font", 50, 100, 0)
epdiy.draw_text("English Font Display", 50, 170, 0)  # 100+70=170

epdiy.update()
```

## 🏗️ 项目架构

```
micropython-papers3m5/
├── micropython/         # MicroPython v1.25.0 (自动克隆)
├── epdiy/              # EPDiy 电子墨水屏库
├── papers3/            # Papers3 MicroPython 模块
│   ├── modpapers3.c    # 主模块入口
│   ├── papers3_epdiy.c # 显示器支持 (多字体)
│   ├── papers3_buzzer.c # 蜂鸣器
│   ├── papers3_battery.c # 电池监控
│   ├── papers3_gyro.c  # 陀螺仪
│   ├── papers3_rtc.c   # 实时时钟
│   ├── papers3_touch.c # 触摸屏
│   ├── papers3_led.c   # LED控制
│   ├── chinese_24.h    # 中文字体(生成参数24px, 实际行高70px)
│   └── test.py         # 测试框架
├── scripts/            # 构建脚本
│   ├── prepare.sh      # 环境准备
│   ├── build.sh        # 固件编译
│   ├── flash.sh        # 智能烧写
│   └── generate_chinese_font.py # 字体生成工具
└── build/              # 编译输出
```

## 📊 系统信息

```python
# 查看系统信息
papers3.info()        # 基础系统信息
papers3.flash_info()  # Flash 使用情况
papers3.ram_info()    # RAM 状态信息
```

**当前固件规格:**
- **固件大小**: 5.0MB (占用8MB分区的62%)
- **Flash配置**: 16MB (8MB App + 8MB VFS)
- **RAM配置**: 8MB PSRAM + 512KB 内部RAM
- **字体支持**: 中文字体(行高70px) × 7000+汉字

## 🔄 开发工作流

### 1. 修改代码后重新编译

```bash
# 快速编译和烧写
./scripts/build.sh && ./scripts/flash.sh -e
```

### 2. 串口调试

```bash
# macOS/Linux
screen /dev/cu.usbserial-* 115200

# 或使用 minicom
minicom -b 115200 -D /dev/cu.usbserial-*
```

### 3. 字体管理

```bash
# 生成新字体 (需要先安装 freetype-py)
cd scripts
python3 generate_chinese_font.py --size 20 --output ../papers3

# 在 papers3_epdiy.c 中添加新字体支持
# 然后重新编译
```

## 🎨 界面设计建议

### 字体特点

- **完整中文支持**: 包含7000+常用汉字
- **清晰显示**: 24px大小适合4.7寸屏幕阅读
- **双语支持**: 中英文混合显示效果良好

### 排版建议

```python
# 标题区域
epdiy.draw_text("Papers3 应用", 20, 30, 0)

# 主要内容
epdiy.draw_text("这是主要内容区域", 20, 80, 0)

# 状态信息
epdiy.draw_text("状态: 就绪", 20, 500, 8)
```

## 🛠️ 故障排除

### 常见问题

1. **编译失败 - 分区不足**
   ```
   Error: app partition is too small
   ```
   解决: 已更新分区配置支持12MB App分区

2. **触摸不响应**
   ```python
   # 确保先调用 update()
   touch.update()
   num_touches = touch.get_touches()
   ```

3. **字体显示重叠**
   ```python
   # 使用合适的字体大小和行间距
   epdiy.draw_text("文本1", 50, 100, 0, "small")  # 16px
   epdiy.draw_text("文本2", 50, 125, 0, "small")  # 25px间距
   ```

4. **内存不足**
   ```python
   # 查看内存状态
   papers3.ram_info()
   
   # 及时清理资源
   test.cleanup()
   ```

## 📈 性能优化

- **固件大小**: 7.1MB（包含2套完整中文字体）
- **启动时间**: ~3秒完成硬件初始化
- **显示更新**: ~800ms 全屏刷新
- **内存使用**: 280KB 内部RAM，2MB PSRAM可用

## 🤝 贡献指南

1. Fork 项目仓库
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

## 🙏 致谢

- [MicroPython](https://micropython.org/) - Python 微控制器实现
- [EPDiy](https://github.com/vroland/epdiy) - 电子墨水屏驱动库
- [M5Stack](https://m5stack.com/) - Papers3 硬件平台

---

**Happy Coding! 🚀**