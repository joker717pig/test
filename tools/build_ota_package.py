#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_ota_package.py — EDA_EMG OTA 升级包生成脚本

功能：
  1. 复制 ESP32 固件、图片固件、STM32 固件到输出目录 eda_emg/
  2. STM32: Intel HEX (.hex) → raw binary (.bin) 转换（IAR 产物）
  3. 对每个固件计算 sha256（app/image）；stm32 用 CRC32（反射 0xEDB88320，与 Boot crc32.c 一致）
  4. 生成 version.json（云端清单，与 main/ota/app_ota.c 的 parse_manifest 对应）

目录结构（输出 eda_emg/，可直接整夹上传到云端 .../pelvicFloorMuscleUpgradeKit/fota/eda_emg/）：
  eda_emg/
  ├── version.json
  ├── esp32/eda_emg_esp32.bin
  ├── image/storage_a.bin
  └── stm32/fw.bin

正式云端（2026-09-07 起，设备端 app_ota.h 已指向此地址）：
  http://health.eda-global.cn/pelvicFloorMuscleUpgradeKit/fota/eda_emg/version.json
  ⚠️ 该服务器【没有 FTP】，打包后请把 eda_emg/ 整个目录【手动上传】到云端对应路径。

FTP 上传（可选，加 --upload；仅旧测试服务器 dgbuysmart 可用，正式云端勿用）：
  - 旧服务器 203.170.59.165 = www.dgbuysmart.com（HTTP 与 FTP 同一台机器）
  - FTP 根目录即 HTTP 的 /fota/lzq3496900/，其下 eda_emg/ 对应旧云端 .../eda_emg/
  - 把本地 eda_emg/ 输出目录（version.json + esp32/image/stm32）递归上传到 FTP 的 eda_emg/

用法：
  python tools/build_ota_package.py [--esp32-ver 2.0] [--image-ver 1.0] [--stm32-ver 1.0]
                                    [--out D:\\...\\eda_emg] [--upload]
  版本号默认自动从固件提取(ESP32 APP_FW_VERSION / STM32 FW_VERSION / 图片 image_ver.txt)，
  与设备自报版本严格一致；--xxx-ver 仅在需要临时覆盖时手动指定。

仅依赖 Python 标准库（hashlib/struct/pathlib/ftplib）。
"""

import argparse
import ftplib
import hashlib
import os
import re
import struct
import sys
import zlib
from pathlib import Path
from typing import Optional

# ---- 默认路径（相对本脚本所在目录向上到工程根） ----
PROJECT_ROOT = Path(__file__).resolve().parent.parent   # tools/ -> 工程根
ESP32_BIN     = PROJECT_ROOT / "build" / "eda_emg_esp32.bin"
IMAGE_BIN     = PROJECT_ROOT / "build" / "storage_a.bin"
STM32_HEX     = Path(r"D:\GitLab\EDA_EMG\EWARM\EDA_EMG\Exe\EDA_EMG.hex")

# ---- 版本号自动提取来源（正式版：version.json 版本号与固件内部版本号严格一致）----
ESP32_VER_HEADER = PROJECT_ROOT / "main" / "ota" / "app_ota.h"        # APP_FW_VERSION (0xMMmm)
IMAGE_VER_FILE   = PROJECT_ROOT / "spiffs_data" / "image_ver.txt"      # 图片槽版本 "主.次"
STM32_VER_HEADER = Path(r"D:\GitLab\EDA_EMG\code\MODBUS\MB_RegMap.h")  # FW_VERSION (0xMMmm)

# ---- FTP 上传配置（仅旧测试服务器 dgbuysmart；正式云端 health.eda-global.cn 无 FTP，手动上传）----
# 旧服务器 www.dgbuysmart.com 解析到 203.170.59.165；FTP 根目录 = HTTP 的 /fota/lzq3496900/
# 正式云端地址见 main/ota/app_ota.h 的 APP_OTA_BASE_URL（health.eda-global.cn，无 FTP）。
FTP_HOST     = "203.170.59.165"
FTP_PORT     = 21
FTP_USER     = "lzq3496900"
FTP_PASS     = "lzq2477981"
FTP_REMOTE_ROOT = "eda_emg"   # 上传到 FTP 根的 eda_emg/（对应云端 .../eda_emg/）


def sha256_hex(path: Path) -> str:
    """返回文件 sha256 的小写 hex 字符串。"""
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def crc32_hex(path: Path) -> str:
    """返回文件 CRC32 (ISO-HDLC, reflected 0xEDB88320) 的小写 8 位 hex。
    与 STM32 Boot crc32.c 逐字节一致 (zlib.crc32)。"""
    crc = 0
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            crc = zlib.crc32(chunk, crc)
    return format(crc & 0xFFFFFFFF, "08x")


def ver_hex_to_str(v: int) -> str:
    """16 位版本号 0xMMmm -> "主.次"（高字节主版本，低字节次版本）。0x0200 -> '2.0'。"""
    return f"{(v >> 8) & 0xFF}.{v & 0xFF}"


def extract_hex_define(header: Path, macro: str) -> int:
    """从 C 头文件解析 #define MACRO 0x....（或十进制）的整数值。"""
    txt = header.read_text(encoding="utf-8", errors="ignore")
    m = re.search(rf"#\s*define\s+{macro}\s+(0[xX][0-9a-fA-F]+|\d+)", txt)
    if not m:
        raise ValueError(f"在 {header} 找不到 #define {macro}")
    return int(m.group(1), 0)


