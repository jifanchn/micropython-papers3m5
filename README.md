# MicroPython Papers3M5 项目

为M5Stack Papers3硬件提供MicroPython支持的完整解决方案。

## 硬件规格

- **主控**: ESP32-S3R8 (双核 240MHz)
- **存储**: 16MB Flash + 8MB PSRAM  
- **显示**: 4.7英寸 E-Ink 电子墨水屏 (960x540)
- **传感器**: BMI270 6轴IMU (加速度计+陀螺仪)
- **时钟**: BM8563 实时时钟 (RTC)
- **接口**: I2C, SPI, GPIO, ADC
- **电源**: 锂电池 + USB-C充电

## 版本信息

- **MicroPython**: v1.25.0
- **EPDiy**: main分支 (commit: fe3113a) - 已集成到项目中，修复ESP-IDF v5.4.1编译问题
- **ESP-IDF**: v5.4.1
- **固件大小**: 1.8MB (12% Flash使用率)

### EPDiy集成说明

EPDiy库已完全集成到项目中，不再作为外部依赖：

- **原始仓库**: https://github.com/vroland/epdiy
- **集成版本**: main分支 (commit: fe3113a)
- **修改内容**: 
  - 清除了原始.git信息，成为项目的组成部分
  - 针对M5Stack Papers3硬件进行了适配修改
  - 修复了ESP-IDF v5.4.1的编译兼容性问题
  - 实现了MicroPython接口封装
- **集成原因**: 
  - 避免外部依赖管理复杂性
  - 便于针对Papers3硬件进行定制修改
  - 确保版本一致性和构建稳定性

## 已实现功能

### ✅ 核心系统
- [x] 系统信息查询 (`papers3.info()`)
- [x] 内存和Flash状态监控
- [x] ESP32-S3硬件特性支持

### ✅ E-Ink显示 (EPDiy)
- [x] 显示初始化和清屏 (`papers3.epdiy.init()`, `papers3.epdiy.clear()`)
- [x] 基础绘图函数 (`draw_pixel`, `draw_line`, `draw_rect`, `fill_rect`)
- [x] 高级绘图函数 (`draw_circle`, `fill_circle`, `draw_triangle`, `fill_triangle`)
- [x] 文本绘制 (`papers3.epdiy.draw_text()`)
- [x] 屏幕更新 (`papers3.epdiy.update()`)
- [x] 4.7英寸960x540分辨率支持，16级灰度

### ✅ PWM蜂鸣器
- [x] 蜂鸣器初始化 (`papers3.buzzer.init()`)
- [x] 音调播放 (`papers3.buzzer.beep(frequency, duration)`)
- [x] 频率范围：100Hz - 10kHz

### ✅ 电池监控
- [x] 电池初始化 (`papers3.battery.init()`)
- [x] 电压读取 (`papers3.battery.voltage()`)
- [x] 状态查询 (`papers3.battery.status()`)
- [x] ADC精度：12位，范围0-3.3V

### ✅ BMI270 陀螺仪传感器
- [x] 6轴IMU支持 (3轴加速度计 + 3轴陀螺仪)
- [x] 传感器初始化 (`gyro = papers3.Gyro(); gyro.init()`)
- [x] 加速度读取 (`gyro.read_accel()`) - 单位：g (重力加速度)
- [x] 陀螺仪读取 (`gyro.read_gyro()`) - 单位：dps (度/秒)
- [x] 测量范围：±4G (加速度), ±2000dps (陀螺仪)
- [x] 采样频率：100Hz
- [x] I2C地址：0x68

### ✅ BM8563 实时时钟 (RTC)
- [x] 实时时钟功能
- [x] RTC初始化 (`rtc = papers3.RTC(); rtc.init()`)
- [x] 时间读取 (`rtc.datetime()`) - 格式：(年, 月, 日, 星期, 时, 分, 秒)
- [x] 时间设置 (`rtc.datetime(year, month, day, weekday, hour, minute, second)`)
- [x] 闹钟功能 (`rtc.alarm(hour, minute)`)
- [x] I2C地址：0x51

## 快速开始

### 1. 编译固件

```bash
# 克隆项目
git clone <repository-url>
cd micropython-papers3m5

# 准备环境（仅初次使用需要）
./scripts/prepare.sh

# 编译固件
./scripts/build.sh
```

### 2. 烧写固件

```bash
# 清空Flash
esptool.py erase_flash

# 烧写固件
esptool.py write_flash -z 0x0 build/firmware.bin
```

### 3. 基础测试

```python
import papers3

# 系统信息
papers3.info()

# E-Ink显示测试
papers3.epdiy.init()
papers3.epdiy.clear()
papers3.epdiy.draw_text(100, 100, "Hello Papers3", 0x00)
papers3.epdiy.update()

# 蜂鸣器测试
papers3.buzzer.init()
papers3.buzzer.beep(1000, 500)  # 1kHz, 500ms

# 电池状态
papers3.battery.init()
print("电池电压:", papers3.battery.voltage(), "V")
print("电池状态:", papers3.battery.status())
```

### 4. 传感器测试

```python
# BMI270陀螺仪测试
gyro = papers3.Gyro()
gyro.init()
print('加速度 (g):', gyro.read_accel())    # 例：(-0.002, -0.003, -0.990)
print('陀螺仪 (dps):', gyro.read_gyro())   # 例：(-0.305, -0.183, 0.183)

# BM8563 RTC测试
rtc = papers3.RTC()
rtc.init()
print('当前时间:', rtc.datetime())          # 例：(29, 1, 5, 5, 12, 7, 34)
rtc.datetime(2025, 1, 25, 0, 14, 30, 0)    # 设置时间
rtc.alarm(8, 30)                           # 设置8:30闹钟
```

