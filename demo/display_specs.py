"""
Papers3 显示规格常量
包含经过测试验证的最佳边界设置和字体参数
"""

# 屏幕基本参数
SCREEN_WIDTH = 960
SCREEN_HEIGHT = 540

# 经过测试验证的最佳边界
OPTIMAL_BOUNDARIES = {
    'left': 10,      # 左边界 x=10
    'right': 850,    # 右边界 x=850  
    'top': 45,       # 上边界 y=45
    'bottom': 520    # 下边界 y=520
}

# 安全显示区域
SAFE_AREA = {
    'x': OPTIMAL_BOUNDARIES['left'],
    'y': OPTIMAL_BOUNDARIES['top'],
    'width': OPTIMAL_BOUNDARIES['right'] - OPTIMAL_BOUNDARIES['left'],   # 840px
    'height': OPTIMAL_BOUNDARIES['bottom'] - OPTIMAL_BOUNDARIES['top']   # 475px
}

# 文本布局参数
TEXT_LAYOUT = {
    'start_x': 50,           # 推荐文本起始x坐标
    'start_y': 80,           # 推荐文本起始y坐标  
    'line_spacing': 70,      # 推荐行间距
    'char_width': 10,        # 平均字符宽度
    'char_height': 16,       # 字符高度
    'max_chars_per_line': 80 # 每行最大字符数
}

# 颜色常量
COLORS = {
    'BLACK': 0,         # 黑色 - 主要文本和边框
    'DARK_GRAY': 4,     # 深灰
    'GRAY': 8,          # 中灰 - 坐标标注和网格
    'LIGHT_GRAY': 12,   # 浅灰 - 参考线和辅助元素
    'WHITE': 15         # 白色 - 背景和反色文本
}

# 线条和形状规格
LINE_SPECS = {
    'grid_lines': {'color': COLORS['GRAY'], 'width': 1},         # 网格线
    'border_lines': {'color': COLORS['BLACK'], 'width': 1},      # 边框线
    'reference_lines': {'color': COLORS['LIGHT_GRAY'], 'width': 1} # 参考线
}

SHAPE_SPECS = {
    'circles': {'min_radius': 3, 'max_radius': 50},
    'rectangles': {'min_size': 10, 'border_width': 1}
}

def check_boundaries(x, y):
    """检查坐标是否在安全边界内"""
    return (OPTIMAL_BOUNDARIES['left'] <= x <= OPTIMAL_BOUNDARIES['right'] and 
            OPTIMAL_BOUNDARIES['top'] <= y <= OPTIMAL_BOUNDARIES['bottom'])

def clamp_to_safe_area(x, y):
    """将坐标限制到安全区域内"""
    safe_x = max(OPTIMAL_BOUNDARIES['left'], min(x, OPTIMAL_BOUNDARIES['right']))
    safe_y = max(OPTIMAL_BOUNDARIES['top'], min(y, OPTIMAL_BOUNDARIES['bottom']))
    return safe_x, safe_y

def get_safe_text_position(x, y):
    """获取安全的文本绘制位置，考虑文字尺寸"""
    # 确保文字不会超出右边界和下边界
    safe_x = max(OPTIMAL_BOUNDARIES['left'], 
                min(x, OPTIMAL_BOUNDARIES['right'] - TEXT_LAYOUT['char_width'] * 10))
    safe_y = max(OPTIMAL_BOUNDARIES['top'], 
                min(y, OPTIMAL_BOUNDARIES['bottom'] - TEXT_LAYOUT['char_height']))
    return safe_x, safe_y 