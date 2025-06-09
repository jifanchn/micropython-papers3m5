from esp32 import Partition

# Papers3 使用标准的 "vfs" 分区标签
bdev = Partition.find(Partition.TYPE_DATA, label="vfs")
if not bdev:
    # 备用方案：查找 "ffat" 分区（兼容TinyUF2）
    bdev = Partition.find(Partition.TYPE_DATA, label="ffat", block_size=512)
bdev = bdev[0] if bdev else None 