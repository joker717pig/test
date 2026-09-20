#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
gen_charge_icon.py — 生成 LVGL v9 bin 充电（闪电）图标
输出: EDA_EMG_ESP32/spiffs_data/png/menu/charge.bin

格式对齐同事 battery.bin（已实测解析）:
  magic=0x19, cf=0x14 (LV_COLOR_FORMAT_RGB565A8),
  12B 头 <BBHHHHH : magic cf flags(2) w(2) h(2) stride(2) rsv(2)
  → RGB565 像素数组(w*h*2, 小端) → A8 alpha 数组(w*h)
用法: py tools/gen_charge_icon.py
"""
import struct
from PIL import Image, ImageDraw

W = H = 14                     # 与 Frame.c 中 Img_Charge 尺寸一致
COLOR = (255, 215, 0)          # 金色 #FFD700
OUT = r"d:/GitLab/EDA_EMG_ESP32/spiffs_data/png/menu/charge.bin"

# ---- 闪电多边形（14x14 画布，经典下向闪电）----
PTS = [(9, 0), (4, 8), (7, 8), (5, 14), (12, 6), (8, 6)]


def rgb565(r, g, b):
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def main():
    # 4x 放大绘制 + LANCZOS 缩小 → 平滑半透明边缘（抗锯齿）
    S = 4
    big = Image.new("RGBA", (W * S, H * S), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.polygon([(x * S, y * S) for x, y in PTS], fill=COLOR + (255,))
    img = big.resize((W, H), Image.LANCZOS)

    rgb = bytearray()
    alpha = bytearray()
    for y in range(H):
        for x in range(W):
            r, g, b, a = img.getpixel((x, y))
            rgb += struct.pack("<H", rgb565(r, g, b))
            alpha.append(a)

    hdr = struct.pack("<BBHHHHH", 0x19, 0x14, 0, W, H, W * 2, 0)
    with open(OUT, "wb") as f:
        f.write(hdr)
        f.write(rgb)
        f.write(alpha)

    print("OK:", OUT)
    print("size:", len(hdr) + len(rgb) + len(alpha), "B")


if __name__ == "__main__":
    main()
