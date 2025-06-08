# Papers3 中文字体度量信息

## 字体文件信息

- **文件名**: `chinese_24.h`
- **生成参数**: 24px (传递给fontconvert.py的size参数)
- **字符数**: 6826个字符 (中英文+符号)
- **字体来源**: `/System/Library/Fonts/Supplemental/Songti.ttc`

## 实际字体度量

从字体文件中提取的实际度量信息：

```c
// 字体度量参数 (来自chinese_24.h)
advance_y: 70,    // 推荐行间距 (y轴方向)
ascender: 53,     // 基线上方最大高度
descender: -17,   // 基线下方最大高度  
```

## 字体尺寸计算

- **总字体高度**: 53 - (-17) = **70px**
- **推荐行间距**: **80px** (实际测试验证，70px理论值可能重叠)
- **字符基线**: 相对于绘制Y坐标 +53px
- **字符顶部**: 相对于绘制Y坐标 +0px  
- **字符底部**: 相对于绘制Y坐标 +70px

## 正确的使用方法

### 单行文字
```python
# 绘制单行文字，Y坐标是基线位置
epdiy.draw_text("你好世界", 50, 100, 0)
```

### 多行文字 (正确间距)
```python
# 使用80px行间距避免重叠 (实际测试验证)
y_pos = 50
texts = ["第一行", "第二行", "第三行"]
for text in texts:
    epdiy.draw_text(text, 50, y_pos, 0)
    y_pos += 80  # 推荐行间距，确保不重叠
```

### 错误的间距设置
```python
# ❌ 错误：70px以下间距会导致文字重叠
y_pos = 50
for text in texts:
    epdiy.draw_text(text, 50, y_pos, 0)
    y_pos += 70  # 理论值，实际仍可能重叠
    # y_pos += 30  # 太小，严重重叠
```

## 坐标系统说明

EPDiy使用基线坐标系统：
- `draw_text(text, x, y, color)` 中的 `y` 参数是字符基线位置
- 字符会在基线上方和下方绘制
- 上方高度：53px (ascender)
- 下方高度：17px (descender的绝对值)

## 屏幕边界计算

Papers3 屏幕尺寸：960×540

### 安全绘制区域
```python
# 考虑字体高度的安全区域
safe_top = 53      # ascender高度
safe_bottom = 540 - 17  # 屏幕高度 - descender
safe_left = 0
safe_right = 960

# 多行文字的最大行数
max_lines = (safe_bottom - safe_top) // 80  # 约5-6行
```

## 性能信息

- **字体文件大小**: ~20MB (压缩后)
- **内存占用**: 加载到PSRAM中
- **支持字符**: 6826个常用中英文字符
- **渲染性能**: 硬件加速支持

## 调试技巧

### 验证字体度量
```python
# 打印字体文件中的度量信息
print("Font metrics from chinese_24.h:")
print("advance_y:", 70)
print("ascender:", 53) 
print("descender:", -17)
print("Total height:", 53 - (-17), "px")
```

### 可视化字体边界
```python
# 绘制字体边界框，帮助调试布局
def draw_text_with_bounds(epdiy, text, x, y, color):
    # 绘制文字
    epdiy.draw_text(text, x, y, color)
    
    # 绘制基线
    epdiy.draw_line(x, y, x + 200, y, 8)  # 灰色基线
    
    # 绘制上边界 (ascender)
    epdiy.draw_line(x, y - 53, x + 200, y - 53, 4)  
    
    # 绘制下边界 (descender)  
    epdiy.draw_line(x, y + 17, x + 200, y + 17, 4)
```

---
*文档更新时间: 2024年12月* 