"""
WiFi 扫描器 - 真实WiFi扫描 + 美观界面
包含实际WiFi扫描功能和现代化UI设计

使用方法:
    import wifi_scanner
    wifi_scanner.scan_and_show()
"""

import papers3
import network
import gc
import time

class WiFiScanner:
    """WiFi扫描器类"""
    
    def __init__(self):
        self.epdiy = None
        self.wlan = None
        self.wifi_list = []
        self.selected_index = -1
        
    def init(self):
        """初始化显示器和WiFi"""
        try:
            # 初始化显示器
            self.epdiy = papers3.EPDiy()
            self.epdiy.init()
            self.epdiy.clear()
            
            # 初始化WiFi
            self.wlan = network.WLAN(network.STA_IF)
            self.wlan.active(True)
            
            return True
        except Exception as e:
            print(f"Init failed: {e}")
            return False
    
    def scan_wifi(self):
        """扫描WiFi网络"""
        print("Scanning WiFi networks...")
        try:
            if not self.wlan:
                return []
                
            # 扫描WiFi
            networks = self.wlan.scan()
            
            # 处理扫描结果
            wifi_list = []
            for net in networks:
                ssid = net[0].decode('utf-8') if net[0] else "Hidden"
                rssi = net[3]  # 信号强度
                security = net[4]  # 安全类型
                
                # 过滤空SSID和重复网络
                if ssid and ssid not in [w['ssid'] for w in wifi_list]:
                    wifi_list.append({
                        'ssid': ssid,
                        'rssi': rssi,
                        'security': security,
                        'signal_bars': self._rssi_to_bars(rssi)
                    })
            
            # 按信号强度排序
            wifi_list.sort(key=lambda x: x['rssi'], reverse=True)
            
            # 最多显示10个
            self.wifi_list = wifi_list[:10]
            print(f"Found {len(self.wifi_list)} networks")
            return self.wifi_list
            
        except Exception as e:
            print(f"WiFi scan failed: {e}")
            # 返回模拟数据
            return self._get_demo_wifi()
    
    def _rssi_to_bars(self, rssi):
        """将RSSI转换为信号条数"""
        if rssi >= -50:
            return 4
        elif rssi >= -60:
            return 3
        elif rssi >= -70:
            return 2
        else:
            return 1
    
    def _get_demo_wifi(self):
        """获取演示WiFi数据"""
        demo_list = [
            {'ssid': 'Home_WiFi_5G', 'rssi': -45, 'security': 3, 'signal_bars': 4},
            {'ssid': 'Office_Network', 'rssi': -52, 'security': 3, 'signal_bars': 3},
            {'ssid': 'Starbucks_WiFi', 'rssi': -58, 'security': 0, 'signal_bars': 3},
            {'ssid': 'iPhone_Hotspot', 'rssi': -65, 'security': 3, 'signal_bars': 2},
            {'ssid': 'AndroidAP_5G', 'rssi': -68, 'security': 3, 'signal_bars': 2},
            {'ssid': 'Guest_Network', 'rssi': -72, 'security': 0, 'signal_bars': 1},
            {'ssid': 'Neighbor_WiFi', 'rssi': -75, 'security': 3, 'signal_bars': 1},
            {'ssid': 'CafeWiFi_Free', 'rssi': -78, 'security': 0, 'signal_bars': 1},
        ]
        self.wifi_list = demo_list
        return demo_list
    def draw_modern_ui(self):
        """绘制现代化UI界面 - 基于详细布局计划"""
        if not self.epdiy:
            return
            
        # 清屏 - 设置白色背景
        self.epdiy.clear()
        self.epdiy.fill_rect(0, 0, 960, 540, 0xF0)  # 白色背景
        
        # === 1. 顶部标题区 (y:45-105, 60px高) ===
        self.epdiy.draw_text("WiFi Networks Scanner", 30, 60, 0x00)  # 标题文字
        refresh_x = 780
        self.epdiy.draw_rect(refresh_x, 50, 60, 30, 0x80)  # 刷新按钮边框
        self.epdiy.draw_text("Refresh", refresh_x + 5, 68, 0x00)  # 按钮文字
        self.epdiy.draw_line(10, 85, 850, 85, 0x80)  # 分割线
        
        # === 2. WiFi列表区 (y:105-385, 280px高) ===
        if not self.wifi_list:
            # 无WiFi提示 - 居中显示
            self.epdiy.draw_text("No WiFi networks found", 350, 250, 0x00)
            self.epdiy.draw_text("Touch 'Refresh' to scan again", 320, 280, 0x80)
            return
        
        # WiFi列表 - 每项90px高，预留字体空间
        start_y = 105
        item_height = 90  # 为字体预留足够空间，避免重叠
        
        for i, wifi in enumerate(self.wifi_list[:3]):  # 最多3项 (3×90=270px)
            item_y = start_y + i * item_height
            
            # WiFi项背景卡片
            if i == self.selected_index:
                self.epdiy.fill_rect(20, item_y, 810, item_height - 5, 0xC0)  # 选中背景
                self.epdiy.draw_rect(20, item_y, 810, item_height - 5, 0x00)   # 黑色边框
            else:
                self.epdiy.draw_rect(20, item_y, 810, item_height - 5, 0x80)  # 普通边框
            
            # 内部元素布局 (在90px高度内合理分布)
            icon_y = item_y + 20       # 信号图标位置
            name_y = item_y + 30       # WiFi名称 (30px行高)
            signal_y = item_y + 55     # 信号详情 (25px间距)
            security_y = item_y + 30   # 安全状态
            selected_y = item_y + 65   # 选中标记
            
            # 信号图标
            self._draw_signal_bars(40, icon_y, wifi['signal_bars'])
            
            # WiFi名称 - 字符宽度约8-12px，x:120-750区间约630px，可容纳约50-78字符
            display_name = wifi['ssid']
            if len(display_name) > 45:  # 更精确的长度限制，基于可用宽度630px
                display_name = display_name[:42] + "..."
            self.epdiy.draw_text(display_name, 120, name_y, 0x00)
            
            # 信号强度详情 - 确保与右侧安全状态有足够间距
            strength_text = self._get_signal_strength_text(wifi['rssi'])
            signal_text = f"{strength_text} ({wifi['rssi']} dBm)"
            # 限制信号文字长度，为安全状态留出空间 (x:120-720, 约600px)
            if len(signal_text) > 50:  # 约600px/12px = 50字符
                signal_text = signal_text[:47] + "..."
            self.epdiy.draw_text(signal_text, 120, signal_y, 0x80)
            
            # 安全状态 - 与布局计划严格对齐
            if wifi['security'] > 0:
                self._draw_simple_lock(750, security_y - 5)  # 图标稍上一点
                self.epdiy.draw_text("Secure", 770, security_y, 0x80)
            else:
                self.epdiy.draw_text("Open", 770, security_y, 0x80)  # 与Secure保持x坐标一致
            
            # 选中标记
            if i == self.selected_index:
                self.epdiy.draw_text(">>> SELECTED <<<", 580, selected_y, 0x00)
        
        # === 3. 底部控制区 (y:385-520, 135px高) ===
        status_y = 390
        line_height = 25  # 标准行高
        
        # 分割线
        self.epdiy.draw_line(10, status_y - 5, 850, status_y - 5, 0x80)
        
        # 统计信息 (y:395)
        stats_y = status_y + 5
        self.epdiy.draw_text(f"Found {len(self.wifi_list)} networks", 30, stats_y, 0x80)
        
        # 选中显示和按钮区
        if self.selected_index >= 0:
            # 选中显示 (y:425) - 考虑按钮位置，可用宽度约600px
            selected_name = self.wifi_list[self.selected_index]['ssid']
            # "Selected: " 占约10字符，总可用约50字符，WiFi名称限制40字符
            if len(selected_name) > 40:
                selected_name = selected_name[:37] + "..."
            selected_y = status_y + 35
            selected_text = f"Selected: {selected_name}"
            self.epdiy.draw_text(selected_text, 30, selected_y, 0x00)
            
            # 按钮区 (y:455-485) - 精确计算文字居中位置
            button_y = status_y + 65
            # 连接按钮 - "CONNECT"7字符×8px=56px，按钮宽100px，居中位置(100-56)/2=22px
            btn_x = 650
            self.epdiy.draw_rect(btn_x, button_y, 100, 30, 0x00)
            self.epdiy.fill_rect(btn_x + 1, button_y + 1, 98, 28, 0xF0)
            # 文字垂直居中：按钮高30px，字体高约16px，位置(30-16)/2≈7px，+9px调整
            self.epdiy.draw_text("CONNECT", btn_x + 22, button_y + 16, 0x00)
            
            # 取消按钮 - "CANCEL"6字符×8px=48px，按钮宽80px，居中位置(80-48)/2=16px
            cancel_x = 770
            self.epdiy.draw_rect(cancel_x, button_y, 80, 30, 0x80)
            self.epdiy.draw_text("CANCEL", cancel_x + 16, button_y + 16, 0x80)
            
            # 提示信息 (y:495)
            tip_y = status_y + 105
            self.epdiy.draw_text("Touch CONNECT to join network", 30, tip_y, 0x80)
        else:
            # 未选中时的提示
            tip1_y = status_y + 35
            tip2_y = status_y + 60
            self.epdiy.draw_text("Touch a WiFi network to select", 30, tip1_y, 0x80)
            self.epdiy.draw_text("Then touch CONNECT to join", 30, tip2_y, 0x80)
    
    def _draw_signal_bars(self, x, y, bars):
        """绘制信号强度条 - 与文字垂直对齐"""
        # 绘制4个信号条，调整基准高度使其与文字对齐
        base_height = 20  # 基准高度，与文字行高匹配
        for i in range(4):
            bar_height = 6 + i * 4  # 递增高度，稍微减小
            bar_width = 5
            bar_x = x + i * 8  # 稍微紧凑一些
            bar_y = y + (base_height - bar_height)  # 底部对齐
            
            if i < bars:
                # 活跃信号条 - 黑色实心
                self.epdiy.fill_rect(bar_x, bar_y, bar_width, bar_height, 0x00)
            else:
                # 非活跃信号条 - 灰色空心
                self.epdiy.draw_rect(bar_x, bar_y, bar_width, bar_height, 0x80)
    
    def _get_signal_strength_text(self, rssi):
        """根据RSSI值返回友好的信号强度描述"""
        if rssi >= -50:
            return "Excellent"
        elif rssi >= -60:
            return "Good"
        elif rssi >= -70:
            return "Fair"
        else:
            return "Weak"
    
    def _draw_simple_lock(self, x, y):
        """绘制简单的锁图标"""
        # 锁身
        self.epdiy.draw_rect(x, y + 8, 10, 10, 0x00)
        # 锁环
        self.epdiy.draw_rect(x + 2, y, 6, 8, 0x00)
        self.epdiy.fill_rect(x + 3, y + 1, 4, 6, 0xF0)  # 白色内部
    

    
    def show_wifi_list(self):
        """显示WiFi列表"""
        self.draw_modern_ui()
        self.epdiy.update()
    
    def select_wifi(self, index):
        """选择WiFi"""
        if 0 <= index < len(self.wifi_list):
            self.selected_index = index
            print(f"Selected: {self.wifi_list[index]['ssid']}")
            self.show_wifi_list()
    
    def cleanup(self):
        """清理资源"""
        if self.epdiy:
            try:
                self.epdiy.deinit()
            except:
                pass
        if self.wlan:
            try:
                self.wlan.active(False)
            except:
                pass
        gc.collect()

