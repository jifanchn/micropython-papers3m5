# Papers3 演示程序

这个目录包含了 Papers3 的演示和测试程序。

## 🎯 功能特点

- **完整测试框架**: 全面的硬件功能测试
- **屏幕范围测试**: 测试屏幕的有效显示区域
- **文本布局测试**: 测试不同间距、起始位置的文本显示
- **坐标网格显示**: 显示屏幕坐标系统
- **边界检测**: 测试屏幕边缘显示效果
- **参数可调**: 支持自定义起始点、间距、行数等参数

## 📁 文件说明

- `test.py` - 完整的Papers3测试框架 (包含所有硬件模块测试)
- `screen_range_test.py` - 屏幕范围测试工具 (专门用于显示测试)
- `README.md` - 本说明文件

## 🚀 使用方法

### Papers3 完整测试框架 (test.py)

```python
# 导入测试模块
import test

# 创建测试实例
t = test.create_test()

# 查看帮助
t.help()

# 初始化硬件
t.init()

# 运行基础测试
t.basic()

# 硬件模块测试
t.buzzer_test()    # 蜂鸣器测试
t.battery_test()   # 电池监控测试
t.led_test()       # LED测试
t.gyro_test()      # 陀螺仪测试
t.rtc_test()       # RTC测试
t.touch_test()     # 触摸屏测试

# 显示功能测试
t.simple_chinese() # 简单中文显示测试
t.display_test()   # 基础显示测试
t.chinese()        # 中文字体测试
t.complex_display() # 复杂显示测试（系统状态）
t.touch_paint()    # 触摸绘图测试

# 演示和清理
t.demo()           # 演示模式
t.cleanup()        # 清理所有资源
```

### 屏幕范围测试工具 (screen_range_test.py)

### 方法1: 复制到ESP32并运行

1. **复制文件到ESP32**:
   ```bash
   # 使用 ampy 工具复制文件
   ampy -p /dev/ttyUSB0 put screen_range_test.py
   
   # 或使用项目提供的下载脚本 (推荐)
   ./scripts/download.sh
   ```

2. **在ESP32上运行**:
   ```python
   # 连接到ESP32的REPL
   import screen_range_test as srt
   
   # 运行基础测试
   srt.test_basic()
   
   # 运行网格测试
   srt.test_grid(100)
   
   # 运行文本布局测试
   srt.test_text_layout(50, 50, 80, 6)
   
   # 清理资源
   srt.cleanup()
   ```

### 方法2: 函数式调用

```python
import screen_range_test as srt

# 查看帮助
srt.help()

# 基础屏幕范围测试
srt.test_basic()

# 网格测试 (100px间距)
srt.test_grid(100)

# 文本布局测试 (起点50,50, 行距80px, 6行)
srt.test_text_layout(50, 50, 80, 6)

# 自定义测试 (起点100,80, 行距70px, 5行, 带网格)
srt.test_custom(100, 80, 70, 5, True)

# 边界测试
srt.test_boundaries()

# 清理资源
srt.cleanup()
```

### 方法3: 类方式调用

```python
import screen_range_test

# 创建测试实例
test = screen_range_test.ScreenRangeTest()

# 初始化
test.init()

# 运行各种测试
test.test_basic()
test.test_grid(80)
test.test_text_layout(60, 70, 60, 7)

# 清理
test.cleanup()
```

## 📱 Papers3 屏幕参数

- **分辨率**: 960 x 540 像素
- **坐标系**: (0,0) 为左上角，(959,539) 为右下角
- **颜色值**: 
  - 0 = 黑色
  - 8 = 灰色
  - 15 = 白色

## 🔧 测试功能详解

### 1. 基础测试 (`test_basic()`)
- 显示屏幕边界框
- 标记四个角落和中心点
- 显示坐标信息

### 2. 网格测试 (`test_grid(size)`)
- 显示坐标网格
- 参数: `size` - 网格间距 (默认100px)
- 帮助理解坐标系统

### 3. 文本布局测试 (`test_text_layout(x, y, spacing, lines)`)
- 测试文本间距和排列
- 参数:
  - `x, y` - 起始坐标 (默认50, 50)
  - `spacing` - 行间距 (默认80px)
  - `lines` - 行数 (默认6行)

### 4. 自定义测试 (`test_custom(x, y, spacing, lines, grid)`)
- 自定义参数的综合测试
- 参数:
  - `x, y` - 起始坐标 (默认100, 80)
  - `spacing` - 行间距 (默认70px)
  - `lines` - 行数 (默认5行)
  - `grid` - 是否显示网格 (默认False)

### 5. 边界测试 (`test_boundaries()`)
- 测试屏幕边缘显示效果
- 检查各个边界位置的文字显示

## 🛠️ 工具安装和使用

### 安装 ampy 工具

```bash
pip install adafruit-ampy
```

### 复制文件到ESP32

```bash
# 查看可用串口
ls /dev/ttyUSB* 

# 复制屏幕范围测试文件
ampy -p /dev/ttyUSB0 put screen_range_test.py

# 复制演示文件
ampy -p /dev/ttyUSB0 put demo.py

# 查看ESP32上的文件
ampy -p /dev/ttyUSB0 ls
```

### 连接ESP32 REPL

```bash
# 使用 picocom 连接
picocom /dev/ttyUSB0 -b 115200

# 或使用 screen
screen /dev/ttyUSB0 115200
```

### 在ESP32上运行

```python
# 在REPL中运行
>>> import screen_range_test as srt
>>> srt.help()
>>> srt.test_basic()
>>> srt.cleanup()

# 或运行演示
>>> import screen_range_test as srt
>>> srt.demo_quick()
>>> srt.demo_interactive()
```

## 📊 测试参数建议

### 推荐的测试参数组合

```python
# 标准测试
srt.test_text_layout(50, 50, 80, 6)

# 密集文本测试
srt.test_text_layout(40, 40, 50, 10)

# 大字体测试
srt.test_text_layout(100, 100, 120, 4)

# 右侧区域测试
srt.test_text_layout(500, 60, 70, 7)

# 网格辅助测试
srt.test_custom(80, 60, 60, 8, True)
```

## 🐛 常见问题

### Q: 文本显示不完整或被截断
A: 调整 `start_x` 和 `start_y` 参数，确保文本不超出屏幕边界

### Q: 行间距太小，文字重叠
A: 增加 `line_spacing` 参数，建议最小值为50px

### Q: 显示器初始化失败
A: 检查 papers3 模块是否正确安装和配置

### Q: 程序运行后没有显示
A: 确保调用了 `update()` 方法更新屏幕显示

## 📝 注意事项

1. **电子墨水屏特性**: 更新较慢，每次测试后建议等待几秒
2. **资源管理**: 测试完成后记得调用 `cleanup()` 清理资源
3. **坐标范围**: 确保坐标在 (0,0) 到 (959,539) 范围内
4. **文本长度**: 过长的文本会被自动截断，建议单行不超过25个字符

## 🎨 扩展使用

你可以基于这个工具开发自己的显示测试程序:

```python
import screen_range_test as srt

# 获取测试实例
test = srt._get_test_instance()

# 直接使用EPDiy对象
epd = test.epdiy
epd.clear()
epd.draw_text("自定义文本", 100, 100, 0)
epd.update()

# 清理
srt.cleanup()
```

## 🤝 贡献

如果你发现问题或有改进建议，欢迎提交issue或pull request。 