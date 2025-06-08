# Papers3 字体支持文档

## 概述

Papers3 MicroPython固件提供完整的中文汉字和英文字体支持，基于EPDiy字体系统实现。支持从TTF字体文件生成适用于电子墨水屏的字体数据，并提供简单易用的Python API。

## 特性

- ✅ **完整UTF-8支持**: 支持中文汉字、英文、数字、符号等
- ✅ **字体压缩**: 内置zlib压缩，节省Flash存储空间
- ✅ **多尺寸字体**: 支持生成不同大小的字体
- ✅ **自定义字符集**: 可指定需要的字符，减少字体文件大小
- ✅ **高效渲染**: 基于EPDiy优化的字体渲染引擎
- ✅ **Python API**: 简洁的MicroPython接口

## 字体生成

### 1. 安装依赖

```bash
# 安装freetype-py库
pip install freetype-py
```

### 2. 下载中文字体

推荐使用以下开源中文字体：

- **Noto Sans CJK**: Google开源，支持简繁中文、日文、韩文
  ```bash
  wget https://github.com/googlefonts/noto-cjk/releases/download/Sans2.004/NotoSansCJK-Regular.ttc
  ```

- **思源黑体**: Adobe开源，与Noto Sans CJK相同
  ```bash
  wget https://github.com/adobe-fonts/source-han-sans/releases/download/2.004R/SourceHanSansCN.zip
  ```

- **文泉驿微米黑**: 轻量级开源中文字体
  ```bash
  wget http://wenq.org/wqy2/index.cgi?FontGuide#download
  ```

### 3. 生成字体文件

使用项目提供的字体生成脚本：

```bash
# 生成16px中文字体（包含1500+常用汉字）
python scripts/generate_fonts.py chinese NotoSansCJK-Regular.ttf --size 16 --output fonts/

# 生成12px英文字体
python scripts/generate_fonts.py english Arial.ttf --size 12 --output fonts/

# 生成多个尺寸
python scripts/generate_fonts.py chinese NotoSansCJK-Regular.ttf --sizes 12,16,20,24

# 生成自定义字符集（仅包含需要的字符）
python scripts/generate_fonts.py custom NotoSansCJK-Regular.ttf --chars "你好世界Hello123" --size 16
```

### 4. 生成的文件

生成的字体文件如下：
```
fonts/
├── papers3_chinese_16.h    # 16px中文字体
├── papers3_english_12.h    # 12px英文字体
└── papers3_custom_16.h     # 16px自定义字体
```

## 在固件中集成字体

### 1. 添加字体文件到项目

将生成的 `.h` 文件复制到 `papers3/` 目录：

```bash
cp fonts/papers3_chinese_16.h papers3/fonts/
cp fonts/papers3_english_12.h papers3/fonts/
```

### 2. 在CMakeLists.txt中注册

编辑 `papers3/CMakeLists.txt`，添加字体文件：

```cmake
# 添加字体支持
target_sources(${COMPONENT_LIB} PRIVATE
    font_loader.c
    fonts/papers3_chinese_16.h
    fonts/papers3_english_12.h
)

# 添加字体定义
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    PAPERS3_BUILTIN_FONTS=1
)
```

### 3. 注册字体到系统

在 `papers3/mod_papers3.c` 中注册字体：

```c
#include "fonts/papers3_chinese_16.h"
#include "fonts/papers3_english_12.h"

// 在模块初始化函数中注册字体
mp_obj_t papers3_init_fonts(void) {
    // 注册中文字体
    papers3_font_register("Chinese16", &Chinese16, FONT_TYPE_CHINESE, 16, true);
    
    // 注册英文字体
    papers3_font_register("English12", &English12, FONT_TYPE_ENGLISH, 12, true);
    
    return mp_const_none;
}
```

## MicroPython API使用

### 1. 基础字体操作

```python
import papers3

# 初始化字体系统
papers3.fonts.init()

# 列出可用字体
fonts = papers3.fonts.list()
print("可用字体:", fonts)

# 获取字体信息
font = papers3.fonts.get("Chinese16")
print(f"字体大小: {font.size}")
print(f"字符数量: {font.char_count}")
```

### 2. 文本渲染

```python
import papers3

# 初始化显示屏
display = papers3.Display()
display.init()

# 获取字体
chinese_font = papers3.fonts.get("Chinese16")
english_font = papers3.fonts.get("English12")

# 渲染中文文本
display.text("你好，世界！", 10, 10, font=chinese_font, fg_color=0, bg_color=15)

# 渲染英文文本
display.text("Hello, World!", 10, 40, font=english_font, fg_color=0, bg_color=15)

# 混合渲染
display.text("Papers3 电子墨水屏", 10, 70, font=chinese_font)

# 更新显示
display.update()
```

### 3. 文本测量

