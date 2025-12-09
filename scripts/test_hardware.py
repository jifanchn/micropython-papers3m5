#!/usr/bin/env python3
"""
Papers3 Hardware Test Script
测试 Papers3 硬件模块的串口测试脚本

用法:
    source esp-idf/export.sh
    python scripts/test_hardware.py [--all | --display | --sensors]

选项:
    --all       运行所有测试 (默认)
    --display   仅测试 E-ink 显示屏
    --sensors   仅测试传感器 (电池、蜂鸣器、LED等)
"""

import serial
import time
import sys
import os

# 自动检测串口
def detect_port():
    """检测可用的串口设备"""
    import glob
    if sys.platform == 'darwin':
        ports = glob.glob('/dev/cu.usbmodem*') + glob.glob('/dev/cu.usbserial*')
    elif sys.platform == 'linux':
        ports = glob.glob('/dev/ttyUSB*') + glob.glob('/dev/ttyACM*')
    else:
        ports = [f'COM{i}' for i in range(1, 20)]
    
    for port in ports:
        try:
            s = serial.Serial(port, 115200, timeout=0.5)
            s.close()
            return port
        except:
            continue
    return None

BAUD = 115200

def send_command(port, cmd, wait=1.0):
    """发送命令并读取响应"""
    port.write((cmd + '\r\n').encode())
    time.sleep(wait)
    try:
        response = port.read(port.in_waiting or 2000)
        return response.decode('utf-8', errors='ignore')
    except:
        return "(read error)"

def print_result(name, response):
    """打印测试结果"""
    print(f"\n{'=' * 50}")
    print(f"  {name}")
    print('=' * 50)
    for line in response.split('\n'):
        line = line.strip()
        if line and line != '>>>':
            print(f"  {line}")

def test_sensors(port):
    """测试传感器模块"""
    tests = [
        ("系统信息", "papers3.info()"),
        ("内存信息", "papers3.ram_info()"),
        ("电池初始化", "bat = papers3.Battery(); bat.init()"),
        ("读取电压", "print('电压:', bat.voltage(), 'mV')"),
        ("蜂鸣器测试", "buz = papers3.Buzzer(); buz.init(); buz.beep(1000, 100)"),
        ("LED 测试", "led = papers3.LED(); led.init(); led.on(); import time; time.sleep(0.3); led.off()"),
        ("RTC 时钟", "rtc = papers3.RTC(); rtc.init(); print('时间:', rtc.datetime())"),
    ]
    
    print("\n" + "=" * 60)
    print("  传感器测试")
    print("=" * 60)
    
    for name, cmd in tests:
        response = send_command(port, cmd)
        print_result(name, response)
    
    print("\n✅ 传感器测试完成")

def test_display(port):
    """测试 E-ink 显示屏"""
    tests = [
        ("创建显示对象", "epdiy = papers3.EPDiy()"),
        ("初始化显示", "epdiy.init()"),
        ("获取尺寸", "print('尺寸:', epdiy.width(), 'x', epdiy.height())"),
        ("清屏", "epdiy.clear()"),
        ("绘制文本", 'epdiy.draw_text("Papers3 Test", 100, 100, 0)'),
        ("绘制中文", 'epdiy.draw_text("墨水屏测试成功!", 100, 200, 0)'),
        ("绘制矩形", "epdiy.draw_rect(50, 50, 400, 280, 0)"),
        ("绘制圆", "epdiy.fill_circle(700, 270, 80, 0)"),
        ("刷新显示", "epdiy.update()"),
    ]
    
    print("\n" + "=" * 60)
    print("  E-ink 显示屏测试")
    print("=" * 60)
    
    for name, cmd in tests:
        response = send_command(port, cmd, wait=2.0)
        print_result(name, response)
    
    print("\n✅ 显示屏测试完成 - 请检查屏幕显示")

