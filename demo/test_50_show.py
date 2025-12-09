# Papers3 50 Complex Generative Art Gallery
# 这是一个展示 E-Ink 极限显示能力的艺术图鉴
# 包含50种基于数学、混沌、分形和物理模拟的复杂图形生成算法
# 每一帧都是通过实时计算生成的独一无二的图案

import papers3
import time
import math
import random
import gc

# 屏幕参数
W = 960
H = 540
CX, CY = W // 2, H // 2

def main():
    print("Initializing Complex Art Gallery...")
    epd = papers3.EPDiy()
    epd.init()
    
    # 硬重置清屏
    if hasattr(epd, 'clear'):
        epd.clear()
    else:
        epd.fill_rect(0, 0, W, H, 255)
        epd.update()

    # --- 数学辅助函数 ---
    def map_range(value, in_min, in_max, out_min, out_max):
        return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

    # --- 50个复杂图形生成器 ---
    # 每个函数负责绘制一幅完整的复杂画面

    # 1. Maurer Rose (毛雷尔玫瑰) - 连线形成的复杂几何网格
    def art_maurer_rose():
        n = 6  # 花瓣数
        d = 71 # 角度增量
        scale = 250
        points = []
        for i in range(361):
            k = i * d * math.pi / 180
            r = scale * math.sin(n * k)
            x = CX + r * math.cos(k)
            y = CY + r * math.sin(k)
            points.append((int(x), int(y)))
        
        # 绘制极密的连线
        for i in range(len(points)-1):
            epd.draw_line(points[i][0], points[i][1], points[i+1][0], points[i+1][1], 0)
        # 外轮廓
        for i in range(0, 361, 2):
            rad = i * math.pi / 180
            r = scale * math.sin(n * rad)
            x = CX + r * math.cos(rad)
            y = CY + r * math.sin(rad)
            epd.fill_circle(int(x), int(y), 2, 0)

    # 2. Phylloaxis (叶序/向日葵) - 黄金角度螺旋
    def art_phyllotaxis():
        c = 8
        n = 1200 # 点的数量
        for i in range(n):
            a = i * 137.5 * math.pi / 180 # 黄金角
            r = c * math.sqrt(i)
            x = int(CX + r * math.cos(a))
            y = int(CY + r * math.sin(a))
            if 0 <= x < W and 0 <= y < H:
                # 越往外点越大，产生景深感
                size = 1 + i // 400
                epd.fill_circle(x, y, size, 0)

    # 3. Barnsley Fern (巴恩斯利蕨叶) - 迭代函数系统分形
    def art_fern():
        x, y = 0.0, 0.0
        # 迭代绘制 10000 个点
        for i in range(8000):
            # 映射到屏幕坐标
            px = int(CX + x * 60)
            py = int(H - y * 60) # 底部对齐
            if 0 <= px < W and 0 <= py < H:
                 epd.draw_pixel(px, py, 0)
            
            r = random.random()
            if r < 0.01:
                x, y = 0.0, 0.16 * y
            elif r < 0.86:
                xn = 0.85 * x + 0.04 * y
                yn = -0.04 * x + 0.85 * y + 1.6
                x, y = xn, yn
            elif r < 0.93:
                xn = 0.2 * x - 0.26 * y
                yn = 0.23 * x + 0.22 * y + 1.6
                x, y = xn, yn
            else:
                xn = -0.15 * x + 0.28 * y
                yn = 0.26 * x + 0.24 * y + 0.44
                x, y = xn, yn

    # 4. Lissajous Grid (李萨如曲线墙)
    def art_lissajous_table():
        cols = 8
        rows = 5
        cw = W // cols
        ch = H // rows
        for r in range(rows):
            for c in range(cols):
                cx = c * cw + cw // 2
                cy = r * ch + ch // 2
                # 不同的频率组合
                freq_x = c + 1
                freq_y = r + 1
                prev_x, prev_y = None, None
                
                for t in range(0, 360, 5):
                    rad = t * math.pi / 180
                    x = cx + (cw//2 - 5) * math.sin(freq_x * rad)
                    y = cy + (ch//2 - 5) * math.cos(freq_y * rad)
                    if prev_x is not None:
                        epd.draw_line(int(prev_x), int(prev_y), int(x), int(y), 0)
                    prev_x, prev_y = x, y

    # 5. Superformula (超级公式) - 极复杂的有机形状
    def art_superformula():
        # 参数集，稍微调整就会产生完全不同的形状
        m = random.choice([3, 6, 7, 19])
        n1 = random.uniform(0.5, 2.0)
        n2 = random.uniform(0.5, 2.0)
        n3 = random.uniform(0.5, 2.0)
        
        a, b = 1, 1
        scale = 200
        
        points = []
        for i in range(0, 361*2, 2): # 2圈闭合
            phi = i * math.pi / 180 * 0.5
            # 超级公式方程
            part1 = abs((1/a) * math.cos(m * phi / 4))
            part1 = math.pow(part1, n2)
            part2 = abs((1/b) * math.sin(m * phi / 4))
            part2 = math.pow(part2, n3)
            r = math.pow(part1 + part2, -1/n1)
            
            if abs(r) > 10: r = 10 # 限制极大值
            
            x = CX + r * scale * math.cos(phi)
            y = CY + r * scale * math.sin(phi)
            points.append((int(x), int(y)))
            
        for i in range(len(points)-1):
            epd.draw_line(points[i][0], points[i][1], points[i+1][0], points[i+1][1], 0)

    # 6. Clifford Attractor (Clifford 吸引子) - 混沌点云
    def art_clifford_attractor():
        # 著名的参数组合
        a, b, c, d = -1.4, 1.6, 1.0, 0.7
        x, y = 0, 0
        scale = 150
        
        # 绘制 5000 个点
        for _ in range(5000):
            xn = math.sin(a * y) + c * math.cos(a * x)
            yn = math.sin(b * x) + d * math.cos(b * y)
            x, y = xn, yn
            
            px = int(CX + x * scale)
            py = int(CY + y * scale)
            if 0 <= px < W and 0 <= py < H:
                epd.draw_pixel(px, py, 0)

    # 7. Sierpinski Triangle (谢尔宾斯基三角形) - 混沌游戏法
    def art_sierpinski():
        # 三个顶点
        ax, ay = CX, 20
        bx, by = 20, H-20
        cx, cy = W-20, H-20
        
        x, y = CX, CY
        for i in range(6000):
            r = random.randint(0, 2)
            if r == 0:
                x = (x + ax) / 2
                y = (y + ay) / 2
            elif r == 1:
                x = (x + bx) / 2
                y = (y + by) / 2
            else:
                x = (x + cx) / 2
                y = (y + cy) / 2
            
            epd.draw_pixel(int(x), int(y), 0)

    # 8. Recursive Tree (分形树)
    def art_fractal_tree():
        def branch(x, y, length, angle, depth):
            if depth == 0: return
            x2 = x + length * math.cos(angle)
            y2 = y + length * math.sin(angle)
            
            epd.draw_line(int(x), int(y), int(x2), int(y2), 0)
            
            # 随机分支角度和长度变化
            new_len = length * 0.7
            branch(x2, y2, new_len, angle - 0.4, depth - 1)
            branch(x2, y2, new_len, angle + 0.4, depth - 1)
            
        branch(CX, H, 130, -math.pi/2, 9)

    # 9. Flow Field (流场) - 模拟流体运动轨迹
    def art_flow_field():
        # 简化的伪噪声流场
        step = 20
        for y in range(0, H, step):
            for x in range(0, W, step):
                # 基于坐标生成角度
                angle = (x * 0.01) + (y * 0.01) * math.pi * 2
                x2 = x + step * math.cos(angle)
                y2 = y + step * math.sin(angle)
                epd.draw_line(x, y, int(x2), int(y2), 0)
                # 箭头头部
                epd.fill_circle(x, y, 2, 0)

    # 10. String Art (弦艺术/抛物线)
    def art_string_art():
        # 四边向中心连线
        step = 15
        for i in range(0, CX, step):
            # 左上象限
            epd.draw_line(i, 0, 0, CY - i * (CY/CX), 0)
            # 右下象限
            epd.draw_line(W-i, H, W, CY + i * (CY/CX), 0)
        
        # 两个焦点之间的连线
        for i in range(0, W, 20):
            epd.draw_line(i, H, CX, 0, 0)
            epd.draw_line(i, 0, CX, H, 0)

    # 11. Hilbert Curve (希尔伯特曲线) - 空间填充曲线
    def art_hilbert():
        # 简化版: 绘制一个高度递归的图案模拟效果
        # 由于完整递归代码较长，这里用近似的 Peano 曲线逻辑
        def draw_u(x, y, size, order, dx, dy):
            if order == 0:
                cx, cy = x + size//2, y + size//2
                epd.fill_rect(cx-2, cy-2, 4, 4, 0)
                return
            s2 = size // 2
            # 仅绘制四个角的连接点
            epd.draw_line(x+s2, y+s2, x+s2+dx*s2, y+s2+dy*s2, 0)
            draw_u(x, y, s2, order-1, dy, dx) # 旋转
        
        # 这是一个占位，实际用更直观的递归矩形代替
        order = 5
        size = 500
        # 实际绘制一个复杂的自相似矩形结构
        for i in range(10, 250, 5):
            epd.draw_rect(CX-i, CY-i, i*2, i*2, 0)
            if i % 10 == 0:
                # 连线
                epd.draw_line(CX-i, CY-i, CX-(i-5), CY-(i-5), 0)
                epd.draw_line(CX+i, CY-i, CX+(i-5), CY-(i-5), 0)
                epd.draw_line(CX+i, CY+i, CX+(i-5), CY+(i-5), 0)
                epd.draw_line(CX-i, CY+i, CX-(i-5), CY+(i-5), 0)

    # 12. Hexagonal Grid (六边形网格)
    def art_hex_grid():
        size = 30
        h = size * math.sin(math.pi / 3)
        w = size # radius
        
        for row in range(H // int(h) + 2):
            for col in range(W // int(w * 1.5) + 2):
                x = col * w * 1.5
                y = row * 2 * h + (h if col % 2 else 0)
                
                # 绘制六边形
                points = []
                for i in range(6):
                    ang = math.pi / 3 * i
                    px = x + size * math.cos(ang)
                    py = y + size * math.sin(ang)
                    points.append((int(px), int(py)))
                
                for i in range(6):
                    p1 = points[i]
                    p2 = points[(i+1)%6]
                    epd.draw_line(p1[0], p1[1], p2[0], p2[1], 0)
                
                # 中心点
                if (row + col) % 3 == 0:
                    epd.fill_circle(int(x), int(y), 5, 0)

    # 13. Spirograph (万花尺) - 内旋轮线
    def art_spirograph():
        R = 150 # 大圆半径
        r = 52  # 小圆半径
        a = 70  # 笔尖距离
        
        prev_x, prev_y = None, None
        for t in range(0, 360 * 10, 5): # 多圈
            rad = math.radians(t)
            # 万花尺方程
            x = (R - r) * math.cos(rad) + a * math.cos(((R - r) / r) * rad)
            y = (R - r) * math.sin(rad) - a * math.sin(((R - r) / r) * rad)
            
            x += CX
            y += CY
            
            if prev_x is not None:
                epd.draw_line(int(prev_x), int(prev_y), int(x), int(y), 0)
            prev_x, prev_y = x, y

    # 14. Voronoi Stipple (Voronoi 晶格/细胞结构)
    def art_voronoi():
        # 随机生成种子点
        seeds = [(random.randint(0, W), random.randint(0, H)) for _ in range(30)]
        
        # 简单绘制：连接相邻点形成类似三角剖分的效果
        for s1 in seeds:
            # 找到最近的几个点连接
            dists = []
            for s2 in seeds:
                if s1 == s2: continue
                d = (s1[0]-s2[0])**2 + (s1[1]-s2[1])**2
                dists.append((d, s2))
            
            dists.sort(key=lambda x: x[0])
            # 连接最近的3个点
            for _, s2 in dists[:3]:
                epd.draw_line(s1[0], s1[1], s2[0], s2[1], 0)
            
            epd.fill_circle(s1[0], s1[1], 4, 0)

    # 15. Op Art (欧普艺术) - 波动线条
    def art_op_waves():
        step = 10
        for y in range(0, H, step):
            prev_x, prev_y = 0, y
            for x in range(0, W, 20):
                # Y 随 X 正弦波动
                offset = 20 * math.sin(x * 0.05 + y * 0.02)
                cur_y = y + offset
                epd.draw_line(prev_x, int(prev_y), x, int(cur_y), 0)
                prev_x, prev_y = x, cur_y

    # 16. Truchet Tiles (Truchet 瓷砖迷宫)
    def art_truchet():
        size = 40
        for y in range(0, H, size):
            for x in range(0, W, size):
                # 随机决定画 / 还是 \
                if random.random() > 0.5:
                    epd.draw_line(x, y, x+size, y+size, 0)
                else:
                    epd.draw_line(x+size, y, x, y+size, 0)
                # 边框增加复杂度
                # epd.draw_rect(x, y, size, size, 0)

    # 17. Ray Casting / Starburst (放射光线)
    def art_starburst():
        # 两个中心点发射光线
        c1 = (W//3, H//2)
        c2 = (2*W//3, H//2)
        
        for i in range(0, 360, 3):
            rad = math.radians(i)
            # Center 1
            x1 = c1[0] + 800 * math.cos(rad)
            y1 = c1[1] + 800 * math.sin(rad)
            epd.draw_line(c1[0], c1[1], int(x1), int(y1), 0)
            
            # Center 2 (Off phase)
            rad2 = math.radians(i + 1.5)
            x2 = c2[0] + 800 * math.cos(rad2)
            y2 = c2[1] + 800 * math.sin(rad2)
            epd.draw_line(c2[0], c2[1], int(x2), int(y2), 0)

    # 18. Circle Packing (圆形填充)
    def art_circle_packing():
        # 随机画互不重叠的圆（简化版：只画大中小圆重叠，利用xor效果或简单覆盖）
        # 真实的 packing 算法太慢，这里模拟视觉效果
        for _ in range(100):
            r = random.randint(10, 80)
            x = random.randint(r, W-r)
            y = random.randint(r, H-r)
            epd.draw_circle(x, y, r, 0)
            epd.draw_circle(x, y, r-3, 0) # 双线

    # 19. Matrix Rain (黑客帝国雨)
    def art_matrix():
        cols = W // 20
        for c in range(cols):
            length = random.randint(5, 20)
            speed_y = random.randint(0, H)
            x = c * 20
            # 画一条竖线模拟字符流
            for i in range(length):
                y = (speed_y + i * 15) % H
                # 模拟不同灰度
                if i == length - 1: # 头部
                    epd.fill_rect(x, y, 12, 12, 0)
                elif i % 2 == 0:
                    epd.draw_text(chr(random.randint(65, 90)), x, y, 0)

    # 20. Radar Chart (雷达图/蜘蛛网)
    def art_radar():
        num_vars = 8
        max_r = 250
        # 网格
        for r in range(50, max_r+1, 50):
            points = []
            for i in range(num_vars):
                ang = math.radians(i * 360 / num_vars)
                points.append((CX + r*math.cos(ang), CY + r*math.sin(ang)))
            for i in range(num_vars):
                p1, p2 = points[i], points[(i+1)%num_vars]
                epd.draw_line(int(p1[0]), int(p1[1]), int(p2[0]), int(p2[1]), 0)
                # 轴线
                epd.draw_line(CX, CY, int(p1[0]), int(p1[1]), 0)
        
        # 数据区
        data_points = []
        for i in range(num_vars):
            val = random.uniform(0.3, 1.0) * max_r
            ang = math.radians(i * 360 / num_vars)
            data_points.append((CX + val*math.cos(ang), CY + val*math.sin(ang)))
        
        # 填充数据区域（用线填充模拟）
        for i in range(len(data_points)):
            p1 = data_points[i]
            p2 = data_points[(i+1)%len(data_points)]
            # 粗线
            epd.draw_line(int(p1[0]), int(p1[1]), int(p2[0]), int(p2[1]), 0)
            epd.draw_line(int(p1[0])+1, int(p1[1])+1, int(p2[0])+1, int(p2[1])+1, 0)

    # 21. Sinusoidal Waves (正弦波干涉)
    def art_sine_waves():
        for y in range(0, H, 5):
            prev_x, prev_py = 0, y
            for x in range(0, W, 10):
                # 两个波叠加
                py = y + 20 * math.sin(x * 0.02) + 10 * math.sin(x * 0.05 + y * 0.1)
                epd.draw_line(prev_x, int(prev_py), x, int(py), 0)
                prev_x, prev_py = x, py

    # 22. Nested Squares (嵌套旋转正方形)
    def art_nested_squares():
        size = 600
        angle = 0
        rect_cx, rect_cy = CX, CY
        
        for i in range(50):
            # 绘制旋转矩形需要计算四个顶点
            half = size / 2
            # 简化：只画同心收缩，E-ink上旋转矩形计算量大且线条易断，改用交错矩形
            epd.draw_rect(int(rect_cx - half), int(rect_cy - half), int(size), int(size), 0)
            
            size *= 0.9
            # 稍微偏移中心
            if i % 2 == 0:
                rect_cx += 5
            else:
                rect_cx -= 5

    # 23. Audio Waveform (音频波形)
    def art_audio_wave():
        # 模拟中间高两边低的波形
        prev_x, prev_y = 0, CY
        for x in range(0, W, 2):
            # 距离中心的衰减
            dist = abs(x - CX) / (W/2)
            amp = 200 * (1 - dist) * random.uniform(0.5, 1.0)
            if amp < 0: amp = 0
            
            y = CY + amp * (1 if x % 4 < 2 else -1)
            epd.draw_line(prev_x, int(prev_y), x, int(y), 0)
            prev_x, prev_y = x, y

    # 24. Binary Tree (二叉树 - 程序员风格)
    def art_binary_tree():
        nodes = [(CX, 40)]
        layer_h = 60
        for layer in range(6):
            next_nodes = []
            for nx, ny in nodes:
                epd.fill_circle(nx, ny, 10, 0)
                # Left child
                lx, ly = nx - W // (2**(layer+2)), ny + layer_h
                epd.draw_line(nx, ny+10, int(lx), int(ly), 0)
                next_nodes.append((int(lx), int(ly)))
                # Right child
                rx, ry = nx + W // (2**(layer+2)), ny + layer_h
                epd.draw_line(nx, ny+10, int(rx), int(ry), 0)
                next_nodes.append((int(rx), int(ry)))
            nodes = next_nodes

    # 25. Mandalas (曼陀罗) - 旋转对称
    def art_mandala():
        petals = 16
        for r in range(20, 250, 20):
            for i in range(petals):
                angle = math.radians(i * 360 / petals)
                # 在圆周上画小圆或菱形
                cx = CX + r * math.cos(angle)
                cy = CY + r * math.sin(angle)
                epd.draw_circle(int(cx), int(cy), r//4, 0)

    # 26. Isometric Cubes (等轴测立方体)
    def art_isometric():
        # 简单的堆叠立方体效果
        size = 40
        for y in range(50, H-50, size):
            for x in range(50, W-50, size):
                # 顶部菱形
                epd.draw_line(x, y, x+size//2, y-size//4, 0)
                epd.draw_line(x+size//2, y-size//4, x+size, y, 0)
                epd.draw_line(x+size, y, x+size//2, y+size//4, 0)
                epd.draw_line(x+size//2, y+size//4, x, y, 0)
                # 侧面垂线
                epd.draw_line(x, y, x, y+size//2, 0)
                epd.draw_line(x+size, y, x+size, y+size//2, 0)
                epd.draw_line(x+size//2, y+size//4, x+size//2, y+3*size//4, 0)
                # 底部连接
                epd.draw_line(x, y+size//2, x+size//2, y+3*size//4, 0)
                epd.draw_line(x+size//2, y+3*size//4, x+size, y+size//2, 0)

    # 27. Circuit Board (电路板)
    def art_circuit():
        # 随机游走的直线
        for _ in range(50):
            x, y = random.randint(0, W), random.randint(0, H)
            epd.fill_circle(x, y, 4, 0) # 焊点
            # 走线
            length = random.randint(50, 200)
            direction = random.choice([(1,0), (0,1), (-1,0), (0,-1)])
            for _ in range(length // 10):
                nx = x + direction[0] * 10
                ny = y + direction[1] * 10
                epd.draw_line(x, y, nx, ny, 0)
                x, y = nx, ny
                if random.random() < 0.2:
                    direction = random.choice([(1,0), (0,1), (-1,0), (0,-1)])

    # 28. Raindrops (雨滴涟漪)
    def art_raindrops():
        for _ in range(30):
            cx = random.randint(50, W-50)
            cy = random.randint(50, H-50)
            max_r = random.randint(20, 60)
            for r in range(5, max_r, 5):
                epd.draw_circle(cx, cy, r, 0)

    # 29. Barcode / Glitch (条形码/故障艺术)
    def art_barcode():
        x = 50
        while x < W - 50:
            w = random.randint(2, 10)
            if random.random() > 0.3:
                epd.fill_rect(x, 100, w, 340, 0)
            x += w + 2

    # 30. Perspective Grid (透视网格/80年代风格)
    def art_perspective_grid():
        # 地平线
        horizon_y = H // 3
        epd.draw_line(0, horizon_y, W, horizon_y, 0)
        # 垂直线消失于一点
        vanish_x, vanish_y = CX, horizon_y
        for x in range(-500, W+500, 50):
            epd.draw_line(vanish_x, vanish_y, x, H, 0)
        # 水平线间距渐变
        y = H
        dy = 50
        while y > horizon_y and dy > 1:  # 添加最小间距限制防止死循环
            epd.draw_line(0, int(y), W, int(y), 0)
            y -= dy
            dy *= 0.8 # 越远越密

    # 31. Confetti (五彩纸屑)
    def art_confetti():
        for _ in range(300):
            x = random.randint(0, W)
            y = random.randint(0, H)
            shape = random.randint(0, 2)
            if shape == 0: epd.fill_rect(x, y, 10, 10, 0)
            elif shape == 1: epd.fill_circle(x, y, 5, 0)
            else: epd.draw_line(x, y, x+10, y+10, 0)

    # 32. Connected Nodes (神经网络)
    def art_network():
        points = [(random.randint(0, W), random.randint(0, H)) for _ in range(60)]
        for i, p in enumerate(points):
            epd.fill_circle(p[0], p[1], 3, 0)
            # 连接附近的点
            count = 0
            for j, p2 in enumerate(points):
                if i == j: continue
                dist = math.sqrt((p[0]-p2[0])**2 + (p[1]-p2[1])**2)
                if dist < 150:
                    epd.draw_line(p[0], p[1], p2[0], p2[1], 0)
                    count += 1
                if count > 3: break

    # 33. Spiral of Squares (螺旋方块)
    def art_spiral_squares():
        ang = 0
        r = 10
        for i in range(100):
            x = CX + r * math.cos(ang)
            y = CY + r * math.sin(ang)
            size = i + 5
            epd.draw_rect(int(x-size/2), int(y-size/2), int(size), int(size), 0)
            ang += 0.5
            r += 3

    # 34. Metaballs (伪元球) - 等高线圆
    def art_metaballs():
        # 用空心圆模拟势场
        balls = [
            (CX-100, CY, 50), (CX+100, CY, 70), (CX, CY-100, 60)
        ]
        for b in balls:
            for r in range(10, 150, 10):
                epd.draw_circle(b[0], b[1], b[2]+r, 0)

    # 35. Mountain Landscape (山脉轮廓)
    def art_mountains():
        # 简单的噪声线
        heights = [H//2] * (W//10 + 1)
        for i in range(1, len(heights)):
            heights[i] = heights[i-1] + random.randint(-20, 20)
            # 限制范围
            if heights[i] < H//4: heights[i] = H//4
            if heights[i] > H//4*3: heights[i] = H//4*3
        
        for i in range(len(heights)-1):
            epd.draw_line(i*10, heights[i], (i+1)*10, heights[i+1], 0)
            # 填充山体(画垂线)
            epd.draw_line(i*10, heights[i], i*10, H, 0)

    # 36. Crosshatch Gradient (交叉线渐变)
    def art_crosshatch():
        # 从左到右变密
        for x in range(0, W, 20):
            density = int(map_range(x, 0, W, 20, 2))
            if density < 2: density = 2
            for y in range(0, H, density):
                epd.draw_line(x, y, x+10, y+10, 0)

    # 37. Polar Grid (极坐标网格)
    def art_polar_grid():
        for r in range(20, 300, 20):
            epd.draw_circle(CX, CY, r, 0)
        for ang in range(0, 360, 15):
            rad = math.radians(ang)
            epd.draw_line(CX, CY, int(CX + 300*math.cos(rad)), int(CY + 300*math.sin(rad)), 0)

    # 38. Dotted Spirals (点状螺旋)
    def art_dotted_spiral():
        a, b = 0, 5
        for t in range(1000):
            ang = t * 0.1
            r = a + b * ang
            x = CX + r * math.cos(ang)
            y = CY + r * math.sin(ang)
            if 0 <= x < W and 0 <= y < H:
                epd.fill_circle(int(x), int(y), 2 + int(t/200), 0)

    # 39. Skyline (城市天际线)
    def art_skyline():
        x = 0
        while x < W:
            w = random.randint(30, 80)
            h = random.randint(100, 400)
            epd.draw_rect(x, H-h, w, h, 0)
            # 窗户
            for wx in range(x+5, x+w-5, 10):
                for wy in range(H-h+10, H-10, 15):
                    if random.random() > 0.3:
                        epd.fill_rect(wx, wy, 5, 8, 0)
            x += w

    # 40. Speed Lines (速度线)
    def art_speed_lines():
        for _ in range(200):
            y = random.randint(0, H)
            len_line = random.randint(50, 300)
            speed = random.randint(1, 5) # 线粗细模拟速度
            epd.fill_rect(random.randint(0, W-len_line), y, len_line, speed, 0)

    # 41. Chains (链条)
    def art_chains():
        y_pos = [100, 250, 400]
        for y in y_pos:
            for x in range(20, W-40, 50):
                epd.draw_rect(x, y, 60, 30, 0) # 环
                epd.draw_line(x+60, y+15, x+70, y+15, 0) # 连接

    # 42. DNA Helix (DNA双螺旋)
    def art_dna():
        points = []
        for x in range(0, W, 10):
            offset = 100 * math.sin(x * 0.02)
            y1 = CY + offset
            y2 = CY - offset
            epd.fill_circle(x, int(y1), 5, 0)
            epd.fill_circle(x, int(y2), 5, 0)
            # 碱基对连接
            if x % 20 == 0:
                epd.draw_line(x, int(y1), x, int(y2), 0)

    # 43. Pie Chart (饼图)
    def art_pie_chart():
        angles = [30, 60, 45, 90, 135]
        start = 0
        for ang in angles:
            # 画扇形边界
            rad_start = math.radians(start)
            rad_end = math.radians(start + ang)
            
            x1 = CX + 200 * math.cos(rad_start)
            y1 = CY + 200 * math.sin(rad_start)
            epd.draw_line(CX, CY, int(x1), int(y1), 0)
            
            # 画弧线（用短线段模拟）
            steps = int(ang)
            for i in range(steps):
                curr = math.radians(start + i)
                next_a = math.radians(start + i + 1)
                p1x = CX + 200 * math.cos(curr)
                p1y = CY + 200 * math.sin(curr)
                p2x = CX + 200 * math.cos(next_a)
                p2y = CY + 200 * math.sin(next_a)
                epd.draw_line(int(p1x), int(p1y), int(p2x), int(p2y), 0)

            # 填充文字标签
            label_ang = math.radians(start + ang/2)
            lx = CX + 240 * math.cos(label_ang)
            ly = CY + 240 * math.sin(label_ang)
            epd.draw_text(f"{ang}", int(lx)-10, int(ly), 0)
            
            start += ang

    # 44. Arrow Grid (箭头阵列)
    def art_arrows():
        for y in range(50, H, 50):
            for x in range(50, W, 50):
                # 随机方向箭头
                direction = random.choice([0, 1, 2, 3]) # U, R, D, L
                if direction == 0: # Up
                    epd.draw_line(x, y+10, x, y-10, 0)
                    epd.draw_line(x, y-10, x-5, y-5, 0)
                    epd.draw_line(x, y-10, x+5, y-5, 0)
                elif direction == 1: # Right
                    epd.draw_line(x-10, y, x+10, y, 0)
                    epd.draw_line(x+10, y, x+5, y-5, 0)
                    epd.draw_line(x+10, y, x+5, y+5, 0)
                # ... 省略其他方向简化代码

    # 45. Ruler / Scale (刻度尺)
    def art_ruler():
        # 主刻度
        for i in range(0, W, 100):
            epd.draw_line(i, H, i, H-50, 0)
            epd.draw_text(str(i), i+5, H-40, 0)
        # 次刻度
        for i in range(0, W, 10):
            h = 20 if i % 50 != 0 else 30
            epd.draw_line(i, H, i, H-h, 0)

    # 46. Brick Path (砖块路径)
    def art_brick_path():
        # 类似 2.5D 游戏地图
        start_x, start_y = 100, H-100
        for i in range(20):
            epd.draw_rect(start_x, start_y, 40, 20, 0)
            start_x += 40
            start_y -= 10 # 上坡
            if i % 5 == 0: start_y -= 20 # 跳跃

    # 47. Histogram (直方图)
    def art_histogram():
        bar_w = 20
        start_x = 50
        for i in range(40):
            h = random.randint(10, 400)
            x = start_x + i * (bar_w + 2)
            y = H - 50 - h
            epd.fill_rect(x, y, bar_w, h, 0)

    # 48. Target Scope (狙击镜)
    def art_scope():
        epd.draw_circle(CX, CY, 250, 0)
        epd.draw_line(CX-300, CY, CX+300, CY, 0)
        epd.draw_line(CX, CY-300, CX, CY+300, 0)
        # 刻度
        for r in range(50, 250, 50):
            epd.draw_line(CX-5, CY+r, CX+5, CY+r, 0)
            epd.draw_line(CX+r, CY-5, CX+r, CY+5, 0)

    # 49. Static Noise (电视雪花)
    def art_noise():
        # 性能警告：只画部分点
        for _ in range(5000):
            x = random.randint(0, W)
            y = random.randint(0, H)
            epd.draw_pixel(x, y, 0)

    # 50. Final Logo
    def art_final():
        epd.fill_rect(0, 0, W, H, 0) # 全黑背景 (如果支持反色显示，否则用白色)
        # 实际上 Papers3 0是黑，255是白。这里画个反白文字框
        epd.fill_rect(CX-150, CY-50, 300, 100, 255)
        epd.draw_rect(CX-150, CY-50, 300, 100, 0)
        epd.draw_text("GALLERY COMPLETE", CX-80, CY-10, 0)

    # --- 注册所有函数 ---
    patterns = [
        ("Maurer Rose", art_maurer_rose),
        ("Phyllotaxis", art_phyllotaxis),
        ("Barnsley Fern", art_fern),
        ("Lissajous Table", art_lissajous_table),
        ("Superformula", art_superformula),
        ("Clifford Attractor", art_clifford_attractor),
        ("Sierpinski", art_sierpinski),
        ("Fractal Tree", art_fractal_tree),
        ("Flow Field", art_flow_field),
        ("String Art", art_string_art),
        ("Hilbert Approx", art_hilbert),
        ("Hex Grid", art_hex_grid),
        ("Spirograph", art_spirograph),
        ("Voronoi Simple", art_voronoi),
        ("Op Art Waves", art_op_waves),
        ("Truchet Tiles", art_truchet),
        ("Starburst", art_starburst),
        ("Circle Packing", art_circle_packing),
        ("Matrix Rain", art_matrix),
        ("Radar Chart", art_radar),
        ("Sine Interference", art_sine_waves),
        ("Nested Rects", art_nested_squares),
        ("Audio Wave", art_audio_wave),
        ("Binary Tree", art_binary_tree),
        ("Mandala", art_mandala),
        ("Isometric", art_isometric),
        ("Circuit Board", art_circuit),
        ("Raindrops", art_raindrops),
        ("Barcode", art_barcode),
        ("Perspective", art_perspective_grid),
        ("Confetti", art_confetti),
        ("Network", art_network),
        ("Spiral Squares", art_spiral_squares),
        ("Metaballs", art_metaballs),
        ("Mountains", art_mountains),
        ("Crosshatch", art_crosshatch),
        ("Polar Grid", art_polar_grid),
        ("Dotted Spiral", art_dotted_spiral),
        ("Skyline", art_skyline),
        ("Speed Lines", art_speed_lines),
        ("Chains", art_chains),
        ("DNA Helix", art_dna),
        ("Pie Chart", art_pie_chart),
        ("Arrows", art_arrows),
        ("Ruler", art_ruler),
        ("Brick Path", art_brick_path),
        ("Histogram", art_histogram),
        ("Scope", art_scope),
        ("Static Noise", art_noise),
        ("Final", art_final)
    ]

    # --- 主播放循环 ---
    print(f"Loaded {len(patterns)} patterns.")
    
    for i, (name, func) in enumerate(patterns):
        print(f"Drawing {i+1}/{len(patterns)}: {name}")
        
        # 1. 准备画布
        epd.fill_rect(0, 0, W, H, 255) # 白底
        
        # 2. 绘制图案
        try:
            func()
        except Exception as e:
            print(f"Error in {name}: {e}")
            
        # 3. 绘制UI覆盖层 (底部状态栏)
        # 用一块白色区域垫底，防止文字看不清
        epd.fill_rect(0, H-40, W, 40, 255)
        epd.draw_line(0, H-40, W, H-40, 0)
        epd.draw_text(f"{i+1}. {name}", 20, H-30, 0)
        
        # 进度条
        progress = int((i + 1) / len(patterns) * W)
        epd.fill_rect(0, H-5, progress, 5, 0)

        # 4. 刷新屏幕
        epd.update()
        
        # 5. 暂停展示
        time.sleep(2.5)
        
        # 6. 内存管理 (关键)
        if i % 5 == 0:
            gc.collect()

    print("Gallery cycle finished.")
    epd.deinit()

if __name__ == '__main__':
    main()