def has_stm32_source() -> bool:
    """同事电脑可能没有 EDA_EMG(STM32) 仓库：固件 hex 与版本头都在才视为有 STM32 目标。"""
    return STM32_HEX.exists() and STM32_VER_HEADER.exists()


def autodetect_versions(include_stm32: bool):
    """从三目标固件自动提取真实版本号，返回 (esp32, image, stm32_or_None) 的 "主.次" 字符串。
    include_stm32=False（同事电脑无 STM32 源码）时 stm32 返回 None，打包会跳过该目标。"""
    esp32 = ver_hex_to_str(extract_hex_define(ESP32_VER_HEADER, "APP_FW_VERSION"))
    image = IMAGE_VER_FILE.read_text(encoding="utf-8").strip()
    stm32 = None
    if include_stm32:
        stm32 = ver_hex_to_str(extract_hex_define(STM32_VER_HEADER, "FW_VERSION"))
    return esp32, image, stm32


def hex_to_bin(hex_path: Path, out_bin: Path) -> int:
    """
    将 Intel HEX 转换为 raw binary。
    返回写入的字节数（按 min_addr 对齐后的镜像大小）。
    仅支持标准 Intel HEX（记录类型 00/01/02/04），IAR 输出满足。
    STM32 固件带 extended linear address 基址（0x08000000），
    这里把所有 data 段相对最小起始地址归零，得到与 Flash 0x08000000
    对齐的裸镜像（OTA 烧写时地址即相对 0x08000000 的偏移）。
    """
    base = 0
    segments = []   # (绝对地址, 字节串)
    min_addr = None

    with open(hex_path, "r", encoding="ascii") as f:
        for line in f:
            line = line.strip()
            if not line or not line.startswith(":"):
                continue
            raw = bytes.fromhex(line[1:])
            byte_count = raw[0]
            addr = (raw[1] << 8) | raw[2]
            rectype = raw[3]
            payload = raw[4:4 + byte_count]

            if rectype == 0x00:  # data record
                target_addr = base + addr
                segments.append((target_addr, payload))
                if min_addr is None or target_addr < min_addr:
                    min_addr = target_addr
            elif rectype == 0x01:  # EOF
                break
            elif rectype == 0x02:  # extended segment address (base << 4)
                base = ((payload[0] << 8) | payload[1]) << 4
            elif rectype == 0x04:  # extended linear address (base << 16)
                base = ((payload[0] << 8) | payload[1]) << 16
            elif rectype in (0x03, 0x05):  # start address, ignore
                continue
            else:
                print(f"  [warn] 未知记录类型 {rectype:02X}，已跳过")

    if min_addr is None:
        print("  [错误] hex 文件无数据记录")
        return 0

    data = bytearray()
    for abs_addr, payload in segments:
        off = abs_addr - min_addr
        if off + len(payload) > len(data):
            data.extend(b"\xff" * (off + len(payload) - len(data)))
        data[off:off + len(payload)] = payload

    # 4 字节对齐 (Boot 写 Flash 按 word 写, 要求 size%4==0; 补 0xFF 不影响运行)
    while len(data) % 4 != 0:
        data.append(0xFF)

    out_bin.parent.mkdir(parents=True, exist_ok=True)
    with open(out_bin, "wb") as f:
        f.write(data)

    return len(data)


