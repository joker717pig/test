#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_spiffs_refs.py — 校验代码里的 png/ 引用与 spiffs_data/png 镜像一致
用法: py tools/check_spiffs_refs.py [--root 目录]

扫描 main/ui_page（可加 --root 指定别的目录，如 LVGL_PC 全部页面），
提取所有 png/<dir>/<file>.bin 引用，检查 spiffs_data/png/<dir>/<file> 是否存在，
并列出镜像中未被任何引用命中的文件（孤儿，供清理）。
"""
import os
import re
import sys

CODE_ROOT = r"D:/GitLab/EDA_EMG_ESP32/main/ui_page"
PNG = r"D:/GitLab/EDA_EMG_ESP32/spiffs_data/png"
PAT = re.compile(r"png/([A-Za-z0-9_]+)/([^\"'\)]+\.bin)")


def main():
    root = CODE_ROOT
    if "--root" in sys.argv:
        root = sys.argv[sys.argv.index("--root") + 1]
    refs = set()
    for rp, _, fs in os.walk(root):
        for fn in fs:
            if not fn.endswith((".c", ".h")):
                continue
            try:
                txt = open(os.path.join(rp, fn), encoding="utf-8", errors="ignore").read()
            except OSError:
                continue
            refs.update((d, f) for d, f in PAT.findall(txt))

    print("== 代码引用 (%d 个，扫 %s) ==" % (len(refs), root))
    miss = []
    for d, f in sorted(refs):
        ok = os.path.isfile(os.path.join(PNG, d, f))
        if not ok:
            miss.append((d, f))
        print("  %-4s %s/%s" % ("OK" if ok else "MISS", d, f))
    print("MISSING: %d 个" % len(miss))

    # 镜像孤儿（无引用命中）
    have = set()
    for rp, _, fs in os.walk(PNG):
        rel = os.path.relpath(rp, PNG).replace("\\", "/")
        for fn in fs:
            if fn.lower().endswith(".bin"):
                d = "" if rel == "." else rel
                have.add((d, fn))
    orphan = sorted(have - refs)
    print("== 镜像孤儿（代码未引用，可考虑删）: %d ==" % len(orphan))
    for d, f in orphan:
        print("  %s/%s" % (d, f))

    return 0 if not miss else 1


if __name__ == "__main__":
    sys.exit(main())
