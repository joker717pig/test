#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
reorg_spiffs_png.py — 把同事 png/ 目录镜像到 ESP32 spiffs_data/png/
用法: py tools/reorg_spiffs_png.py

来源: D:/GitLab/EDA_EMG_LVGL_PC/png/  →  目标: D:/GitLab/EDA_EMG_ESP32/spiffs_data/png/
- 保留子目录结构（font/menu/ThePage/AssPage/page2/PosRehPage/SetPage）
- 字体超长名改名: lv_font_honorans_medium_14.bin → lv_font_14.bin（SPIFFS 32B 限制）
- 校验每个 .bin 的 LVGL9 12B 头（magic=0x19）并打印 w/h/cf
- 检查旧 spiffs_data/{icon,image,font} 里哪些文件未被镜像覆盖（报告孤儿，不删除）
"""
import os
import struct
import shutil

SRC = r"D:/GitLab/EDA_EMG_LVGL_PC/png"
DST = r"D:/GitLab/EDA_EMG_ESP32/spiffs_data/png"
OLD_DIRS = [r"D:/GitLab/EDA_EMG_ESP32/spiffs_data/icon",
            r"D:/GitLab/EDA_EMG_ESP32/spiffs_data/image",
            r"D:/GitLab/EDA_EMG_ESP32/spiffs_data/font"]
RENAME = {"lv_font_honorans_medium_14.bin": "lv_font_14.bin"}


def check_lvgl_header(path):
    """LVGL9 二进制图 12B 头: magic(1) cf(1) flags(2) w(2) h(2) stride(2) rsv(2)"""
    try:
        with open(path, "rb") as f:
            hdr = f.read(12)
        if len(hdr) < 12:
            return "BAD(short)"
        magic, cf, flags, w, h, stride, rsv = struct.unpack("<BBHHHHH", hdr)
        if magic != 0x19:
            return "BAD(magic!=0x19)"
        return "OK  cf=%d fl=0x%04x %dx%d st=%d rsv=%d" % (cf, flags, w, h, stride, rsv)
    except Exception as e:
        return "ERR %s" % e


def main():
    total = 0
    total_bytes = 0
    mirror = {}  # 相对路径 -> 绝对路径，用于孤儿检查
    for root, dirs, files in os.walk(SRC):
        rel = os.path.relpath(root, SRC)
        sub = "" if rel == "." else rel.replace("\\", "/")
        bin_files = [n for n in files if n.lower().endswith(".bin")]
        if not bin_files:
            continue
        dst_dir = os.path.join(DST, sub) if sub else DST
        os.makedirs(dst_dir, exist_ok=True)
        for name in sorted(bin_files):
            dst_name = RENAME.get(name, name)
            src_path = os.path.join(root, name)
            dst_path = os.path.join(dst_dir, dst_name)
            shutil.copy2(src_path, dst_path)
            sz = os.path.getsize(dst_path)
            total += 1
            total_bytes += sz
            key = (sub + "/" if sub else "") + dst_name
            mirror[key] = dst_path
            print("%-44s %8d B  %s" % (key, sz, check_lvgl_header(dst_path)))

    # 旧目录孤儿检查（仅报告）
    print("\n--- 旧目录孤儿检查（仅报告，不删除） ---")
    orphan = []
    for old in OLD_DIRS:
        if not os.path.isdir(old):
            continue
        for name in sorted(os.listdir(old)):
            if name.lower().endswith(".bin") and name not in mirror:
                orphan.append(os.path.join(old, name))
    if orphan:
        print("以下旧文件未在 png 镜像中（确认代码无引用后可删）:")
        for p in orphan:
            print("  -", p)
    else:
        print("无孤儿：旧 icon/image/font 全部被 png 镜像覆盖")

    print("\nDONE: 复制 %d 个文件，共 %.1f KB -> %s" % (total, total_bytes / 1024.0, DST))


if __name__ == "__main__":
    main()