def build_version_json(esp32_ver: str, esp32_size: int, esp32_sha: str,
                       image_ver: str, image_size: int, image_sha: str,
                       stm32_ver: Optional[str], stm32_size: int, stm32_crc: str) -> str:
    """生成 version.json 文本（JSON 手写，与 app_ota.c 字段一致）。
    esp32/image/stm32 各有**独立**的版本号，分别判断是否升级。
    stm32_ver=None（同事电脑无 STM32 源码）时省略 stm32 项，设备端会自动跳过该目标。
    stm32 用 crc32 (与 Boot crc32.c 一致)，esp32/image 用 sha256。"""
    lines = [
        f'    "esp32": {{ "version": "{esp32_ver}", "url": "esp32/eda_emg_esp32.bin", "sha256": "{esp32_sha}", "size": {esp32_size} }}',
        f'    "image": {{ "version": "{image_ver}", "url": "image/storage_a.bin", "sha256": "{image_sha}", "size": {image_size} }}',
    ]
    if stm32_ver is not None:
        lines.append(
            f'    "stm32": {{ "version": "{stm32_ver}", "url": "stm32/fw.bin", "crc32": "{stm32_crc}", "size": {stm32_size} }}'
        )
    body = ",\n".join(lines)
    return "{\n  \"targets\": {\n" + body + "\n  }\n}\n"


def load_old_stm32(out_root: Path):
    """读取输出目录里已存在的 version.json：若含 stm32 条目且 stm32/fw.bin 仍在，
    返回旧 stm32 信息 dict(version/crc32/size)，否则返回 None。
    用于同事电脑无 STM32 源码时，沿用上一次已生成好的 stm32 目标，避免覆盖丢失。"""
    import json
    manifest = out_root / "version.json"
    if not manifest.exists():
        return None
    try:
        data = json.loads(manifest.read_text(encoding="utf-8"))
        stm32 = data.get("targets", {}).get("stm32")
    except Exception:
        return None
    if not isinstance(stm32, dict):
        return None
    fw = out_root / "stm32" / "fw.bin"
    if not fw.exists():
        return None
    return {
        "version": str(stm32.get("version", "0.0")),
        "crc32": str(stm32.get("crc32", "")),
        "size": int(stm32.get("size", 0)),
    }


def _ftp_mkdirs(ftp: ftplib.FTP, remote_dir: str):
    """在 FTP 上逐级创建目录（相对当前 cwd；已存在则跳过）。"""
    for part in remote_dir.strip("/").split("/"):
        if not part:
            continue
        try:
            ftp.cwd(part)
        except ftplib.error_perm:
            ftp.mkd(part)
            ftp.cwd(part)


def ftp_upload_tree(local_root: Path, remote_root: str) -> int:
    """把本地目录树递归上传到 FTP 的 remote_root 下。

    返回成功上传的文件数；失败抛异常由调用方处理。
    注意：增量覆盖；不会删除云端多余文件。
    """
    ftp = ftplib.FTP()
    ftp.connect(FTP_HOST, FTP_PORT, timeout=30)
    ftp.login(FTP_USER, FTP_PASS)
    ftp.set_pasv(True)   # 被动模式（参考脚本 UsePassive=$true）
    try:
        home = ftp.pwd()          # 登录后的绝对根（通常 "/"）
        _ftp_mkdirs(ftp, remote_root)
        remote_abs = home.rstrip("/") + "/" + remote_root.strip("/")

        uploaded = 0
        for dirpath, _dirnames, filenames in os.walk(local_root):
            rel_dir = Path(dirpath).relative_to(local_root)
            # 先回到远程根，再逐级进入当前子目录
            ftp.cwd(remote_abs)
            if str(rel_dir) != ".":
                for part in rel_dir.parts:
                    try:
                        ftp.cwd(part)
                    except ftplib.error_perm:
                        ftp.mkd(part)
                        ftp.cwd(part)
            for name in filenames:
                local_file = Path(dirpath) / name
                remote_rel = (rel_dir / name).as_posix()
                with open(local_file, "rb") as fp:
                    ftp.storbinary(f"STOR {name}", fp)   # 已处于目标目录，仅存文件名
                uploaded += 1
                print(f"  [FTP] 已上传: {remote_rel}")
        return uploaded
    finally:
        try:
            ftp.quit()
        except Exception:
            ftp.close()


