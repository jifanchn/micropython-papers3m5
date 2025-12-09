"""
Papers3 硬件测试模块
提供完整的硬件功能测试接口

使用方法:
    import papers3
    test = papers3.test()
    test.help()     # 查看所有功能
    test.init()     # 初始化硬件
    test.basic()    # 运行基础测试
    test.touch_paint() # 触摸绘图测试
    test.complex_display() # 复杂显示测试
"""

import papers3
import time

class Test:
    """Papers3 硬件测试类"""
    
    def __init__(self):
        """初始化测试类"""
        self.buzzer = None
        self.battery = None
        self.epdiy = None
        self.gyro = None
        self.rtc = None
        self.led = None
        self.touch = None
        self.initialized = False
        
    def init(self):
        """初始化所有硬件模块"""
        print("=== Papers3 硬件测试初始化 ===")
        
        # 单独初始化每个模块，失败的跳过
        
        # 初始化蜂鸣器
        try:
            self.buzzer = papers3.Buzzer()
            self.buzzer.init()
            print("✅ 蜂鸣器初始化成功")
        except Exception as e:
            print(f"❌ 蜂鸣器初始化失败: {e}")
            
        # 初始化电池监控
        try:
            self.battery = papers3.Battery()
            self.battery.init()
            print("✅ 电池监控初始化成功")
        except Exception as e:
            print(f"❌ 电池监控初始化失败: {e}")
            
        # 初始化EPD显示器
        try:
            if self.epdiy is None:
                self.epdiy = papers3.EPDiy()
                self.epdiy.init()
            print("✅ EPD显示器初始化成功")
        except Exception as e:
            print(f"❌ EPD显示器初始化失败: {e}")
            
        # 初始化陀螺仪
        try:
            self.gyro = papers3.Gyro()
            self.gyro.init()
            print("✅ 陀螺仪初始化成功")
        except Exception as e:
            print(f"❌ 陀螺仪初始化失败: {e}")
            
        # 初始化RTC
        try:
            self.rtc = papers3.RTC()
            self.rtc.init()
            print("✅ RTC初始化成功")
        except Exception as e:
            print(f"❌ RTC初始化失败: {e}")
            
        # 初始化LED
        try:
            self.led = papers3.LED()
            self.led.init()
            print("✅ LED初始化成功")
        except Exception as e:
            print(f"❌ LED初始化失败: {e}")
            
        # 初始化触摸屏
        try:
            self.touch = papers3.Touch()
            self.touch.init()
            print("✅ 触摸屏初始化成功")
        except Exception as e:
            print(f"❌ 触摸屏初始化失败: {e}")
            
        self.initialized = True
        
        # 统计初始化结果
        success_count = sum([
            self.buzzer is not None,
            self.battery is not None,
            self.epdiy is not None,
            self.gyro is not None,
            self.rtc is not None,
            self.led is not None,
            self.touch is not None
        ])
        
        print(f"🎉 硬件初始化完成！成功: {success_count}/7 个模块")
        return success_count > 0
            
    def _check_init(self):
        """检查是否已初始化"""
        if not self.initialized:
            print("⚠️  请先调用 test.init() 初始化硬件")
            return False
        return True
        
    def _safe_draw_text(self, text, x, y, color):
        """安全的文本绘制，自动截断长文本"""
        try:
            # 限制文本长度，避免EPDiy处理长文本时出错
            if len(text) > 20:  # 限制最大20字符
                text = text[:17] + "..."  # 截断并添加省略号
            self.epdiy.draw_text(text, x, y, color)
            return True
        except Exception as e:
            print(f"⚠️ 绘制文本失败: {text[:10]}... -> {e}")
            return False
    
    def help(self):
        """显示帮助信息"""
        print("=== Papers3 硬件测试帮助 ===")
        print("")
        print("📋 可用测试函数:")
        print("  test.init()           - 初始化所有硬件模块 (必须先执行)")
        print("  test.help()           - 显示此帮助信息")
        print("")
        print("🔧 基础测试:")
        print("  test.basic()          - 运行所有基础硬件测试")
        print("  test.system_info()    - 显示系统信息")
        print("  test.buzzer_test()    - 蜂鸣器测试")
        print("  test.battery_test()   - 电池监控测试")
        print("  test.led_test()       - LED闪烁测试")
        print("  test.gyro_test()      - 陀螺仪和加速度计测试")
        print("  test.rtc_test()       - RTC实时时钟测试")
        print("  test.touch_test()     - 触摸屏基础测试")
        print("  test.display_test()   - 显示器基础测试")
        print("  test.temperature_test() - EPD温度设置测试")
        print("  test.simple_chinese() - 简单中文显示测试")
        print("")
        print("🎨 高级测试:")
        print("  test.touch_paint()    - 触摸绘图测试 (30秒倒计时画点)")
        print("  test.complex_display() - 复杂显示测试 (中英文混合绘图)")
        print("")
        print("🧹 资源管理:")
        print("  test.cleanup()        - 清理所有硬件资源")
        print("")
        print("💡 使用示例:")
        print("  >>> test = papers3.test()")
        print("  >>> test.init()")
        print("  >>> test.basic()      # 运行基础测试")
        print("  >>> test.touch_paint() # 触摸绘图测试")
        print("  >>> test.cleanup()")
        print("")
        
    def basic(self):
        """运行所有基础硬件测试"""
        if not self._check_init():
            return
            
        print("\n=== 开始基础硬件测试 ===")
        
        self.system_info()
        self.buzzer_test()
        self.battery_test()
        self.led_test()
        self.gyro_test()
        self.rtc_test()
        self.touch_test()
        self.display_test()
        self.temperature_test()
        self.chinese()
        
        print("🎉 基础测试完成！")
        
    def all(self):
        """运行所有测试 (兼容性保留)"""
        print("⚠️  建议使用 test.basic() 替代 test.all()")
        self.basic()
        
    def system_info(self):
        """显示系统信息"""
        print("\n--- 系统信息测试 ---")
        papers3.info()
        papers3.flash_info()
        papers3.ram_info()
        
    def buzzer_test(self):
        """蜂鸣器测试"""
        if not self._check_init():
            return
            
        print("\n--- 蜂鸣器测试 ---")
        try:
            print("播放测试音调...")
            self.buzzer.beep(1000, 200)  # 1kHz, 200ms
            time.sleep(0.3)
            self.buzzer.beep(1500, 200)  # 1.5kHz, 200ms
            time.sleep(0.3)
            self.buzzer.beep(2000, 200)  # 2kHz, 200ms
            print("✅ 蜂鸣器测试完成")
        except Exception as e:
            print(f"❌ 蜂鸣器测试失败: {e}")
            
    def battery_test(self):
        """电池测试"""
        if not self._check_init():
            return
            
        print("\n--- 电池监控测试 ---")
        try:
            voltage = self.battery.voltage()
            percentage = self.battery.percentage()
            print(f"电池电压: {voltage} mV")
            print(f"电池电量: {percentage}%")
            
            if voltage > 3700:
                print("🔋 电池电量充足")
            elif voltage > 3400:
                print("🔋 电池电量正常")
            else:
                print("🔋 电池电量偏低，建议充电")
                
            print("✅ 电池监控测试完成")
        except Exception as e:
            print(f"❌ 电池监控测试失败: {e}")
            
    def led_test(self):
        """LED测试"""
        if not self._check_init():
            return
            
        print("\n--- LED测试 ---")
        if self.led is None:
            print("❌ LED未初始化，跳过测试")
            return
            
        try:
            print("LED闪烁测试...")
            for i in range(3):
                self.led.on()
                time.sleep(0.2)
                self.led.off()
                time.sleep(0.2)
            print("✅ LED测试完成")
        except Exception as e:
            print(f"❌ LED测试失败: {e}")
            
    def gyro_test(self):
        """陀螺仪测试"""
        if not self._check_init():
            return
            
        print("\n--- 陀螺仪测试 ---")
        if self.gyro is None:
            print("❌ 陀螺仪未初始化，跳过测试")
            return
            
        try:
            accel = self.gyro.read_accel()
            gyro = self.gyro.read_gyro()
            print(f"加速度计: X={accel[0]:.2f}, Y={accel[1]:.2f}, Z={accel[2]:.2f}")
            print(f"陀螺仪: X={gyro[0]:.2f}, Y={gyro[1]:.2f}, Z={gyro[2]:.2f}")
            print("✅ 陀螺仪测试完成")
        except Exception as e:
            print(f"❌ 陀螺仪测试失败: {e}")
            
    def rtc_test(self):
        """RTC测试"""
        if not self._check_init():
            return
            
        print("\n--- RTC测试 ---")
        if self.rtc is None:
            print("❌ RTC未初始化，跳过测试")
            return
            
        try:
            datetime = self.rtc.datetime()
            print(f"当前时间: {datetime}")
            print("✅ RTC测试完成")
        except Exception as e:
            print(f"❌ RTC测试失败: {e}")
            
    def touch_test(self):
        """触摸屏测试"""
        if not self._check_init():
            return
            
        print("\n--- 触摸屏测试 ---")
        if self.touch is None:
            print("❌ 触摸屏未初始化，跳过测试")
            return
            
        try:
            print("检测触摸点，请触摸屏幕...")
            for i in range(10):  # 增加检测次数
                # 主动更新触摸数据
                self.touch.update()
                num_touches = self.touch.get_touches()
                
                if num_touches > 0:
                    print(f"检测到 {num_touches} 个触摸点:")
                    for j in range(num_touches):
                        point = self.touch.get_point(j)
                        print(f"  触摸点 {j}: X={point[0]}, Y={point[1]}, Size={point[2]}")
                else:
                    print(f"第 {i+1} 次检测: 未检测到触摸")
                time.sleep(0.5)
            print("✅ 触摸屏测试完成")
        except Exception as e:
            print(f"❌ 触摸屏测试失败: {e}")
            
    def display_test(self):
        """显示器基本测试"""
        if not self._check_init():
            return
            
        print("\n--- 显示器测试 ---")
        if self.epdiy is None:
            print("❌ EPD显示器未初始化，跳过测试")
            return
            
        try:
            # 清屏
            self.epdiy.clear()
            
            # 绘制测试图形
            self.epdiy.draw_rect(50, 50, 200, 100, 0)
            self.epdiy.draw_text("Papers3 Display Test", 60, 80, 0)
            self.epdiy.draw_line(50, 200, 250, 200, 0)
            self.epdiy.draw_circle(400, 150, 50, 0)
            
            # 更新显示
            self.epdiy.update()
            print("✅ 显示器测试完成")
        except Exception as e:
            print(f"❌ 显示器测试失败: {e}")
            
    def temperature_test(self):
        """EPD温度设置测试"""
        if not self._check_init():
            return
            
        print("\n--- EPD温度设置测试 ---")
        if self.epdiy is None:
            print("❌ EPD显示器未初始化，跳过测试")
            return
            
        try:
            # 读取默认温度
            temp = self.epdiy.get_temperature()
            print(f"当前EPD温度: {temp:.1f}°C")
            
            # 设置新温度
            new_temp = 26.0
            print(f"设置EPD温度为: {new_temp:.1f}°C")
            self.epdiy.set_temperature(new_temp)
            
            # 验证设置
            temp = self.epdiy.get_temperature()
            print(f"验证EPD温度: {temp:.1f}°C")
            
            if abs(temp - new_temp) < 0.1:
                print("✅ 温度设置成功")
            else:
                print("❌ 温度设置验证失败")
                
        except Exception as e:
            print(f"❌ 温度测试失败: {e}")
            
    def chinese(self):
        """中文字体测试"""
        if not self._check_init():
            return
            
        print("\n--- 中文字体测试 ---")
        if self.epdiy is None:
            print("❌ EPD显示器未初始化，跳过测试")
            return
            
        try:
            # 清屏
            self.epdiy.clear()
            
            # 显示中文测试文本，增加间距到80px避免重叠，避免标点符号
            test_texts = [
                "你好世界",
                "Papers3",
                "支持中文",
                "MicroPython",
                "显示测试"
            ]
            
            y_pos = 50
            for i, text in enumerate(test_texts):
                print(f"  绘制第{i+1}行: '{text}' at y={y_pos}")
                self.epdiy.draw_text(text, 50, y_pos, 0)
                y_pos += 80  # 增加到80px行间距，确保不重叠
                
            # 更新显示
            self.epdiy.update()
            print("✅ 中文字体测试完成")
            
            # 添加延迟，确保操作完成
            import time
            time.sleep(1)
            
        except Exception as e:
            print(f"❌ 中文字体测试失败: {e}")
            import sys
            sys.print_exception(e)  # 打印详细错误信息
            
    def touch_paint(self):
        """触摸绘图测试 - 30秒倒计时画点"""
        if not self._check_init():
            return
            
        print("\n--- 触摸绘图测试 ---")
        if self.epdiy is None or self.touch is None:
            print("❌ EPD显示器或触摸屏未初始化，跳过测试")
            return
            
        print("🎨 请在屏幕上触摸画点，持续30秒")
        print("⏰ 左上角会显示剩余时间")
        
        try:
            # 清屏
            self.epdiy.clear()
            self.epdiy.draw_text("Touch Paint Test", 10, 200, 0)
            self.epdiy.draw_text("Touch screen to draw dots", 10, 280, 8)  # 200+80=280
            self.epdiy.update()
            
            start_time = time.ticks_ms()
            duration = 30000  # 30秒
            last_update = 0
            touched_points = []
            
            while True:
                current_time = time.ticks_ms()
                elapsed = time.ticks_diff(current_time, start_time)
                
                if elapsed >= duration:
                    break
                
                remaining = (duration - elapsed) // 1000
                
                # 每秒更新倒计时显示
                if current_time - last_update > 1000:
                    # 清除左上角区域并重新绘制倒计时
                    self.epdiy.fill_rect(10, 0, 120, 50, 15)  # 白色覆盖，适中区域
                    self.epdiy.draw_text(f"Time: {remaining}s", 10, 30, 0)
                    
                    # 重新绘制所有触摸点
                    for point in touched_points:
                        self.epdiy.fill_circle(point[0], point[1], 3, 0)  # 3像素黑点
                    
                    self.epdiy.update()
                    last_update = current_time
                
                # 检测触摸
                self.touch.update()
                num_touches = self.touch.get_touches()
                if num_touches > 0:
                    for j in range(num_touches):
                        point = self.touch.get_point(j)
                        x, y = point[0], point[1]
                        # 避免在倒计时区域画点
                        if y > 90:
                            # 检查是否是新的触摸点
                            is_new = True
                            for existing in touched_points:
                                if abs(existing[0] - x) < 10 and abs(existing[1] - y) < 10:
                                    is_new = False
                                    break
                            
                            if is_new:
                                touched_points.append((x, y))
                                self.epdiy.fill_circle(x, y, 5, 0)
                                print(f"📍 触摸点: ({x}, {y})")
                
                time.sleep(0.05)  # 50ms 检测间隔
            
            # 测试结束
            self.epdiy.clear()
            time.sleep(0.1)  # 清屏后稍等
            self._safe_draw_text("Touch Paint Finished!", 10, 50, 0)
            self._safe_draw_text(f"Total: {len(touched_points)}", 10, 130, 8)  # 缩短文本
            self.epdiy.update()
            
            print(f"🎨 触摸绘图测试完成！共绘制 {len(touched_points)} 个点")
            
        except KeyboardInterrupt:
            print("\n🛑 触摸绘图测试被中断")
        except Exception as e:
            print(f"❌ 触摸绘图测试失败: {e}")
            
    def complex_display(self):
        """复杂显示测试"""
        if not self._check_init():
            return
            
        print("\n--- 复杂显示测试 ---")
        if self.epdiy is None:
            print("❌ EPD显示器未初始化，跳过测试")
            return
            
        try:
            # 清屏
            self.epdiy.clear()
            
            # 绘制标题框 (高度调整为80px)
            self.epdiy.fill_rect(10, 10, 940, 80, 0)  # 黑色背景
            self.epdiy.draw_text("Papers3 状态", 50, 60, 15)  # 白色文字，垂直居中，缩短文本
            
            # 绘制系统信息，使用80px行间距避免重叠
            info_y = 110
            
            # 电池信息 (缩短文本)
            voltage = self.battery.voltage() if self.battery else 3700
            self.epdiy.draw_text(f"电池 {voltage}mV", 50, info_y, 0)
            info_y += 80
            
            # 时间信息 (缩短文本)
            datetime = self.rtc.datetime() if self.rtc else (2024, 1, 1, 0, 12, 0, 0, 0)
            time_str = f"{datetime[1]:02d}/{datetime[2]:02d} {datetime[4]:02d}:{datetime[5]:02d}"
            self.epdiy.draw_text(f"时间 {time_str}", 50, info_y, 0)
            info_y += 80
            
            # 陀螺仪信息 (缩短文本)
            accel = self.gyro.read_accel() if self.gyro else (0.0, 0.0, 9.8)
            self.epdiy.draw_text(f"加速度 {accel[2]:.1f}", 50, info_y, 0)
            info_y += 80
            
            # 绘制信息框 (简化文本，确保不超出边界)
            self.epdiy.draw_rect(30, 300, 500, 120, 0)
            self.epdiy.draw_text("状态", 50, 330, 0)
            self.epdiy.draw_text("✓ 显示OK", 80, 380, 0)  # 330+50=380
            
            # 更新显示
            self.epdiy.update()
            print("✅ 复杂显示测试完成")
            
            # 添加延迟
            import time
            time.sleep(1)
            
        except Exception as e:
            print(f"❌ 复杂显示测试失败: {e}")
            
    def demo(self):
        """演示模式 - 实时显示系统信息"""
        if not self._check_init():
            return
            
        print("\n--- 演示模式 ---")
        print("按 Ctrl+C 退出演示模式")
        
        if self.epdiy is None:
            print("❌ EPD显示器未初始化，跳过测试")
            return
            
        try:
            import time
            while True:
                # 显示系统信息
                self.epdiy.clear()
                time.sleep(0.1)  # 清屏后稍等
                self.epdiy.draw_text("Papers3 演示", 50, 50, 0)  # 缩短标题
                
                # 显示电池信息，使用80px行间距避免重叠
                voltage = self.battery.voltage()
                self.epdiy.draw_text(f"电池 {voltage}mV", 50, 130, 0)  # 50+80=130
                
                # 显示时间 (缩短格式)
                datetime = self.rtc.datetime()
                time_str = f"{datetime[4]:02d}:{datetime[5]:02d}"
                self.epdiy.draw_text(f"时间 {time_str}", 50, 210, 0)  # 130+80=210
                
                # 显示陀螺仪数据 (只显示Z轴)
                accel = self.gyro.read_accel()
                self.epdiy.draw_text(f"Z轴 {accel[2]:.1f}", 50, 290, 0)  # 210+80=290
                
                # 显示简化状态
                self.epdiy.draw_text("运行中...", 50, 370, 0)  # 290+80=370
                
                # 更新显示
                self.epdiy.update()
                
                # 等待3秒 (缩短刷新间隔)
                time.sleep(3)
                
        except KeyboardInterrupt:
            print("\n演示模式已退出")
        except Exception as e:
            print(f"❌ 演示模式失败: {e}")
            
    def cleanup(self):
        """清理资源"""
        print("\n--- 清理硬件资源 ---")
        try:
            if self.buzzer:
                self.buzzer.deinit()
            if self.battery:
                self.battery.deinit()
            if self.epdiy:
                self.epdiy.deinit()
            if self.gyro:
                self.gyro.deinit()
            if self.rtc:
                self.rtc.deinit()
            if self.led:
                self.led.deinit()
            if self.touch:
                self.touch.deinit()
            print("✅ 资源清理完成")
        except Exception as e:
            print(f"❌ 资源清理失败: {e}")

    def simple_chinese(self):
        """简单中文测试"""
        if not self._check_init():
            return
            
        print("\n--- 简单中文测试 ---")
        if self.epdiy is None:
            print("❌ EPD显示器未初始化，跳过测试")
            return
            
        try:
            # 清屏并绘制简单中文
            self.epdiy.clear()
            time.sleep(1)
            
            # 简单的中文测试，使用80px间距避免重叠
            print("  绘制第1行: '你好世界' at y=100")
            self.epdiy.draw_text("你好世界", 50, 100, 0)
            print("  绘制第2行: 'Papers3 中文' at y=180")
            self.epdiy.draw_text("Papers3 中文", 50, 180, 0)  # 100+80=180
            
            # 更新显示
            print("  更新显示...")
            self.epdiy.update()
            print("✅ 简单中文测试完成")
            
            # 添加延迟
            import time
            time.sleep(0.5)
            
        except Exception as e:
            print(f"❌ 简单中文测试失败: {e}")
            import sys
            sys.print_exception(e)  # 打印详细错误信息

# 创建测试实例的工厂函数
def create_test():
    """创建测试实例"""
    return Test() 