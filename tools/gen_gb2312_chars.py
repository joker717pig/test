# -*- coding: utf-8 -*-
"""
生成 GB2312 全量 6763 个汉字的清单（LVGL 中文字库 1.7 用）。

思路：遍历 GB2312 双字节编码 0xA1A1..0xF7FE，用 'gb2312' 解码得到字符，
筛出 CJK 统一汉字（U+4E00..U+9FFF）即 GB2312 的全部 6763 个汉字
（区位表映射 Unicode，避免直接用 U+4E00-9FA5 全量 20902 字）。

产物（相对本文件所在目录 tools/）：
  gb2312_chars.txt      6763 汉字组成的 UTF-8 字符串（lv_font_conv --symbols 用）
  gb2312_codepoints.txt 6763 个十六进制码点，逗号分隔（--range 备用）
"""
import os

OUT_DIR = os.path.dirname(os.path.abspath(__file__))

cps = []
chars = []
for hi in range(0xA1, 0xF8):        # 区 1~87：0xA1-0xF7
    for lo in range(0xA1, 0xFF):    # 位 1~94：0xA1-0xFE
        try:
            ch = bytes([hi, lo]).decode('gb2312')
        except UnicodeDecodeError:
            continue                 # 空位（未定义字符）跳过
        if '\u4e00' <= ch <= '\u9fff':
            cps.append(ord(ch))
            chars.append(ch)

print('count =', len(chars))
assert len(chars) == 6763, 'GB2312 汉字数应恰为 6763，实际 %d' % len(chars)

with open(os.path.join(OUT_DIR, 'gb2312_chars.txt'), 'w', encoding='utf-8') as f:
    f.write(''.join(chars))

with open(os.path.join(OUT_DIR, 'gb2312_codepoints.txt'), 'w', encoding='ascii') as f:
    f.write(','.join('0x%04X' % c for c in cps))

# 抽查业务字（处方/治疗界面常用词应全部命中）
PROBES = '盆底肌电极治疗评估开始停止强度处方模式频率波形分析康复训练参数设置电量充电'
missing = [p for p in PROBES if p not in chars]
print('probes missing:', missing)
print('OK: tools/gb2312_chars.txt (%d chars, %d bytes) + gb2312_codepoints.txt written'
      % (len(chars), len(''.join(chars).encode('utf-8'))))
