# Papers3 Simple Boundary Test (Fixed v2)
# Fixes: Title vertical position, Center text alignment, Ghosting logic

import papers3
import time
import gc

W = 960
H = 540

def main():
    print("Initializing EPD...")
    epd = papers3.EPDiy()
    epd.init()
    
    # --- 修复1: 启动时显式清屏 ---
    # 目的: 清除上一轮运行留下的残影 (Ghosting)
    print("Clearing screen (hard reset)...")
    if hasattr(epd, 'clear'):
        epd.clear()
    else:
        epd.fill_rect(0, 0, W, H, 255)
        epd.update()
    time.sleep(1)
    
    patterns = [
        ("1: Corners", lambda: (
            # 四角黑块 (80x80)，用于测试屏幕边缘显示是否完整
            epd.fill_rect(0, 0, 80, 80, 0),
            epd.fill_rect(W-80, 0, 80, 80, 0),
            epd.fill_rect(0, H-80, 80, 80, 0),
            epd.fill_rect(W-80, H-80, 80, 80, 0)
        )),
        ("2: Border", lambda: (
            epd.draw_rect(5, 5, W-10, H-10, 0),
            epd.draw_rect(20, 20, W-40, H-40, 0)
        )),
        ("3: Cross", lambda: (
            epd.draw_line(0, H//2, W, H//2, 0),
            epd.draw_line(W//2, 0, W//2, H, 0),
            epd.fill_circle(W//2, H//2, 30, 0)
        )),
        ("4: Circles", lambda: [epd.draw_circle(W//2, H//2, r, 0) for r in range(30, 200, 25)]),
        ("5: Gray", lambda: [epd.fill_rect(i*60, 150, 60, 200, i*16) for i in range(16)]),
        ("6: Diagonal", lambda: (
            epd.draw_line(0, 0, W, H, 0),
            epd.draw_line(W, 0, 0, H, 0)
        )),
        ("7: Shapes", lambda: (
            epd.fill_rect(50, 150, 150, 150, 0),
            epd.fill_circle(400, 225, 60, 0),
            epd.fill_triangle(550, 300, 700, 300, 625, 150, 0)
        )),
        ("8: Text", lambda: (
            # --- 修复3: 文字布局微调 ---
            # Top: Y=90，避开顶部的 Title (Y=55)
            # Center: Y=H//2 - 24，实现视觉垂直居中 (原 H//2 会导致文字偏下)
            # Side: X=W-240，防止长单词在右侧被切断
            epd.draw_text("TopLeft", 20, 90, 0),
            epd.draw_text("TopRight", W-240, 90, 0),
            epd.draw_text("Center", W//2-60, H//2-24, 0), 
            epd.draw_text("BotLeft", 20, H-80, 0),
            epd.draw_text("BotRight", W-240, H-80, 0)
        )),
        ("9: Grid", lambda: (
            [epd.draw_line(x, 0, x, H, 100) for x in range(0, W, 100)],
            [epd.draw_line(0, y, W, y, 100) for y in range(0, H, 100)]
        )),
        ("10: Done", lambda: (
            epd.draw_rect(100, 150, W-200, H-300, 0),
            epd.draw_text("Test Complete!", 300, 250, 0),
            epd.draw_text("Papers3 OK", 320, 330, 0)
        ))
    ]
    
    for i, (title, draw_fn) in enumerate(patterns):
        print(f"Pattern {title}")
        
        # 每一屏都用白色填充背景，尽可能减少残影
        epd.fill_rect(0, 0, W, H, 255)
        
        # --- 修复2: 标题位置优化 ---
        # 逻辑: 
        # 1. 水平居中 (center_x): 避免与 Pattern 1 的左上角黑块重叠
        # 2. 垂直位置 (Y=55): 之前 Y=30 太靠上，现下移至 55，留出视觉呼吸空间
        est_width = len(title) * 15 
        center_x = (W - est_width) // 2
        epd.draw_text(title, center_x, 55, 0)
        
        draw_fn()
        epd.update()
        time.sleep(2)
    
    print("Cleanup...")
    epd.deinit()
    gc.collect()
    print("Done!")

if __name__ == '__main__':
    main()