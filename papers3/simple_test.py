# simple_test.py
# Verify Papers3 Hardware Refactoring

import papers3
import time
import machine

def test_touch():
    print("Testing Touch...")
    try:
        t = papers3.Touch()
        t.init()
        print("Touch initialized. Touch the screen...")
        for i in range(50):
            if t.available():
                t.update()
                if t.get_touches() > 0:
                    pt = t.get_point(0)
                    print(f"Touch: {pt}")
            time.sleep(0.1)
        t.deinit()
        print("Touch test passed.")
    except Exception as e:
        print(f"Touch test failed: {e}")

def test_gyro():
    print("Testing Gyro...")
    try:
        g = papers3.Gyro()
        g.init()
        acc = g.read_accel()
        gyro = g.read_gyro()
        print(f"Accel: {acc}")
        print(f"Gyro: {gyro}")
        g.deinit()
        print("Gyro test passed.")
    except Exception as e:
        print(f"Gyro test failed: {e}")

def test_rtc():
    print("Testing RTC...")
    try:
        r = papers3.RTC()
        r.init()
        dt = r.datetime()
        print(f"RTC DateTime: {dt}")
        r.deinit()
        print("RTC test passed.")
    except Exception as e:
        print(f"RTC test failed: {e}")

def test_sdcard():
    print("Testing SDCard...")
    try:
        sd = papers3.SDCard()
        sd.init()
        print("SDCard initialized.")
        try:
            sd.mount()
            print("SDCard mounted.")
            info = sd.info()
            print(f"SDCard Info: {info}")
            sd.unmount()
            print("SDCard unmounted.")
        except Exception as e:
            print(f"SDCard mount/info failed: {e}")
        
        sd.deinit()
        print("SDCard test finished.")
    except Exception as e:
        print(f"SDCard init failed: {e}")

def test_epd():
    print("Testing EPDiy...")
    try:
        epd = papers3.EPDiy()
        epd.init()
        print(f"EPD Size: {epd.width()}x{epd.height()}")
        
        # Test Temperature
        print(f"Original Temp: {epd.get_temperature()}")
        epd.set_temperature(28.5)
        print(f"New Temp: {epd.get_temperature()}")
        
        epd.clear()
        epd.draw_rect(10, 10, 100, 100, 0)
        epd.draw_text("Papers3 Refactored", 20, 50, 0)
        epd.update()
        print("EPD updated.")
        
        epd.deinit()
        print("EPD test passed.")
    except Exception as e:
        print(f"EPD test failed: {e}")

def test_reinit():
    print("Testing Re-initialization Robustness...")
    try:
        # 1. Init Deinit Cycle
        print("Cycle 1: Normal")
        epd = papers3.EPDiy()
        epd.init()
        epd.deinit()
        
        # 2. Init without Deinit (Simulating loss)
        print("Cycle 2: No Deinit -> New Init")
        epd = papers3.EPDiy()
        epd.init()
        del epd 
        # Force GC if possible, or just init new one
        
        print("Cycle 3: Init on top of Init")
        epd2 = papers3.EPDiy()
        epd2.init() # Should trigger safe re-init
        epd2.deinit()
        
        print("Re-init test passed.")
    except Exception as e:
        print(f"Re-init test failed: {e}")

def run_all():
    print("=== Starting Papers3 Hardware Test ===")
    test_reinit() # Run this first to check stability
    test_rtc()
    test_gyro()
    test_touch() 
    test_sdcard()
    test_epd()
    print("=== Test Complete ===")

if __name__ == "__main__":
    run_all()
