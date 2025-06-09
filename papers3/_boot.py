import gc
import vfs
from flashbdev import bdev

print("Papers3: 启动初始化...")

try:
    if bdev:
        vfs.mount(bdev, "/")
except OSError:
    import inisetup
    inisetup.setup()

# 检查并创建/执行用户引导程序
try:
    # 检查boot.py是否存在
    try:
        with open("boot.py", "r") as f:
            pass  # 文件存在
    except OSError:
        # boot.py不存在，创建默认的
        with open("boot.py", "w") as f:
            f.write("""\
# Papers3 用户引导程序
# 这个文件在每次启动时执行（包括深度睡眠唤醒）

print("Papers3: 用户引导程序启动")

try:
    import papers3
    
    # 显示系统信息
    papers3.info()
    papers3.flash_info()
    papers3.ram_info()
    
    print("Papers3: 系统就绪！")
    print("Papers3: 演示程序位于 /demo 目录")
    print("Papers3: 输入 help() 查看更多信息")
    
except ImportError as e:
    print(f"Papers3: 模块导入失败: {e}")
except Exception as e:
    print(f"Papers3: 引导程序错误: {e}")

# 可以在这里添加你的自定义初始化代码
# 例如: WiFi连接, 传感器初始化等

# 取消注释以启用WebREPL
# import webrepl
# webrepl.start()
""")
        print("Papers3: 创建默认用户引导程序")
    
    # 执行boot.py
    exec(open("boot.py").read())
    
except Exception as e:
    print(f"Papers3: 引导程序处理失败: {e}")

gc.collect()
print("Papers3: 启动初始化完成") 