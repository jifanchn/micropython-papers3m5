"""
Papers3 屏幕范围测试程序
用于测试屏幕的显示范围、文本布局和坐标系统

功能特点:
- 可调节文本间距、起始位置、行数
- 显示坐标信息和屏幕边界
- 函数式调用，易于使用
- 支持中英文混合显示

使用方法:
    import screen_range_test as srt
    srt.test_basic()                    # 基础测试
    srt.test_grid()                     # 网格测试  
    srt.test_text_layout()              # 文本布局测试
    srt.test_custom(x=100, y=50, spacing=60, lines=8)  # 自定义测试
"""

import papers3
import time

class ScreenRangeTest:
    """屏幕范围测试类"""
    
    def __init__(self):
        """初始化测试类"""
        self.epdiy = None
        self.initialized = False
        self.width = 960   # Papers3屏幕宽度
        self.height = 540  # Papers3屏幕高度
        
    def init(self):
        """初始化EPD显示器"""
        try:
            self.epdiy = papers3.EPDiy()
            self.epdiy.init()
            self.initialized = True
            print("✅ EPD显示器初始化成功")
            print(f"📐 屏幕分辨率: {self.width} x {self.height}")
            return True
        except Exception as e:
            print(f"❌ EPD显示器初始化失败: {e}")
            return False
            
    def cleanup(self):
        """清理资源"""
        if self.epdiy:
            try:
                self.epdiy.deinit()
                print("✅ EPD显示器资源已清理")
            except:
                pass
        self.initialized = False
        
    def _check_init(self):
        """检查是否已初始化"""
        if not self.initialized:
            print("⚠️  请先调用 init() 初始化显示器")
            return False
        return True
        
    def _safe_draw_text(self, text, x, y, color=0):
        """安全绘制文本，自动处理边界"""
        try:
            # 确保坐标在最佳显示范围内
            x = max(10, min(x, 850))  # 左边界x=10，右边界x=850
            y = max(45, min(y, 520))  # 上边界y=45，下边界y=520
            
            # 限制文本长度
            if len(text) > 20:
                text = text[:17] + "..."
                
            self.epdiy.draw_text(text, x, y, color)
            return True
        except Exception as e:
            print(f"⚠️ 绘制文本失败: '{text}' at ({x},{y}) -> {e}")
            return False
            
    def test_basic(self):
        """基础屏幕范围测试"""
        if not self._check_init():
            return
            
        print("\n=== 基础屏幕范围测试 ===")
        
        try:
            # 清屏
            self.epdiy.clear()
            
            # 绘制屏幕边界
            self.epdiy.draw_rect(0, 0, self.width-1, self.height-1, 0)
            print(f"📐 绘制屏幕边界: (0,0) 到 ({self.width-1},{self.height-1})")
            
            # 修复的四个角落标记 - 确保文字不超出边界
            corners = [
                (10, 30, "左上(0,0)"),                    # 左上角，文字较短
                (self.width-100, 30, "右上"),              # 右上角，简化文字
                (10, self.height-40, "左下"),              # 左下角，简化文字  
                (self.width-100, self.height-40, "右下")   # 右下角，简化文字
            ]
            
            for x, y, text in corners:
                self._safe_draw_text(text, x, y)
                print(f"📍 {text} at ({x},{y})")
                
            # 中心点标记
            center_x, center_y = self.width // 2, self.height // 2
            self._safe_draw_text(f"中心点 ({center_x},{center_y})", center_x-60, center_y)
            self.epdiy.draw_circle(center_x, center_y, 20, 0)
            print(f"📍 中心点 ({center_x},{center_y})")
            
            # 更新显示
            self.epdiy.update()
            print("✅ 基础屏幕范围测试完成")
            
        except Exception as e:
            print(f"❌ 基础测试失败: {e}")
            
    def test_grid(self, grid_size=100):
        """网格测试 - 显示坐标网格"""
        if not self._check_init():
            return
            
        print(f"\n=== 网格测试 (间距: {grid_size}px) ===")
        
        try:
            # 清屏
            self.epdiy.clear()
            
            # 绘制垂直网格线
            for x in range(0, self.width, grid_size):
                self.epdiy.draw_line(x, 0, x, self.height-1, 8)  # 灰色线
                if x > 0 and x < self.width - 50:  # 避免右边界文字超出
                    self._safe_draw_text(str(x), x-15, 45, 8)  # 最佳位置：y=45
                    
            # 绘制水平网格线
            for y in range(0, self.height, grid_size):
                self.epdiy.draw_line(0, y, self.width-1, y, 8)  # 灰色线
                if y > 70 and y < self.height - 20:  # 避免与上方坐标重叠，从y=70开始，下方留20px
                    self._safe_draw_text(str(y), 10, y-5, 8)  # 最佳位置：x=10
                    
            # 绘制坐标轴（黑色加粗）
            self.epdiy.draw_line(0, 0, self.width-1, 0, 0)  # 顶边
            self.epdiy.draw_line(0, 0, 0, self.height-1, 0)  # 左边
            
            # 标题放在右上角最佳位置
            self._safe_draw_text(f"网格{grid_size}px", 850, 50, 0)
            
            # 更新显示
            self.epdiy.update()
            print(f"✅ 网格测试完成 (共 {len(range(0, self.width, grid_size))} x {len(range(0, self.height, grid_size))} 个格子)")
            
        except Exception as e:
            print(f"❌ 网格测试失败: {e}")
            
    def test_text_layout(self, start_x=80, start_y=50, line_spacing=70, num_lines=6):
        """文本布局测试 - 测试文本间距和排列"""
        if not self._check_init():
            return
            
        print(f"\n=== 文本布局测试 ===")
        print(f"参数: 起点({start_x},{start_y}), 行距{line_spacing}px, {num_lines}行")
        
        try:
            # 清屏
            self.epdiy.clear()
            
            # 绘制参考线，避开文字区域
            self.epdiy.draw_line(start_x-10, 0, start_x-10, self.height-1, 8)  # 垂直参考线左移
            
            # 测试文本行
            test_texts = [
                f"行1: 起点({start_x},{start_y})",
                f"行2: 间距{line_spacing}px", 
                "行3: Hello World",
                "行4: 中英文测试",
                f"行5: Papers3测试",
                f"行6: 屏幕{self.width}x{self.height}",
                "行7: 额外测试",
                "行8: 边界检查"
            ]
            
            current_y = start_y
            for i in range(min(num_lines, len(test_texts))):
                text = test_texts[i]
                
                # 检查是否超出屏幕范围
                if current_y > self.height - 40:
                    print(f"⚠️  第{i+1}行超出屏幕范围 (y={current_y})")
                    break
                    
                # 绘制文本
                success = self._safe_draw_text(text, start_x, current_y)
                if success:
                    print(f"📝 第{i+1}行: '{text}' at ({start_x},{current_y})")
                    
                    # 在左侧绘制行号标记，避免重叠
                    self.epdiy.draw_circle(start_x-25, current_y, 3, 8)
                    self._safe_draw_text(str(i+1), start_x-35, current_y-5, 8)
                else:
                    print(f"❌ 第{i+1}行绘制失败")
                    
                current_y += line_spacing
                
            # 绘制边界提示
            if current_y > self.height:
                boundary_y = self.height - 30
                self._safe_draw_text(f"屏幕底部边界 y={self.height}", start_x, boundary_y, 0)
                self.epdiy.draw_line(0, boundary_y + 20, self.width-1, boundary_y + 20, 0)
                
            # 更新显示
            self.epdiy.update()
            print(f"✅ 文本布局测试完成")
            
        except Exception as e:
            print(f"❌ 文本布局测试失败: {e}")
            
    def test_custom(self, start_x=100, start_y=80, line_spacing=70, num_lines=5, grid=False):
        """自定义测试 - 用户指定参数"""
        if not self._check_init():
            return
            
        print(f"\n=== 自定义测试 ===")
        print(f"参数: 起点({start_x},{start_y}), 行距{line_spacing}px, {num_lines}行, 网格={grid}")
        
        try:
            # 清屏
            self.epdiy.clear()
            
            # 可选择绘制网格
            if grid:
                for x in range(0, self.width, 100):
                    self.epdiy.draw_line(x, 0, x, self.height-1, 12)
                for y in range(0, self.height, 50):
                    self.epdiy.draw_line(0, y, self.width-1, y, 12)
                    
            # 绘制标题
            self._safe_draw_text("自定义屏幕范围测试", 20, 30, 0)
            
            # 绘制测试内容
            test_items = [
                f"测试起点: ({start_x}, {start_y})",
                f"行间距: {line_spacing} 像素",
                f"计划显示: {num_lines} 行文本",
                f"屏幕尺寸: {self.width} x {self.height}",
                "这是测试文本行"
            ]
            
            current_y = start_y
            for i in range(min(num_lines, len(test_items))):
                if current_y > self.height - 50:
                    self._safe_draw_text("⚠️ 超出显示范围", start_x, current_y, 0)
                    break
                    
                text = test_items[i] if i < len(test_items) else f"测试行 {i+1}"
                self._safe_draw_text(text, start_x, current_y, 0)
                
                # 绘制行号标记
                self.epdiy.draw_circle(start_x-30, current_y, 5, 8)
                self._safe_draw_text(str(i+1), start_x-35, current_y-8, 8)
                
                print(f"📝 行{i+1}: '{text}' at ({start_x},{current_y})")
                current_y += line_spacing
                
            # 绘制参数信息框
            info_y = self.height - 120
            self.epdiy.draw_rect(20, info_y, 300, 100, 8)
            self._safe_draw_text("参数信息:", 30, info_y + 20, 0)
            self._safe_draw_text(f"起点: ({start_x},{start_y})", 30, info_y + 50, 0)
            self._safe_draw_text(f"间距: {line_spacing}px", 30, info_y + 80, 0)
            
            # 更新显示
            self.epdiy.update()
            print("✅ 自定义测试完成")
            
        except Exception as e:
            print(f"❌ 自定义测试失败: {e}")
            
    def test_boundaries(self):
        """边界测试 - 测试屏幕边缘显示"""
        if not self._check_init():
            return
            
        print(f"\n=== 边界测试 ===")
        
        try:
            # 清屏
            self.epdiy.clear()
            
            # 测试各种边界位置
            boundary_tests = [
                # (x, y, text, description)
                (0, 0, "原点(0,0)", "左上角最边缘"),
                (5, 25, "近原点(5,25)", "左上角安全区域"),
                (self.width-100, 25, "右上(860,25)", "右上角区域"),
                (5, self.height-30, "左下(5,510)", "左下角区域"),
                (self.width-150, self.height-30, "右下(810,510)", "右下角区域"),
                (self.width//2, 5, "顶部中心", "顶部边缘"),
                (self.width//2, self.height-25, "底部中心", "底部边缘"),
            ]
            
            for x, y, text, desc in boundary_tests:
                # 确保坐标有效
                safe_x = max(0, min(x, self.width-10))
                safe_y = max(20, min(y, self.height-10))
                
                success = self._safe_draw_text(text, safe_x, safe_y, 0)
                if success:
                    print(f"✅ {desc}: '{text}' at ({safe_x},{safe_y})")
                    # 标记点
                    self.epdiy.draw_circle(safe_x-10, safe_y, 2, 8)
                else:
                    print(f"❌ {desc}: 绘制失败")
                    
            # 绘制完整边框
            self.epdiy.draw_rect(1, 1, self.width-2, self.height-2, 0)
            
            # 中心信息
            center_x, center_y = self.width//2, self.height//2
            self._safe_draw_text("边界测试中心", center_x-60, center_y-10, 0)
            self._safe_draw_text(f"屏幕: {self.width}x{self.height}", center_x-80, center_y+20, 0)
            
            # 更新显示
            self.epdiy.update()
            print("✅ 边界测试完成")
            
        except Exception as e:
            print(f"❌ 边界测试失败: {e}")

# ===== 全局函数接口 (便于直接调用) =====

# 全局测试实例
_test_instance = None

def _get_test_instance():
    """获取测试实例，自动初始化"""
    global _test_instance
    if _test_instance is None:
        _test_instance = ScreenRangeTest()
        if not _test_instance.init():
            return None
    return _test_instance

def test_basic():
    """基础屏幕范围测试"""
    test = _get_test_instance()
    if test:
        test.test_basic()
        
def test_grid(grid_size=100):
    """网格测试"""
    test = _get_test_instance()
    if test:
        test.test_grid(grid_size)
        
def test_text_layout(start_x=50, start_y=50, line_spacing=80, num_lines=6):
    """文本布局测试"""
    test = _get_test_instance()
    if test:
        test.test_text_layout(start_x, start_y, line_spacing, num_lines)
        
def test_custom(start_x=100, start_y=80, line_spacing=70, num_lines=5, grid=False):
    """自定义测试"""
    test = _get_test_instance()
    if test:
        test.test_custom(start_x, start_y, line_spacing, num_lines, grid)
        
def test_boundaries():
    """边界测试"""
    test = _get_test_instance()
    if test:
        test.test_boundaries()
        
def cleanup():
    """清理资源"""
    global _test_instance
    if _test_instance:
        _test_instance.cleanup()
        _test_instance = None
        
def help():
    """显示帮助信息"""
    print("=== Papers3 屏幕范围测试帮助 ===")
    print("")
    print("📋 基础测试函数:")
    print("  test_basic()                    - 基础屏幕范围测试 (边界+角落+中心)")
    print("  test_grid(size=100)             - 坐标网格测试")
    print("  test_text_layout(x,y,spacing,lines) - 文本布局测试")
    print("  test_custom(x,y,spacing,lines,grid) - 自定义参数测试")
    print("  test_boundaries()               - 边界位置测试")
    print("  cleanup()                       - 清理显示器资源")
    print("")
    print("🎭 演示函数:")
    print("  demo_quick()                    - 快速演示 (基础测试)")
    print("  demo_all()                      - 完整演示 (所有测试)")
    print("  demo_custom_examples()          - 自定义参数演示")
    print("  demo_interactive()              - 交互式演示菜单")
    print("  help()                          - 显示此帮助")
    print("")
    print("💡 使用示例:")
    print("  import screen_range_test as srt")
    print("  srt.demo_quick()                # 快速演示")
    print("  srt.demo_interactive()          # 交互式演示")
    print("  srt.test_basic()                # 单独运行基础测试")
    print("  srt.test_grid(50)               # 50px网格")
    print("  srt.test_text_layout(100, 60, 70, 8)  # 自定义文本布局")
    print("  srt.test_custom(50, 40, 60, 7, True)  # 带网格的自定义测试")
    print("  srt.cleanup()                   # 清理资源")
    print("")
    print("📐 Papers3 屏幕参数:")
    print("  分辨率: 960 x 540 像素")
    print("  坐标系: (0,0) 左上角, (959,539) 右下角") 
            print("  颜色: 0x00=黑色, 0x80=灰色, 0xF0=白色")
    print("")

def demo_all():
    """运行所有演示测试"""
    print("=== Papers3 屏幕范围测试演示 ===")
    print("将依次运行多个测试，每个测试间隔5秒")
    print("")
    
    tests = [
        ("基础测试", test_basic),
        ("网格测试", lambda: test_grid(80)),
        ("文本布局测试", lambda: test_text_layout(60, 70, 60, 7)),
        ("自定义测试", lambda: test_custom(80, 50, 80, 6, True)),
        ("边界测试", test_boundaries),
    ]
    
    for i, (name, test_func) in enumerate(tests):
        print(f"\n🔄 运行测试 {i+1}/{len(tests)}: {name}")
        try:
            test_func()
            if i < len(tests) - 1:  # 不是最后一个测试
                print(f"⏱️  等待5秒后继续下一个测试...")
                import time
                time.sleep(5)
        except KeyboardInterrupt:
            print(f"\n⏹️  用户中断了演示")
            break
        except Exception as e:
            print(f"❌ 测试失败: {e}")
            
    print("\n🎉 所有演示测试完成！")
    
def demo_quick():
    """快速演示 - 只运行基础测试"""
    print("=== 快速演示 - 基础测试 ===")
    test_basic()
    
def demo_custom_examples():
    """演示自定义参数的使用"""
    print("=== 自定义参数演示 ===")
    
    examples = [
        ("小间距文本", lambda: test_text_layout(50, 50, 40, 8)),
        ("大间距文本", lambda: test_text_layout(100, 80, 100, 4)),
        ("右侧文本", lambda: test_text_layout(500, 60, 70, 6)),
        ("带网格的自定义", lambda: test_custom(120, 40, 60, 7, True)),
    ]
    
    for i, (name, test_func) in enumerate(examples):
        print(f"\n🔄 演示 {i+1}/{len(examples)}: {name}")
        try:
            test_func()
            if i < len(examples) - 1:
                print("⏱️  等待3秒...")
                import time
                time.sleep(3)
        except KeyboardInterrupt:
            print("\n⏹️  演示被中断")
            break
        except Exception as e:
            print(f"❌ 演示失败: {e}")
            
    print("\n✅ 自定义参数演示完成！")

def demo_interactive():
    """交互式演示菜单"""
    print("Papers3 屏幕范围测试演示")
    print("选择演示模式:")
    print("1. 完整演示 (所有测试)")
    print("2. 快速演示 (基础测试)")
    print("3. 自定义参数演示")
    print("4. 帮助信息")
    
    try:
        choice = input("请选择 (1-4): ").strip()
        
        if choice == "1":
            demo_all()
        elif choice == "2":
            demo_quick()
        elif choice == "3":
            demo_custom_examples()
        elif choice == "4":
            help()
        else:
            print("无效选择，运行快速演示")
            demo_quick()
            
    except KeyboardInterrupt:
        print("\n👋 演示结束")
    except Exception as e:
        print(f"❌ 演示出错: {e}")
    finally:
        # 清理资源
        try:
            cleanup()
        except:
            pass

# 脚本直接运行时的测试
if __name__ == "__main__":
    print("Papers3 屏幕范围测试程序")
    help() 