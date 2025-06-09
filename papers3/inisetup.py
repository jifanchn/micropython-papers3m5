import vfs
from flashbdev import bdev


def check_bootsec():
    buf = bytearray(bdev.ioctl(5, 0))  # 5 is SEC_SIZE
    bdev.readblocks(0, buf)
    empty = True
    for b in buf:
        if b != 0xFF:
            empty = False
            break
    if empty:
        return True
    fs_corrupted()


def fs_corrupted():
    import time
    import micropython

    # Allow this loop to be stopped via Ctrl-C.
    micropython.kbd_intr(3)

    while 1:
        print(
            """\
Papers3: 文件系统损坏。如果有重要数据，建议备份Flash快照尝试恢复。
否则，请执行完整的固件重刷（清空Flash + 烧写固件）。
"""
        )
        time.sleep(3)


def setup():
    check_bootsec()
    print("Papers3: 正在执行首次设置")
    if bdev.info()[4] == "vfs":
        vfs.VfsLfs2.mkfs(bdev)
        fs = vfs.VfsLfs2(bdev)
        print("Papers3: 使用 LittleFS v2 文件系统")
    elif bdev.info()[4] == "ffat":
        vfs.VfsFat.mkfs(bdev)
        fs = vfs.VfsFat(bdev)
        print("Papers3: 使用 FAT 文件系统")
    vfs.mount(fs, "/")
    
    # 创建Papers3专用的用户引导程序
    with open("boot.py", "w") as f:
        f.write(
            """\
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
"""
        )
    
    print("Papers3: 用户引导程序创建完成")
    return fs 