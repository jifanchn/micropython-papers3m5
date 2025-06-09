"""
Papers3 显示规格使用示例
展示如何使用 papers3.display_specs 模块中的常量和函数
"""

import papers3
from display_specs import (
    OPTIMAL_BOUNDARIES, SAFE_AREA, TEXT_LAYOUT, COLORS,
    check_boundaries, get_safe_text_position
)

def demo_safe_boundaries():
    """演示安全边界的使用"""
    print("=== 演示安全边界使用 ===")
    
    # 初始化显示器
    epdiy = papers3.EPDiy()
    epdiy.init()
    epdiy.clear()
    
    # 绘制安全区域边框
    epdiy.draw_rect(
        SAFE_AREA['x'], SAFE_AREA['y'], 
        SAFE_AREA['width'], SAFE_AREA['height'], 
        COLORS['GRAY']
    )
    
    # 在安全区域内绘制文本
    texts = [
        "SAFE AREA DEMO",
        f"Left: {OPTIMAL_BOUNDARIES['left']}",
        f"Right: {OPTIMAL_BOUNDARIES['right']}",
        f"Top: {OPTIMAL_BOUNDARIES['top']}",
        f"Bottom: {OPTIMAL_BOUNDARIES['bottom']}",
        f"Size: {SAFE_AREA['width']}x{SAFE_AREA['height']}"
    ]
    
    y = TEXT_LAYOUT['start_y']
    for text in texts:
        safe_x, safe_y = get_safe_text_position(TEXT_LAYOUT['start_x'], y)
        epdiy.draw_text(text, safe_x, safe_y, COLORS['BLACK'])
        print(f"Draw: '{text}' at ({safe_x},{safe_y})")
        y += TEXT_LAYOUT['line_spacing']
    
    epdiy.update()
    epdiy.deinit()

def demo_color_usage():
    """演示颜色常量的使用"""
    print("=== 演示颜色常量使用 ===")
    
    epdiy = papers3.EPDiy()
    epdiy.init()
    epdiy.clear()
    
    # 使用不同颜色绘制
    y = 80
    color_demos = [
        ("BLACK TEXT", COLORS['BLACK']),
        ("DARK GRAY", COLORS['DARK_GRAY']),
        ("GRAY TEXT", COLORS['GRAY']),
        ("LIGHT GRAY", COLORS['LIGHT_GRAY'])
    ]
    
    for text, color in color_demos:
        epdiy.draw_text(text, 100, y, color)
        print(f"Color {color}: {text}")
        y += 60
    
    # 绘制不同颜色的形状
    epdiy.draw_circle(300, 150, 30, COLORS['BLACK'])
    epdiy.draw_circle(400, 150, 30, COLORS['GRAY'])
    epdiy.draw_circle(500, 150, 30, COLORS['LIGHT_GRAY'])
    
    epdiy.update()
    epdiy.deinit()

def demo_boundary_check():
    """演示边界检查功能"""
    print("=== 演示边界检查 ===")
    
    # 测试不同坐标
    test_coords = [
        (5, 40),      # 超出边界
        (10, 45),     # 正好在边界
        (100, 100),   # 安全区域内
        (850, 520),   # 右下边界
        (900, 530)    # 超出边界
    ]
    
    for x, y in test_coords:
        is_safe = check_boundaries(x, y)
        safe_x, safe_y = get_safe_text_position(x, y)
        status = "SAFE" if is_safe else "UNSAFE"
        print(f"({x},{y}) -> {status}, Safe pos: ({safe_x},{safe_y})")

def demo_text_layout():
    """演示文本布局标准"""
    print("=== 演示文本布局标准 ===")
    
    epdiy = papers3.EPDiy()
    epdiy.init()
    epdiy.clear()
    
    # 使用标准布局参数
    x = TEXT_LAYOUT['start_x']
    y = TEXT_LAYOUT['start_y']
    spacing = TEXT_LAYOUT['line_spacing']
    
    lines = [
        "LAYOUT STANDARD",
        f"Start: ({x},{y})",
        f"Spacing: {spacing}px",
        f"Max chars: {TEXT_LAYOUT['max_chars_per_line']}",
        "Line 5 example",
        "Line 6 example"
    ]
    
    # 绘制参考线
    epdiy.draw_line(x-10, 0, x-10, 540, COLORS['LIGHT_GRAY'])
    
    current_y = y
    for i, line in enumerate(lines):
        # 检查是否超出安全区域
        if current_y > OPTIMAL_BOUNDARIES['bottom'] - TEXT_LAYOUT['char_height']:
            epdiy.draw_text("...(truncated)", x, current_y, COLORS['GRAY'])
            break
            
        epdiy.draw_text(line, x, current_y, COLORS['BLACK'])
        
        # 绘制行号
        epdiy.draw_text(str(i+1), x-30, current_y, COLORS['GRAY'])
        
        print(f"Line {i+1}: '{line}' at ({x},{current_y})")
        current_y += spacing
    
    epdiy.update()
    epdiy.deinit()

def help():
    """显示帮助信息"""
    print("""
=== Papers3 显示规格使用示例 ===

可用函数:
  demo_safe_boundaries()    # 演示安全边界使用
  demo_color_usage()        # 演示颜色常量使用  
  demo_boundary_check()     # 演示边界检查功能
  demo_text_layout()        # 演示文本布局标准
  help()                    # 显示此帮助

使用示例:
  import display_specs_example as dse
  dse.demo_safe_boundaries()
  dse.demo_color_usage()

 显示规格常量:
   from display_specs import OPTIMAL_BOUNDARIES, COLORS
   print(OPTIMAL_BOUNDARIES)
   print(COLORS)

详细文档: docs/DISPLAY_SPECS.md
""")

if __name__ == "__main__":
    help() 