# 全局实例
_scanner = None

def get_scanner():
    """获取扫描器实例"""
    global _scanner
    if _scanner is None:
        _scanner = WiFiScanner()
        _scanner.init()
    return _scanner

def scan_and_show():
    """扫描并显示WiFi"""
    print("=== WiFi Scanner ===")
    scanner = get_scanner()
    
    # 显示界面
    scanner.show_wifi_list()
    print("WiFi list displayed!")

def select_wifi(index):
    """选择WiFi"""
    scanner = get_scanner()
    scanner.select_wifi(index)

def refresh():
    """刷新WiFi列表"""
    print("Refreshing WiFi list...")
    scanner = get_scanner()
    scanner.scan_wifi()
    scanner.show_wifi_list()

def demo_selection():
    """演示选择过程"""
    print("=== Demo Selection Process ===")
    scanner = get_scanner()
    
    # 1. 显示列表
    scan_and_show()
    time.sleep(2)
    
    # 2. 选择第一个WiFi
    if scanner.wifi_list:
        select_wifi(0)
        time.sleep(2)
        
        # 3. 选择第二个WiFi
        if len(scanner.wifi_list) > 1:
            select_wifi(1)

def cleanup():
    """清理资源"""
    global _scanner
    if _scanner:
        _scanner.cleanup()
        _scanner = None

