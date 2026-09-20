# -*- coding: utf-8 -*-
"""验证 LVGL 字库 .c 的 cmap 字符集 == GB2312 全量 6763 字 + ASCII。

解析 lv_font_conv 生成的 lvgl 格式 .c：
  - cmaps[] 每个 entry：.range_start / .range_length / .list_length / .type
    / .unicode_list / .glyph_id_ofs_list
  - 重建字符集：
      FORMAT0_TINY/FULL : range_start..range_start+range_length-1（连续）
      SPARSE_TINY/FULL  : unicode_list 存差值（首元素相对 range_start，
                          后续相对前一个字符）
比对 tools/gb2312_chars.txt（6763 字）+ ASCII 0x20-0x7F。
"""
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
C_FILE = os.path.join(ROOT, 'main', 'ui', 'font', 'lv_font_simhei_24px.c')
GB_FILE = os.path.join(ROOT, 'tools', 'gb2312_chars.txt')

text = open(C_FILE, encoding='utf-8').read()

# 1) 提取 lv_font_conv 生成的 unicode_list / glyph_id_ofs_list 数组
arrays = {}
arr_re = re.compile(
    r'static const (?:lv_)?(?:u?int(?:8|16|32)_t)\s+(\w+)\[\]\s*=\s*\{(.*?)\};',
    re.S)
for m in arr_re.finditer(text):
    name = m.group(1)
    body = m.group(2)
    vals = []
    for x in re.findall(r'0x[0-9A-Fa-f]+|\d+', body):
        vals.append(int(x, 16) if x.lower().startswith('0x') else int(x))
    arrays[name] = vals

# 2) 提取 cmaps[] 每个 entry 的字段
entry_re = re.compile(
    r'\{\s*\.range_start\s*=\s*(\d+),\s*\.range_length\s*=\s*(\d+),\s*'
    r'\.glyph_id_start\s*=\s*(\d+),\s*\.unicode_list\s*=\s*(\w+|NULL),\s*'
    r'\.glyph_id_ofs_list\s*=\s*(\w+|NULL),\s*\.list_length\s*=\s*(\d+),\s*'
    r'\.type\s*=\s*LV_FONT_FMT_TXT_CMAP_(\w+)', re.S)

chars = set()
for m in entry_re.finditer(text):
    rs, rl, _gs, ul, gol, ll, typ = (int(m.group(1)), int(m.group(2)),
                                     int(m.group(3)), m.group(4), m.group(5),
                                     int(m.group(6)), m.group(7))
    if typ == 'FORMAT0_TINY':
        # 连续区间：range_start .. range_start+range_length-1 全部收录
        for i in range(rl):
            chars.add(rs + i)
    elif typ == 'FORMAT0_FULL':
        # unicode 连续，但 glyph_id_ofs_list[i]==0（且非首位）表示缺字
        ofs_vals = arrays.get(gol, [])
        for i in range(rl):
            if ofs_vals and ofs_vals[i] == 0 and i != 0:
                continue
            chars.add(rs + i)
    elif typ in ('SPARSE_TINY', 'SPARSE_FULL'):
        # unicode_list 存相对 range_start 的偏移，逐个加回
        for v in arrays.get(ul, []):
            chars.add(rs + v)
    else:
        print('!! 未知 cmap type:', typ)

# 3) 与 GB2312 全集比对
gb = open(GB_FILE, encoding='utf-8').read()
gb_cps = {ord(c) for c in gb}
# lv_font_conv -r 0x20-0x7F 实际收录 0x20-0x7E（0x7F DEL 控制字符无字形，正常排除）
ascii_cps = set(range(0x20, 0x7F))

missing = sorted(gb_cps - chars)
extra = sorted(chars - gb_cps - ascii_cps)
ascii_missing = sorted(ascii_cps - chars)

print('cmap glyph 数 =', len(chars))
print('GB2312 期望   =', len(gb_cps), ' ASCII 期望 =', len(ascii_cps))
print('缺字(GB2312 不在 cmap) =', len(missing), missing[:20])
print('多余(非 GB2312/ASCII)  =', len(extra), extra[:20])
print('缺 ASCII               =', len(ascii_missing), ascii_missing[:20])

# 业务字抽查
probes = '盆底肌电极治疗评估开始停止强度处方模式频率波形分析康复训练参数设置电量充电'
pm = [p for p in probes if ord(p) not in chars]
print('业务字缺失:', pm)

ok = not missing and not ascii_missing and len(extra) == 0
print('VERIFY:', 'PASS' if ok else 'FAIL')