def main(argv):
    parser = argparse.ArgumentParser(description="EDA_EMG OTA 升级包生成")
    # 版本号统一两位（主.次），与下位机 16 位 0xMMmm 口径一致（第三位 patch 忽略）。
    # 默认 None = 自动从固件提取；仅当需要临时覆盖时才用命令行指定。
    parser.add_argument("--esp32-ver", default=None, help="ESP32 版本（主.次）；不填自动取 APP_FW_VERSION")
    parser.add_argument("--image-ver", default=None, help="图片版本（主.次）；不填自动取 spiffs_data/image_ver.txt")
    parser.add_argument("--stm32-ver", default=None, help="STM32 版本（主.次）；不填自动取 FW_VERSION")
    parser.add_argument("--out", default=str(PROJECT_ROOT / "eda_emg"),
                        help="输出目录（默认 <工程根>/eda_emg）")
    parser.add_argument("--upload", action="store_true",
                        help="打包完成后通过 FTP 上传到云端（203.170.59.165 / eda_emg/）")
    args = parser.parse_args(argv)

    out_root = Path(args.out)

    # ---- 0. 版本号：默认自动从固件提取（与设备自报版本严格一致）；命令行参数可覆盖 ----
    has_stm32 = has_stm32_source()

    # 无本机 STM32 源码时，若输出目录已有旧 stm32 目标（version.json 含 stm32 且 stm32/fw.bin 在），
    # 则原样沿用，不要因为重跑打包就丢掉已有的 stm32 信息。
    old_stm32 = load_old_stm32(out_root) if not has_stm32 else None
    if old_stm32 is not None:
        print(f"== 检测到已有 STM32 目标（本机无源码，沿用）: version={old_stm32['version']} size={old_stm32['size']} ==")

    print("== 版本号（正式版与固件一致）==")
    try:
        auto_esp32, auto_image, auto_stm32 = autodetect_versions(has_stm32)
    except Exception as e:
        print(f"[错误] 自动提取版本号失败: {e}")
        print("       可用 --esp32-ver/--image-ver/--stm32-ver 手动指定。")
        return 1
    esp32_ver = args.esp32_ver or auto_esp32
    image_ver = args.image_ver or auto_image
    if has_stm32:
        stm32_ver = args.stm32_ver or auto_stm32
    elif old_stm32 is not None:
        stm32_ver = args.stm32_ver or old_stm32["version"]
    else:
        stm32_ver = None
    print(f"  esp32 : {esp32_ver}  ({'命令行指定' if args.esp32_ver else '自动 APP_FW_VERSION'})")
    print(f"  image : {image_ver}  ({'命令行指定' if args.image_ver else '自动 image_ver.txt'})")
    if has_stm32:
        print(f"  stm32 : {stm32_ver}  ({'命令行指定' if args.stm32_ver else '自动 FW_VERSION'})")
    elif old_stm32 is not None:
        print(f"  stm32 : {stm32_ver}  ({'命令行指定' if args.stm32_ver else '沿用已有目标'})")
    elif args.stm32_ver:
        print(f"  [warn] 已忽略 --stm32-ver={args.stm32_ver}（无 STM32 目标）")
    else:
        print("  stm32 : (无 STM32 目标，本次省略)")

    # ---- 1. 校验源文件存在 ----
    for f, name in [(ESP32_BIN, "ESP32 固件"), (IMAGE_BIN, "图片固件")]:
        if not f.exists():
            print(f"[错误] 找不到 {name}: {f}")
            print("       请先编译 ESP32 (idf.py build)。")
            return 1
    if not has_stm32 and old_stm32 is None:
        print("[提示] 未找到 STM32 固件，且无已有 STM32 目标，本次省略 stm32 项。")

    # ---- 2. 目录结构 ----
    esp32_dir = out_root / "esp32"
    image_dir = out_root / "image"
    stm32_dir = out_root / "stm32"
    dirs = [esp32_dir, image_dir]
    if has_stm32 or old_stm32 is not None:
        dirs.append(stm32_dir)
    else:
        # 既无本机源码、又无旧 stm32 目标：清掉输出目录里残留的 stm32/，避免整目录上传带上旧文件
        if stm32_dir.exists():
            import shutil as _sh
            _sh.rmtree(stm32_dir)
            print("[提示] 已清理残留的 stm32/ 目录（无 STM32 目标）")
    for d in dirs:
        d.mkdir(parents=True, exist_ok=True)

    # ---- 3. 复制 ESP32 / image 固件 ----
    esp32_out = esp32_dir / "eda_emg_esp32.bin"
    image_out = image_dir / "storage_a.bin"

    import shutil
    print("== 复制固件 ==")
    shutil.copyfile(ESP32_BIN, esp32_out)
    esp32_size = esp32_out.stat().st_size
    print(f"  esp32: {esp32_out.name}  ({esp32_size} B)")

    shutil.copyfile(IMAGE_BIN, image_out)
    image_size = image_out.stat().st_size
    print(f"  image: {image_out.name}  ({image_size} B)")

    # ---- 4. STM32 hex -> bin ----
    stm32_out = stm32_dir / "fw.bin"
    if has_stm32:
        print("== STM32 hex -> bin ==")
        stm32_size = hex_to_bin(STM32_HEX, stm32_out)
        print(f"  stm32: {stm32_out.name}  ({stm32_size} B)")
    elif old_stm32 is not None:
        print("== STM32 hex -> bin ==  [沿用已有 stm32/fw.bin，不重新生成]")
        stm32_size = old_stm32["size"]
    else:
        print("== STM32 hex -> bin ==  [跳过：无 STM32 目标]")
        stm32_size = 0

    # ---- 5. 计算校验值 ----
    print("== 计算校验值 ==")
    esp32_sha = sha256_hex(esp32_out)
    image_sha = sha256_hex(image_out)
    print(f"  esp32: {esp32_sha} (sha256)")
    print(f"  image: {image_sha} (sha256)")
    stm32_crc = ""
    if has_stm32:
        stm32_crc = crc32_hex(stm32_out)
        print(f"  stm32: {stm32_crc} (crc32)")
    elif old_stm32 is not None:
        stm32_crc = old_stm32["crc32"]
        print(f"  stm32: {stm32_crc} (crc32，沿用)")

    # ---- 6. 生成 version.json ----
    print("== 生成 version.json ==")
    json_text = build_version_json(esp32_ver, esp32_size, esp32_sha,
                                   image_ver, image_size, image_sha,
                                   stm32_ver, stm32_size, stm32_crc)
    with open(out_root / "version.json", "w", encoding="utf-8") as f:
        f.write(json_text)
    print(json_text)

    print(f"\n[完成] 升级包已生成到: {out_root}")
    print("       上传时把整个 eda_emg 文件夹内容手动传到正式云端即可：")
    print("       http://health.eda-global.cn/pelvicFloorMuscleUpgradeKit/fota/eda_emg/")

    # ---- 7. 可选 FTP 上传 ----
    if args.upload:
        print("\n== FTP 上传 ==")
        print(f"  目标: ftp://{FTP_HOST}:{FTP_PORT}/{FTP_REMOTE_ROOT}/")
        try:
            n = ftp_upload_tree(out_root, FTP_REMOTE_ROOT)
            print(f"[完成] FTP 上传成功，共 {n} 个文件。")
        except Exception as e:
            print(f"[错误] FTP 上传失败: {e}")
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))