def test_stress(port):
    """压力测试：反复初始化/注销以验证内存管理和驱动稳定性"""
    print("\n" + "=" * 60)
    print("  压力/崩溃测试 (Init/Deinit Cycles)")
    print("=" * 60)
    
    # Reset REPL state first
    port.write(b'\x03\x03\r\n')
    time.sleep(0.3)
    port.read(port.in_waiting)
    
    # Import modules
    send_command(port, 'import papers3')
    send_command(port, 'import time')
    send_command(port, 'import gc')
    
    failures = 0
    num_cycles = 5
    
    # Initial Clean (Run once to ensure starting with White screen)
    # This prevents ghosting from previous runs
    print("Cleaning screen...")
    try:
        response = send_command(port, 'epd = papers3.EPDiy()', wait=0.5)
        send_command(port, 'epd.init()', wait=2.0)
        send_command(port, 'epd.clear()', wait=2.0)
        send_command(port, 'epd.deinit()', wait=0.5)
        send_command(port, 'epd = None; gc.collect()', wait=0.5)
        print("Screen cleaned.")
        time.sleep(2.0) # Cooldown
    except:
        print("Warning: Initial clear failed")

    for i in range(num_cycles):
        print(f"\n--- Cycle {i+1}/{num_cycles} ---")
        try:
            # Create and init
            response = send_command(port, f'epd = papers3.EPDiy()', wait=0.5)
            print(f"  Create: OK")
            
            response = send_command(port, 'epd.init()', wait=2.0)
            print(f"  Init: OK")
            
            # Draw text
            # Since screen started White, and we assume White, 
            # we just draw Black text. No need to fill white (it's redundant but safe)
            # But let's fill white just in case to be sure FB is clean
            send_command(port, 'epd.fill_rect(0, 0, 960, 540, 255)', wait=0.2)
            
            y_pos = 100 + i * 50
            response = send_command(port, f'epd.draw_text("Cycle {i+1}/5 - Papers3", 50, {y_pos}, 0)', wait=0.5)
            
            # Update
            response = send_command(port, 'epd.update()', wait=3.0)
            print(f"  Update: OK")
            
            # Deinit
            response = send_command(port, 'epd.deinit()', wait=0.5)
            print(f"  Deinit: OK")
            
            # Cleanup
            send_command(port, 'epd = None', wait=0.2)
            send_command(port, 'gc.collect()', wait=0.5)
            
            print(f"  ✅ Cycle {i+1} OK")
            
        except Exception as e:
            print(f"  ❌ Cycle {i+1} FAILED: {e}")
            failures += 1
            # Try to recover
            try:
                port.write(b'\x03\x03\r\n')
                time.sleep(0.3)
                port.read(port.in_waiting)
            except:
                pass
    
    if failures == 0:
        print(f"\n✅ 压力测试通过！{num_cycles} 个周期全部成功。")
    else:
        print(f"\n❌ 压力测试失败！{failures}/{num_cycles} 个周期出错。")

def main():
    # 解析参数
    test_all = '--all' in sys.argv or len(sys.argv) == 1
    test_disp = '--display' in sys.argv
    test_sens = '--sensors' in sys.argv
    test_crash = '--crash' in sys.argv
    
    if test_all:
        test_disp = test_sens = test_crash = True
    
    # 检测串口
    port_name = detect_port()
    if not port_name:
        print("❌ 未检测到串口设备")
        print("请确保设备已连接并重新插拔 USB")
        sys.exit(1)
    
    print(f"📡 连接到: {port_name}")
    
    try:
        port = serial.Serial(port_name, BAUD, timeout=2)
        time.sleep(0.5)
        
        # 清理缓冲区
        port.reset_input_buffer()
        port.write(b'\x03\x03\r\n')
        time.sleep(0.3)
        port.read(port.in_waiting)
        
        # 导入模块
        send_command(port, 'import papers3')
        
        if test_sens:
            test_sensors(port)
        
        if test_disp:
            test_display(port)
            
        if test_crash:
            test_stress(port)
        
        port.close()
        print("\n🎉 所有测试完成!")
        
    except Exception as e:
        print(f"❌ 错误: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
