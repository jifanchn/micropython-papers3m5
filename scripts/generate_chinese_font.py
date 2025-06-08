#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Papers3 中文字体生成脚本
======================

使用common_7000.txt文件中的汉字和系统宋体字体生成适用于Papers3的中文字体。
输出文件将保存到papers3目录中。

使用方法:
    python generate_chinese_font.py [--size SIZE] [--output OUTPUT]

示例:
    python generate_chinese_font.py --size 32
    python generate_chinese_font.py --size 36 --output ../papers3/fonts/

作者: Papers3 开发团队
"""

import argparse
import os
import sys
import subprocess
from pathlib import Path


def find_songti_font():
    """查找系统中的宋体字体文件"""
    possible_paths = [
        "/System/Library/Fonts/Supplemental/Songti.ttc",
        "/System/Library/Fonts/Songti.ttc", 
        "/Library/Fonts/Songti.ttc",
        "~/Library/Fonts/Songti.ttc",
        "/System/Library/Fonts/STSong.ttf",
        "/Library/Fonts/STSong.ttf",
        "~/Library/Fonts/STSong.ttf"
    ]
    
    for path in possible_paths:
        expanded_path = os.path.expanduser(path)
        if os.path.exists(expanded_path):
            return expanded_path
    
    return None


def load_common_chars():
    """从common_7000.txt文件加载常用汉字"""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    common_file = os.path.join(script_dir, "common_7000.txt")
    
    if not os.path.exists(common_file):
        print(f"错误: 找不到 {common_file}")
        return None
    
    try:
        with open(common_file, 'r', encoding='utf-8') as f:
            content = f.read()
        
        # 清理内容，移除换行符和空格
        chars = content.replace('\n', '').replace(' ', '').replace('\r', '')
        
        # 去重并排序
        unique_chars = ''.join(sorted(set(chars)))
        
        print(f"从 {common_file} 加载了 {len(unique_chars)} 个字符")
        return unique_chars
        
    except Exception as e:
        print(f"读取 {common_file} 时出错: {e}")
        return None


def check_dependencies():
    """检查必要的依赖项"""
    try:
        import freetype
        return True
    except ImportError:
        print("错误: 缺少freetype-py库")
        print("请安装: pip install freetype-py")
        return False


def generate_font(font_path, font_name, size, chars, output_file, compress=True):
    """使用epdiy的fontconvert.py生成字体"""
    
    # 查找fontconvert.py脚本
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir)
    fontconvert_script = os.path.join(project_root, "epdiy", "scripts", "fontconvert.py")
    
    if not os.path.exists(fontconvert_script):
        print(f"错误: 找不到fontconvert.py脚本: {fontconvert_script}")
        return False
    
    # 构建命令
    cmd = [
        sys.executable,
        fontconvert_script,
        font_name,
        str(size),
        font_path,
        "--string", chars
    ]
    
    if compress:
        cmd.append("--compress")
    
    print(f"执行命令: {' '.join(cmd[:4])} ... --string [字符集] {'--compress' if compress else ''}")
    
    try:
        # 执行字体转换
        result = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8')
        
        if result.returncode == 0:
            # 保存输出到文件
            with open(output_file, 'w', encoding='utf-8') as f:
                f.write(result.stdout)
            return True
        else:
            print(f"字体生成失败:")
            print(result.stderr)
            return False
            
    except Exception as e:
        print(f"执行字体转换时出错: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(
        description="生成Papers3中文字体",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例用法:
  生成32px中文字体:
    %(prog)s --size 32

  生成36px中文字体并指定输出目录:
    %(prog)s --size 36 --output ../papers3/fonts/

  禁用压缩:
    %(prog)s --size 32 --no-compress
        """
    )

    parser.add_argument(
        '--size', '-s',
        type=int,
        default=32,
        help='字体大小 (默认: 32)'
    )

    parser.add_argument(
        '--output', '-o',
        default='../papers3',
        help='输出目录 (默认: ../papers3)'
    )

    parser.add_argument(
        '--compress',
        action='store_true',
        default=True,
        help='启用字体压缩 (默认启用)'
    )

    parser.add_argument(
        '--no-compress',
        action='store_true',
        help='禁用字体压缩'
    )

    parser.add_argument(
        '--font-path',
        help='指定字体文件路径 (默认自动查找宋体)'
    )

    parser.add_argument(
        '--verbose', '-v',
        action='store_true',
        help='显示详细信息'
    )

    args = parser.parse_args()

    print("=== Papers3 中文字体生成器 ===")
    
    # 检查依赖
    if not check_dependencies():
        sys.exit(1)
    
    # 查找字体文件
    if args.font_path:
        font_path = args.font_path
        if not os.path.exists(font_path):
            print(f"错误: 指定的字体文件不存在: {font_path}")
            sys.exit(1)
    else:
        font_path = find_songti_font()
        if not font_path:
            print("错误: 未找到宋体字体文件")
            print("请使用 --font-path 参数指定字体文件路径")
            sys.exit(1)
    
    print(f"使用字体文件: {font_path}")
    
    # 加载常用汉字
    chars = load_common_chars()
    if not chars:
        sys.exit(1)
    
    # 添加ASCII字符
    ascii_chars = ''.join(chr(i) for i in range(32, 127))
    all_chars = ascii_chars + chars
    
    print(f"总字符数: {len(set(all_chars))}")
    
    # 处理压缩选项
    compress = args.compress and not args.no_compress
    
    # 创建输出目录
    output_dir = Path(args.output)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    # 生成字体
    font_name = f"Chinese{args.size}"
    output_file = output_dir / f"chinese_{args.size}.h"
    
    print(f"正在生成 {args.size}px 中文字体...")
    print(f"输出文件: {output_file}")
    print(f"压缩: {'是' if compress else '否'}")
    print()
    
    success = generate_font(
        font_path, font_name, args.size, all_chars, 
        str(output_file), compress
    )
    
    if success:
        file_size = os.path.getsize(output_file) / 1024
        print(f"✓ 字体生成成功!")
        print(f"  文件: {output_file}")
        print(f"  大小: {file_size:.1f} KB")
        print()
        print("使用方法:")
        print(f"1. 在C代码中包含: #include \"chinese_{args.size}.h\"")
        print(f"2. 注册字体: papers3_font_register(\"Chinese{args.size}\", &Chinese{args.size}, FONT_TYPE_CHINESE, {args.size}, {str(compress).lower()});")
        print(f"3. 在Python中使用: font = papers3.fonts.get(\"Chinese{args.size}\")")
    else:
        print("✗ 字体生成失败")
        sys.exit(1)


if __name__ == "__main__":
    main() 