## API 参考

### 系统模块

```python
papers3.info()                    # 显示系统信息
```

### EPDiy显示模块

```python
papers3.epdiy.init()              # 初始化显示
papers3.epdiy.clear()             # 清屏
papers3.epdiy.draw_text(x, y, text, color)  # 绘制文本
papers3.epdiy.update()            # 更新显示

# 基础绘图函数
papers3.epdiy.draw_pixel(x, y, color)           # 绘制像素
papers3.epdiy.draw_line(x0, y0, x1, y1, color)  # 绘制直线
papers3.epdiy.draw_rect(x, y, w, h, color)      # 绘制矩形
papers3.epdiy.fill_rect(x, y, w, h, color)      # 填充矩形

# 高级绘图函数
papers3.epdiy.draw_circle(x, y, r, color)       # 绘制圆形
papers3.epdiy.fill_circle(x, y, r, color)       # 填充圆形
papers3.epdiy.draw_triangle(x0, y0, x1, y1, x2, y2, color)  # 绘制三角形
papers3.epdiy.fill_triangle(x0, y0, x1, y1, x2, y2, color)  # 填充三角形
```

### 蜂鸣器模块

```python
papers3.buzzer.init()             # 初始化蜂鸣器
papers3.buzzer.beep(freq, duration)  # 播放音调
```

### 电池模块

```python
papers3.battery.init()            # 初始化电池监控
papers3.battery.voltage()         # 读取电压 (V)
papers3.battery.status()          # 获取状态信息
```

### BMI270陀螺仪模块

```python
gyro = papers3.Gyro()             # 创建陀螺仪对象
gyro.init()                       # 初始化传感器
gyro.read_accel()                 # 读取加速度 (x, y, z) 单位：g
gyro.read_gyro()                  # 读取陀螺仪 (x, y, z) 单位：dps
```

### BM8563 RTC模块

```python
rtc = papers3.RTC()               # 创建RTC对象
rtc.init()                        # 初始化RTC
rtc.datetime()                    # 读取时间 (年,月,日,星期,时,分,秒)
rtc.datetime(year, month, day, weekday, hour, minute, second)  # 设置时间
rtc.alarm(hour, minute)           # 设置闹钟
```

## 技术架构

### I2C总线配置
- **SDA引脚**: GPIO 41
- **SCL引脚**: GPIO 42  
- **频率**: 100kHz
- **驱动**: ESP-IDF I2C驱动 (避免MicroPython machine模块冲突)

### 构建系统
- EPDiy库已完全集成，避免外部依赖
- 支持增量编译和清理构建
- 自动环境检测和依赖管理
- MicroPython仍使用符号链接保持完整性

### 内存管理
- Flash使用率：19% (1.6MB/8.5MB可用)
- PSRAM支持：8MB外部PSRAM
- 堆内存优化：支持大型应用

## 开发进度

- [x] **阶段1**: 基础系统支持 (EPDiy, 蜂鸣器, 电池)
- [x] **阶段2**: 传感器集成 (BMI270陀螺仪, BM8563 RTC)
- [ ] **阶段3**: SD卡存储支持
- [ ] **阶段4**: WiFi和网络功能
- [ ] **阶段5**: 高级应用示例

## 测试验证

### 硬件测试结果 ✅

**BMI270陀螺仪**：
- 加速度计：正常读取重力数据 (-0.002, -0.003, -0.990)g
- 陀螺仪：正常读取角速度数据 (-0.305, -0.183, 0.183)dps
- I2C通信：稳定，无错误

**BM8563 RTC**：
- 时间读取：正常 (29, 1, 5, 5, 12, 7, 34)
- 时间设置：支持
- I2C通信：稳定，无错误

**系统稳定性**：
- 启动时间：< 3秒
- 内存使用：正常
- 无崩溃或重启问题

## 故障排除

### 常见问题

1. **编译失败**
   - 检查ESP-IDF版本 (需要v5.4.1)
   - 确认子模块已正确初始化
   - 清理构建缓存：`./scripts/clean.sh`

2. **I2C通信错误**
   - 确认硬件连接正确
   - 检查I2C引脚配置 (SDA=41, SCL=42)
   - 验证传感器电源供应

3. **显示问题**
   - 确认EPDiy库版本匹配
   - 检查显示屏连接
   - 尝试重新初始化：`papers3.epdiy.init()`

## 贡献指南

1. Fork项目仓库
2. 创建功能分支：`git checkout -b feature/new-feature`
3. 提交更改：`git commit -m "Add new feature"`
4. 推送分支：`git push origin feature/new-feature`
5. 创建Pull Request

## 许可证

本项目采用MIT许可证 - 详见 [LICENSE](LICENSE) 文件。

## 🏆 项目状态

✅ **已完成**: EPDiy完全集成，绘图功能完善，面向对象架构，构建系统，文档完善
🚀 **可投产**: 固件编译成功，核心功能验证通过，开发工具链完整，EPDiy库完全集成
🎯 **EPDiy状态**: main分支 (commit: fe3113a) 完全集成到项目中，支持完整2D绘图功能

## 致谢

- [MicroPython](https://micropython.org/) - Python 3解释器
- [EPDiy](https://github.com/vroland/epdiy) - E-Ink显示驱动
- [ESP-IDF](https://github.com/espressif/esp-idf) - ESP32开发框架
- [M5Stack](https://m5stack.com/) - Papers3硬件平台