```python
# 测量文本尺寸
text = "你好世界"
width, height = chinese_font.measure_text(text)
print(f"文本 '{text}' 尺寸: {width}x{height}")

# 居中显示
screen_width = 960
screen_height = 540
x = (screen_width - width) // 2
y = (screen_height - height) // 2
display.text(text, x, y, font=chinese_font)
```

### 4. 高级字体功能

```python
# 检查字符是否存在
if chinese_font.has_char(ord('你')):
    print("字体包含字符 '你'")

# 逐字符渲染
text = "Hello你好"
x = 10
for char in text:
    char_width = chinese_font.measure_char(char)
    display.char(char, x, 10, font=chinese_font)
    x += char_width

# 多行文本
lines = ["第一行文本", "第二行文本", "第三行文本"]
y = 10
for line in lines:
    display.text(line, 10, y, font=chinese_font)
    y += chinese_font.line_height
```

## 字体优化建议

### 1. 字符集选择

- **常用汉字**: 使用项目内置的1500+常用汉字集合
- **专用字符**: 仅包含应用需要的字符以节省空间
- **ASCII字符**: 对于纯英文应用，使用ASCII字体更节省空间

```python
# 获取常用汉字列表
from papers3.fonts import get_common_chinese_chars
common_chars = get_common_chinese_chars()
print(f"常用汉字数量: {len(common_chars)}")
```

### 2. 字体大小选择

不同字体大小的适用场景：

| 字体大小 | 适用场景 | Flash占用 |
|---------|---------|-----------|
| 12px    | 状态栏、小标签 | ~500KB |
| 16px    | 正文、按钮文字 | ~800KB |
| 20px    | 标题、重要信息 | ~1.2MB |
| 24px    | 大标题、数字显示 | ~1.8MB |

### 3. 压缩设置

- **启用压缩**: 可节省50-70%的Flash空间，但渲染时需要解压缩
- **禁用压缩**: 渲染速度更快，但占用更多Flash空间

```bash
# 启用压缩（推荐）
python scripts/generate_fonts.py chinese font.ttf --size 16 --compress

# 禁用压缩（适用于对速度要求高的场景）
python scripts/generate_fonts.py chinese font.ttf --size 16 --no-compress
```

## 常见问题

### Q: 生成的字体文件太大怎么办？

A: 可以通过以下方式减小字体文件大小：

1. **减少字符集**: 使用 `--chars` 参数仅包含需要的字符
2. **启用压缩**: 使用 `--compress` 参数
3. **减小字体大小**: 选择更小的像素尺寸
4. **分割字体**: 将不同类型的字符分别生成字体

### Q: 如何添加自定义字符？

A: 使用自定义字符集生成：

```bash
# 仅包含应用需要的字符
python scripts/generate_fonts.py custom font.ttf --chars "你好世界123ABC!@#" --size 16
```

### Q: 字体渲染出现乱码怎么办？

A: 检查以下几点：

1. 确保字体文件包含相应字符
2. 检查文本编码是否为UTF-8
3. 验证字体是否正确注册到系统

```python
# 检查字符是否存在
if not font.has_char(ord('你')):
    print("字体不包含字符 '你'")
```

### Q: 如何在有限Flash空间中使用中文字体？

A: 推荐策略：

1. **分级字体**: 关键文字使用中文字体，其他使用ASCII字体
2. **动态加载**: 将字体存储在SD卡，按需加载
3. **字符分组**: 按使用频率分成多个小字体文件

## 技术细节

### 字体数据结构

Papers3字体基于EPDiy格式，包含以下组件：

```c
typedef struct {
    const uint8_t* bitmap;                // 字形位图数据
    const EpdGlyph* glyph;                // 字形信息数组
    const EpdUnicodeInterval* intervals;  // Unicode区间映射
    uint32_t interval_count;              // 区间数量
    bool compressed;                      // 是否压缩
    uint16_t advance_y;                   // 行高
    int ascender;                         // 上升高度
    int descender;                        // 下降高度
} EpdFont;
```

### Unicode区间

中文字符分布在以下Unicode区间：

- **CJK基本汉字**: `U+4E00` - `U+9FFF` (约20,000字)
- **CJK扩展A**: `U+3400` - `U+4DBF` (约6,500字)
- **CJK标点符号**: `U+3000` - `U+303F`
- **中文数字**: `U+FF10` - `U+FF19`

### 内存使用

字体渲染的内存开销：

- **字体数据**: 存储在Flash中，运行时不占用RAM
- **渲染缓冲**: 临时分配，大小取决于字符尺寸
- **压缩缓冲**: 压缩字体需要额外RAM用于解压缩

## 参考资源

- [EPDiy字体系统文档](https://epdiy.readthedocs.io/en/latest/api.html#fonts)
- [FreeType文档](https://freetype.org/freetype2/docs/documentation.html)
- [Unicode中文字符区间](https://unicode.org/charts/PDF/U4E00.pdf)
- [Google Noto字体](https://fonts.google.com/noto)
- [Adobe思源字体](https://adobe.com/fonts/source-han.html) 