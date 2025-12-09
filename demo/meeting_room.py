# Papers3 Conference Room Display V2 (Design Overhaul)
# 重新设计的会议室门牌系统
# 特性：深度清屏、块状UI设计、防冲突布局、进度条显示

import papers3
import time
import gc

# 屏幕常量
W = 960
H = 540

# 颜色
BLACK = 0
WHITE = 255

class ConferenceDisplayV2:
    def __init__(self):
        print("初始化会议显示屏 V2...")
        self.epd = papers3.EPDiy()
        self.epd.init()
        
        # 内部状态
        self.room_name = "第一会议室 | Room A1"
        self.wifi_bars = 3
        self.battery_pct = 100
        
        # 会议数据
        self.current_meeting = None 
        self.next_meeting = None
        
        # 启动时执行深度清洁
        self.deep_clean()

    def deep_clean(self):
        """深度清屏：全黑 -> 全白，去除顽固残影"""
        print("执行深度清屏...")
        # 1. 全黑
        self.epd.fill_rect(0, 0, W, H, BLACK)
        self.epd.update()
        time.sleep(0.5)
        
        # 2. 全白
        self.epd.fill_rect(0, 0, W, H, WHITE)
        self.epd.update()
        time.sleep(0.5)

    # --- 绘图工具 ---

    def _draw_text_bold(self, text, x, y, color=BLACK, thickness=1):
        """绘制加粗文字"""
        self.epd.draw_text(text, x, y, color)
        if thickness >= 1:
            self.epd.draw_text(text, x + 1, y, color)
        if thickness >= 2:
            self.epd.draw_text(text, x, y + 1, color)
            self.epd.draw_text(text, x + 1, y + 1, color)

    def _fill_rect_safe(self, x, y, w, h, color):
        """安全的矩形填充"""
        self.epd.fill_rect(int(x), int(y), int(w), int(h), color)

    # --- 模块绘制 ---

    def _draw_header(self):
        """顶部通栏 (黑底白字) - 高度 90px"""
        h = 90
        self._fill_rect_safe(0, 0, W, h, BLACK)
        
        # 左侧：会议室名 (垂直居中)
        self._draw_text_bold(self.room_name, 40, 35, WHITE, thickness=2)
        
        # 右侧：状态区
        # 电量 (实心块设计)
        bat_x = W - 100
        bat_y = 30
        self.epd.draw_rect(bat_x, bat_y, 40, 20, WHITE)
        self.epd.fill_rect(bat_x + 40, bat_y + 5, 4, 10, WHITE)
        fill_w = int(36 * (self.battery_pct / 100))
        self.epd.fill_rect(bat_x + 2, bat_y + 2, fill_w, 16, WHITE)
        self.epd.draw_text(f"{self.battery_pct}%", bat_x - 50, bat_y, WHITE)
        
        # WiFi
        wifi_x = W - 180
        for i in range(4):
            h_bar = 6 + i * 5
            x_bar = wifi_x + i * 10
            if i < self.wifi_bars:
                self.epd.fill_rect(x_bar, 50 - h_bar, 6, h_bar, WHITE)
            else:
                self.epd.draw_rect(x_bar, 50 - h_bar, 6, h_bar, WHITE)

    def _draw_status_sidebar(self, status_text, bg_color, text_color):
        """左侧状态侧边栏 (用于会议中)"""
        w = 260
        # 背景
        self._fill_rect_safe(0, 90, w, H - 90, bg_color)
        
        # 状态文字
        self._draw_text_bold(status_text, 40, 150, text_color, thickness=2)
        
        # 装饰线
        line_y = 200
        self.epd.fill_rect(40, line_y, 100, 4, text_color)

    def _draw_main_area_free(self):
        """空闲状态布局"""
        # 背景保持白色
        
        # 中间的大框 (双层边框提升质感)
        cx, cy = W // 2, (H + 90) // 2
        box_w, box_h = 400, 160
        
        # 外框
        self.epd.draw_rect(cx - box_w//2, cy - box_h//2, box_w, box_h, BLACK)
        # 内框 (距离外框 6px)
        self.epd.draw_rect(cx - box_w//2 + 6, cy - box_h//2 + 6, box_w - 12, box_h - 12, BLACK)
        
        # 文字 (居中)
        status = "当前空闲"
        # 估算宽度居中
        text_x = cx - (len(status) * 15) // 2
        self._draw_text_bold(status, text_x - 20, cy - 20, BLACK, thickness=2)
        
        # 底部提示
        hint = "Available / 随时可预订"
        hint_x = cx - (len(hint) * 8) // 2
        self.epd.draw_text(hint, hint_x, cy + 120, BLACK)

    def _draw_main_area_busy(self):
        """会议中状态布局 - 分栏设计"""
        m = self.current_meeting
        
        # 1. 左侧黑色侧边栏 (显示时间)
        sidebar_w = 300
        self._fill_rect_safe(0, 90, sidebar_w, H - 90, BLACK)
        
        # "会议中" 标签
        self.epd.fill_rect(40, 130, 120, 40, WHITE)
        self._draw_text_bold("会议中", 55, 140, BLACK, thickness=1)
        
        # 时间显示 (左侧黑底白字)
        times = m['time'].split('-')
        start_time = times[0].strip()
        end_time = times[1].strip() if len(times) > 1 else ""
        
        self._draw_text_bold(start_time, 40, 220, WHITE, thickness=2)
        self.epd.fill_rect(65, 260, 4, 30, WHITE) # 竖线连接
        self._draw_text_bold(end_time, 40, 310, WHITE, thickness=2)
        
        # 2. 右侧白色内容区
        content_x = sidebar_w + 50
        y_start = 140
        
        # 会议主题 (大标题)
        self._draw_text_bold(m['title'], content_x, y_start, BLACK, thickness=2)
        self.epd.fill_rect(content_x, y_start + 40, 50, 4, BLACK) # 标题下划线
        
        # 组织者
        self.epd.draw_text(f"Host: {m['host']}", content_x, y_start + 80, BLACK)
        
        # 进度条
        # 假设当前是会议中间
        bar_y = 350
        bar_w = 500
        bar_h = 10
        self.epd.draw_rect(content_x, bar_y, bar_w, bar_h, BLACK)
        # 模拟 50% 进度
        self.epd.fill_rect(content_x, bar_y, int(bar_w * 0.5), bar_h, BLACK)
        self.epd.draw_text("Time Remaining: 30m", content_x, bar_y + 20, BLACK)

    def _draw_footer(self):
        """底部下一场预告 (灰色/线条区域)"""
        # 画一条贯穿的粗线
        line_y = H - 80
        self.epd.fill_rect(0, line_y, W, 2, BLACK)
        
        if self.next_meeting:
            nm = self.next_meeting
            label = "NEXT > "
            self._draw_text_bold(label, 40, line_y + 30, BLACK, thickness=1)
            
            info = f"{nm['time']} : {nm['title']}"
            self.epd.draw_text(info, 160, line_y + 30, BLACK)
        else:
            self.epd.draw_text("No further meetings today", 40, line_y + 30, BLACK)

    def refresh(self):
        print("刷新屏幕内容...")
        # 基础白底
        self.epd.fill_rect(0, 0, W, H, WHITE)
        
        self._draw_header()
        
        if self.current_meeting:
            self._draw_main_area_busy()
        else:
            self._draw_main_area_free()
            
        self._draw_footer()
        
        self.epd.update()
        print("刷新完成")

    # --- 接口函数 ---
    def set_data(self, room_name, current, next_m=None, wifi=3, bat=100):
        self.room_name = room_name
        self.current_meeting = current
        self.next_meeting = next_m
        self.wifi_bars = wifi
        self.battery_pct = bat

def main():
    display = ConferenceDisplayV2()
    
    # --- 场景 1: 会议进行中 ---
    print("展示场景: 会议中")
    display.set_data(
        room_name="第一会议室 Room 101",
        current={
            "title": "Q3 季度产品复盘会",
            "time": "14:00 - 16:00",
            "host": "Alice Chen"
        },
        next_m={
            "title": "技术架构评审",
            "time": "16:30 - 17:30"
        },
        wifi=4,
        bat=92
    )
    display.refresh()
    
    time.sleep(4)
    
    # --- 场景 2: 空闲状态 ---
    print("展示场景: 空闲")
    # 再次深度清屏，确保从黑块切换到白底时干净
    # (实际使用中视情况调用，这里为了演示效果强制调用)
    display.deep_clean() 
    
    display.set_data(
        room_name="第一会议室 Room 101",
        current=None, # 空闲
        next_m={
            "title": "技术架构评审",
            "time": "16:30 - 17:30"
        },
        wifi=4,
        bat=91
    )
    display.refresh()
    
    time.sleep(2)
    display.epd.deinit()
    gc.collect()

if __name__ == '__main__':
    main()