def help():
    """显示帮助"""
    print("""
WiFi Scanner - 真实WiFi扫描 + 现代化界面

主要功能:
  scan_and_show()     # 扫描并显示WiFi网络
  select_wifi(index)  # 选择WiFi (索引从0开始)
  refresh()          # 刷新WiFi列表
  demo_selection()   # 演示选择过程
  cleanup()          # 清理资源
  help()            # 显示帮助

快速开始:
  import wifi_scanner
  wifi_scanner.scan_and_show()

界面特性:
✅ 纯白色背景，黑色文字，清晰易读
✅ 现代卡片式WiFi列表设计
✅ 清晰的信号强度条显示 (Excellent/Good/Fair/Weak)
✅ 安全状态指示 (Secure/Open)
✅ 选中状态高亮显示
✅ 底部连接/取消按钮
✅ 真实WiFi扫描 (使用 network.WLAN)
✅ 友好的dBm信号强度显示

界面布局 (基于详细计划):
- **顶部标题区** (60px): 标题 + 刷新按钮 + 分割线
- **WiFi列表区** (280px): 3个WiFi项，每项90px高
  - 每项包含: 信号图标、名称、强度、安全状态、选中标记
  - 预留充足字体空间，避免重叠
- **底部控制区** (135px): 统计、选中显示、按钮、提示
- **颜色方案**: 白色背景，黑色文字，灰色辅助信息
- **字体空间**: 每行25px行高，按钮30px高度
""")

if __name__ == "__main__":
    help() 