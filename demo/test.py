# Papers3 Conference Room Display Panel (Chinese Version)
# 会议室门牌显示系统 - 中文优化版
# 改进：中文界面、模拟粗体、栅格对齐排版、反色状态块

import papers3
import time
import gc

# 屏幕常量
W = 960
H = 540

# 颜色常量 (假设 0=黑, 255=白, 具体视硬件而定)
BLACK = 0
WHITE = 255

class ConferenceDisplay:
    def __init__(self):
        print("正在初始化会议显示屏...")
        self.epd = papers3.EPDiy()
        self.epd.init()
        
        # 内部状态
        self.room_name = "第一会议室"
        self.wifi_bars = 3
        self.battery_pct = 100
        
        # 会议数据
        self.current_meeting = None 
        self.next_meeting = None
        
        self.clear_screen()

    def clear_screen(self):
        """完全清屏"""
        if hasattr(self.epd, 'clear'):
            self.epd.clear()
        else:
            self.epd.fill_rect(0, 0, W, H, WHITE)
            self.epd.update()

    # --- 辅助绘图函数 ---

    def _draw_text_bold(self, text, x, y, color=BLACK):
        """模拟粗体文字 (通过微移重绘)"""
        # 原位
        self.epd.draw_text(text, x, y, color)
        # 右移1像素
        self.epd.draw_text(text, x + 1, y, color)
        # 下移1像素 (可选，会让字变得更厚重)
        self.epd.draw_text(text, x, y + 1, color)

    def _draw_box_text(self, text, cx, cy, w, h, invert=False):
        """绘制带框文字 (用于状态标签)"""
        # cx, cy 是中心点
        x = cx - w // 2
        y = cy - h // 2
        
        if invert:
            # 黑底白字
            self.epd.fill_rect(x, y, w, h, BLACK)
            # 简单的居中估算 (假设汉字宽约30, 英文15)
            # 实际需要根据字数微调
            text_w = len(text) * 15 
            self.epd.draw_text(text, cx - text_w // 2, cy - 12, WHITE)
            self._draw_text_bold(text, cx - text_w // 2, cy - 12, WHITE)
        else:
            # 白底黑框
            self.epd.draw_rect(x, y, w, h, BLACK)
            self.epd.draw_rect(x+1, y+1, w-2, h-2, BLACK) # 加粗框
            text_w = len(text) * 15
            self._draw_text_bold(text, cx - text_w // 2, cy - 12, BLACK)

    # --- 业务逻辑 ---

    def set_room_info(self, name):
        self.room_name = name

    def update_status(self, wifi=None, battery=None):
        if wifi is not None: self.wifi_bars = wifi
        if battery is not None: self.battery_pct = battery

    def set_current_meeting(self, title, time_range, host=None):
        if title is None:
            self.current_meeting = None
        else:
            self.current_meeting = {
                "title": title,
                "time": time_range,
                "host": host if host else "未知"
            }

    def set_next_meeting(self, title, time_range):
        if title is None:
            self.next_meeting = None
        else:
            self.next_meeting = {
                "title": title,
                "time": time_range
            }

    # --- 模块绘制 ---

    def _draw_header(self):
        """顶部栏: 40px margin"""
        margin_x = 40
        y_base = 30
        
        # 1. 顶部黑条背景 (提升质感)
        self.epd.fill_rect(0, 0, W, 80, BLACK)
        
        # 2. 会议室名称 (白字粗体)
        self._draw_text_bold(self.room_name, margin_x, y_base, WHITE)
        
        # 3. 状态图标 (白线)
        # WiFi
        right_margin = W - 40
        self._draw_wifi(right_margin - 40, y_base, self.wifi_bars, color=WHITE)
        # 电量
        self._draw_battery(right_margin - 120, y_base + 2, self.battery_pct, color=WHITE)

    def _draw_wifi(self, x, y, bars, color=BLACK):
        max_h = 24
        bar_w = 6
        gap = 4
        for i in range(4):
            bx = x + i * (bar_w + gap)
            h = int(max_h * ((i + 1) / 4))
            by = y + (max_h - h)
            
            if i < bars:
                self.epd.fill_rect(bx, by, bar_w, h, color)
            else:
                self.epd.draw_rect(bx, by, bar_w, h, color)

    def _draw_battery(self, x, y, pct, color=BLACK):
        w, h = 40, 20
        self.epd.draw_rect(x, y, w, h, color)
        self.epd.draw_rect(x+w, y+5, 4, 10, color)
        fill_w = int((w - 4) * (pct / 100))
        if fill_w > 0:
            self.epd.fill_rect(x+2, y+2, fill_w, h-4, color)

    def _draw_main_area(self):
        """主内容区"""
        center_y = (H + 80) // 2 # 除去顶部header后的中心
        
        if self.current_meeting is None:
            # --- 空闲状态 ---
            # 绘制一个巨大的圆角矩形(视觉上的)状态框
            box_w, box_h = 400, 120
            self._draw_box_text("当前空闲", W//2, center_y - 20, box_w, box_h, invert=False)
            self.epd.draw_text("随时可预订使用", W//2 - 90, center_y + 60, BLACK)
            
        else:
            # --- 会议进行中 ---
            m = self.current_meeting
            margin_left = 60
            y_start = 140
            
            # 1. 左侧粗竖线 (装饰)
            self.epd.fill_rect(margin_left, y_start, 10, 220, BLACK)
            
            content_x = margin_left + 40
            
            # 2. 状态标签 (黑底白字小标签)
            self.epd.fill_rect(content_x, y_start, 120, 30, BLACK)
            self.epd.draw_text("会议进行中", content_x + 15, y_start + 5, WHITE)
            
            # 3. 时间段 (超大粗体)
            # 模拟更大字体: 手动绘制两行或间距
            time_str = m['time']
            self._draw_text_bold(time_str, content_x, y_start + 50, BLACK)
            
            # 4. 分割线
            self.epd.draw_line(content_x, y_start + 90, W - 60, y_start + 90, BLACK)
            
            # 5. 会议主题 (重点)
            self._draw_text_bold(m['title'], content_x, y_start + 110, BLACK)
            # 再次加粗一行以突出
            self.epd.draw_text(m['title'], content_x + 1, y_start + 111, BLACK)
            
            # 6. 组织者
            self.epd.draw_text(f"组织者: {m['host']}", content_x, y_start + 170, BLACK)

    def _draw_footer(self):
        """底部预告"""
        y_line = 440
        self.epd.draw_line(40, y_line, W-40, y_line, BLACK)
        self.epd.draw_line(40, y_line+1, W-40, y_line+1, BLACK) # 双线
        
        y_text = y_line + 30
        margin_x = 40
        
        # 标签
        self._draw_text_bold("下场预告:", margin_x, y_text, BLACK)
        
        if self.next_meeting:
            nm = self.next_meeting
            # 用 " | " 分隔时间与主题
            info_x = margin_x + 120
            self.epd.draw_text(f"{nm['time']}   |   {nm['title']}", info_x, y_text, BLACK)
        else:
            self.epd.draw_text("今日已无后续会议", margin_x + 120, y_text, BLACK)

    def refresh(self):
        print("正在刷新屏幕...")
        self.epd.fill_rect(0, 0, W, H, WHITE)
        
        self._draw_header()
        self._draw_main_area()
        self._draw_footer()
        
        self.epd.update()
        print("刷新完成")

def main():
    panel = ConferenceDisplay()
    
    # 场景 1: 会议进行中
    print("--- 场景 1: 会议中 ---")
    panel.set_room_info("大会议室 A101")
    panel.update_status(wifi=3, battery=88)
    
    panel.set_current_meeting(
        title="2025年度 Q1 产品战略规划会", # 中文标题
        time_range="14:00 - 16:30",
        host="产品总监 - 王伟"
    )
    panel.set_next_meeting("技术架构评审", "17:00 - 18:00")
    panel.refresh()
    
    time.sleep(3)
    
    # 场景 2: 空闲
    print("--- 场景 2: 空闲 ---")
    panel.update_status(wifi=4, battery=87)
    panel.set_current_meeting(None, None)
    # 下一场会议依然存在
    panel.refresh()
    
    time.sleep(3)
    
    panel.epd.deinit()
    gc.collect()
    print("演示结束")

if __name__ == '__main__':
    main()