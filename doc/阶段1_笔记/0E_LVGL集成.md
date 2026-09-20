# 阶段 1 LVGL 集成 — 工程/坑笔记（0E）

> 主计划与勾选清单见 `doc/开发计划.md`「阶段 1」。本文档按子阶段记录坑与结论。
> 状态（2026-08-07）：**阶段 1 完成（1.3~1.11）**——LVGL 渲染链路 + 全量中文字库 + demo 屏 + 性能测量全部通过。
> 阶段 1 目标：LVGL **9.5.0** 渲染链路通 + 24px 全量 GB2312 中文字库 + 图形/中文 demo 验证。

## 需求决策（2026-08-07 修订）
- LVGL **9.5.0**（`lvgl/lvgl: "~9.5"`）；`esp_lvgl_port` **^2.8**（实际解析 2.8.0~1）
- **版本沿革**：原拟对齐同事模拟器 9.2.2，但 esp_lvgl_port 2.8 在 **9.2.x 编译不兼容**（缺 `LV_COLOR_FORMAT_RGB565_SWAPPED`，lvgl 9.1/9.5 有）→ 改用 9.5.0（官方适配最新版，零维护）。代价：放弃 9.1 esp32s3 SIMD 加速；**需同步同事升级模拟器到 9.5.x**。
- 中文字库：**24px 全量 GB2312（6763 字）**，`lv_font_conv` 生成（同事模拟端版本未知，不依赖）
- 范围：**仅 LVGL 显示基础**（接入 + 字库 + demo）；按键、主界面 UI 留后续
- 缓冲：**全帧双缓冲 PSRAM（300KB×2）+ SRAM DMA 中转（panel 内 8行×4 环形；`trans_size` 对 SPI 面板无效）**
- 代码结构：`main/` 按模块分子目录（保持 `#include "app_xxx.h"` 原名不变）

## 1.A 代码结构重组（已完成 2026-08-07）
- 目录：`main/common/`（app_log.h、mem_map.h）`main/events/` `main/debug/` `main/lcd/` `main/console/` + 空 `main/ui/`（待 1.5）；`app_main.c`/`Kconfig.projbuild`/`idf_component.yml` 留根
- `main/CMakeLists.txt`：`SRCS`→`SRC_DIRS "." "events" "debug" "lcd" "console"`，`INCLUDE_DIRS` 含全部子目录（含 common/ui），REQUIRES 不变
- 验证：build 零错误（app 0x43790B / 分区 93% free，与阶段 0 一致）；烧录回归通过（banner/4 事件/console/LCD 裸测全正常）；**用户目检屏幕正确（2026-08-07）**

### 坑（1.A）
1. **`SRC_DIRS` 指向无 `.c` 的目录会警告**：`common/`（纯头文件）、`ui/`（空）列入 `SRC_DIRS` 报 "No source files found"（非致命 CMake Warning）→ 只列含 `.c` 的目录，头文件路径靠 `INCLUDE_DIRS`。
2. **`git mv` 对未跟踪文件会失败**：`app_lcd.c/h` 原未跟踪（`??`），`git mv` 报错 → 用 `Move-Item` 普通移动即可（git 后续 `add` 时识别）。
3. **`idf.py monitor` 需 `IDF_PYTHON_ENV_PATH`**：直接用 venv python 调 `idf.py monitor` 会去找 `~/.espressif/python_env/...` 报错 → 需设置 `$env:IDF_PYTHON_ENV_PATH="C:\Espressif\tools\python\v5.5.3\venv"`（build/flash 亦然；本会话用持久终端首次设好 env 即可）。
4. **sync 终端跑 `idf.py` 输出易被截断**：ninja 进度含 `\r`，大输出会写文件且停在中间；确认最终结果可再跑一次增量 build（`2>&1 | Out-String` 可拿到完整尾部）。

## 1.3 组件依赖（已完成 2026-08-07）
- `main/idf_component.yml`：`lvgl/lvgl: "~9.5"` + `espressif/esp_lvgl_port: "^2.8"`（组件管理器 public 依赖自动注入，无需改 `main/CMakeLists.txt` REQUIRES）
- `managed_components/` 被 `.gitignore` 忽略（`idf.py reconfigure` 可重下）；`dependencies.lock` 提交锁定 lvgl **9.5.0** + esp_lvgl_port **2.8.0~1**
- 验证：reconfigure 下载成功 + build 冒烟零错误
### 坑（1.3）
- **9.2.x 与 esp_lvgl_port 2.8 不兼容**：`LV_COLOR_FORMAT_RGB565_SWAPPED` 在 9.1/9.5 存在、**9.2.x 移除** → 编译报 undeclared（详见版本沿革）
- **semver**：`^9.1` 会解析到 9.5.0（caret 允许 <10.0）；`~9.5` 锁定 9.5.x

## 1.4 LVGL Kconfig（已完成 2026-08-07）
- `sdkconfig.defaults` 加：`CONFIG_LV_COLOR_DEPTH_16`(RGB565) / `CONFIG_LV_USE_CLIB_MALLOC`(LVGL 走 ESP-IDF heap 含 PSRAM) / `CONFIG_LV_USE_OBSERVER`+`SYSMON`+`PERF_MONITOR`(perf monitor) / 关 `LV_BUILD_EXAMPLES`+`LV_BUILD_DEMOS`
- 验证：删旧 sdkconfig 重建零错误；`build/config/sdkconfig.h` 确认生效
### 坑（1.4）
- 改 defaults 不自动生效 → **删根 `sdkconfig` 再 build**
- LVGL9 用 `LV_CONF_SKIP` 机制直接读 sdkconfig.h 的 `CONFIG_LV_*`，无独立 `lv_conf_kconfig.h`

## 1.5 LVGL 初始化（已完成 2026-08-07）
- 新建 `main/ui/app_lvgl.c/h` + `app_lcd` 暴露 `app_lcd_get_io()/app_lcd_get_panel()`；`main/CMakeLists.txt` SRC_DIRS 加 `"ui"`
- 配置：`lvgl_port_init`(任务栈 8192) + `lvgl_port_add_disp`(buffer 全帧 480*320、double_buffer、`buff_spiram=true`、`swap_bytes=false`、`trans_size=0`)
- 验证：monitor `LVGL init OK (disp=..., RGB565, double PSRAM buf 300KB)`；app 0x43790→0x8c1e0(574KB) LVGL 链接进固件，86% free
### 坑（1.5）
1. **`trans_size` 对 SPI 面板无效**：esp_lvgl_port_disp.c 未使用该字段（仅 RGB/DSI 路径）→ PSRAM 中转由 panel 内 8行×4 环形 SRAM 缓冲承担，填 0 即可
2. **`lvgl_port_add_disp` assert `io_handle` 非空**（用于 flush 完成回调 on_color_trans_done→lv_disp_flush_ready）
3. **flush 多次 ready 安全**：draw_bitmap 分带多事务触发多次 `lv_disp_flush_ready`，但结尾 NOP polling 排空全部 DMA，flush_cb 返回前传输完成（LVGL 多次 ready 幂等）
4. monitor 报 `No module esp_idf_monitor` = python 被 PATH 重置指向系统解释器 → 显式设 `IDF_PYTHON_ENV_PATH` + PATH 注入 venv Scripts 再跑

## 1.6 渲染冒烟（已完成 2026-08-07）
- `app_lvgl_smoke_test()`：深灰背景 + 三基色矩形(红/绿/蓝 140×120) + 红→蓝横向渐变(450×100) + ASCII label；`app_main.c` **移除裸测**（只跑 LVGL）
- 验证：目检横屏三色块/文字正确、无花屏；perf monitor 屏显 **25FPS / 1%CPU / 5ms**
### 坑（1.6 横屏方向，重要）
- **esp_lvgl_port 覆盖 MADCTL**：`lvgl_port_disp_rotation_update` 在 ROTATION_0 按 `rotation` 配置调 `esp_lcd_panel_swap_xy/mirror` 重写 MADCTL → rotation 全 false 清掉横屏 `0xE8` 的 MV/MX/MY 变竖屏
- **修复**：`rotation={swap_xy=true, mirror_x=true, mirror_y=true}` 如实反映硬件横屏基线（esp_lvgl_port 标准用法；将来 touch 坐标也按此基线校正）
- **性能结论**：不牺牲性能——`sw_rotate=false` 无软件旋转，rotation 不进渲染路径；rotation_update 仅初始化写一次 MADCTL；**改底层初始化 MADCTL 不能替代**（esp_lvgl_port 会无条件覆盖；横屏必须 MV 置位，与 swap_xy=false 寄存器层面矛盾）
- **参考旧工程** `d:\GitLab\EMG\code\lv_port\lv_port_disp.c`（同款 ST7365/STM32）：手写 flush 不经 esp_lvgl_port，无此覆盖问题；横屏思路一致

## 1.7 字库生成（已完成 2026-08-07）
- 产物：`main/ui/font/lv_font_simhei_24px.c`（**8.7MB 文本**；字形数据 ~996KB（0x 字节字面量计数），bpp=4 抗锯齿）。工具脚本进 git：`tools/gen_gb2312_chars.py`（生成 6763 字清单）、`tools/gen_lvgl_font.js`（生成字库）、`tools/verify_lvgl_font.py`（完整性验证）
- 生成命令（等价，node 脚本已封装）：`lv_font_conv --font C:\Windows\Fonts\simhei.ttf --size 24 --bpp 4 --format lvgl -r 0x20-0x7F --symbols <6763字> --lv-include lvgl.h -o main/ui/font/lv_font_simhei_24px.c`
- **验证（权威）**：解析 .c 的 cmap 重建字符集 == GB2312 全集 + ASCII 0x20-0x7E，**6858 glyph 无缺字无错字 PASS**；业务字（盆底肌电极治疗评估...）全命中
### 坑（1.7，重要）
1. **npx.cmd 中文传参编码风险**：`--symbols` 经 npx.cmd（批处理）走 ANSI 代码页转换，中文可能损坏 → **权威方案**：node 脚本直接以 UTF-16 数组参数调 `lv_font_conv.js`（`tools/gen_lvgl_font.js`，路径 `_npx\b62fd1a864044392\node_modules\lv_font_conv\lv_font_conv.js`）。本机实测 npx 方式结果与 UTF-16 方式体积一致（8.7MB），应无损坏，但统一走 node 脚本最稳
2. **`--range` 不支持读文件且 6763 单码点超 Windows 32K 命令行限制** → 用 `--symbols <6763字字符串>`（≈20KB UTF-8）传字符，ASCII 用 `-r 0x20-0x7F`
3. **GB2312 区位表映射**：遍历 GB2312 双字节编码 `0xA1A1..0xF7FE` decode('gb2312') 筛 `\u4e00-\u9fff` 得 6763 字（勿直接 U+4E00-9FA5 全量 20902 字）
4. **cmap 解析语义（LVGL9）**：SPARSE 的 unicode_list 存**相对 range_start 的偏移**（非相邻差）；FORMAT0_FULL 用 glyph_id_ofs_list[i]==0（非首位）标记缺字；`-r 0x20-0x7F` 实际收录 0x20-0x7E（0x7F DEL 控制字符无字形）
5. **体积**：bpp=4 全量字形数据 ~996KB → 编译进固件 .rodata 约 1MB；factory 4MB 分区当前 86% free（≈3.44MB），1.8 编译后确认余量（预估仍有 ~2.4MB）

## 1.8 字库接入（已完成 2026-08-07，烧录目检通过）
- `main/CMakeLists.txt` SRC_DIRS 加 `"ui/font"`（字库 .c 编译进固件）；`app_lvgl.c` smoke test 加中文 label（`extern lv_font_t lv_font_simhei_24px;` + `lv_obj_set_style_text_font`）：盆底肌/治疗评估/处方/强度/频率/开始/停止/电量/模式/波形/分析
- build 零错误：app 0x1477e0（**1.28MB**），factory 4MB **68% free（≈2.72MB）**（1.6 时 574KB/86% free≈3.44MB，字库 +~700KB）
### 坑（1.8，重要）
1. **SRC_DIRS 不递归子目录**（ESP-IDF）：`SRC_DIRS "ui"` 只收 `ui/` 直接下的 .c，`ui/font/*.c` 不编入固件（libmain.a 无该 obj）→ 须显式加 `"ui/font"`。验证方法：grep `build/build.ninja` 里 libmain.a 源列表
2. **LVGL bitmap_index 20 位位域 ~1MB 硬限制**：bpp=4 全量 6763 字位图 ~1.24MB → 编译 `-Woverflow`（bitmap_index=1237971 截断为 189395，字形索引错乱会花屏/错字）→ **降 bpp=2**（位图 ~698KB，max bitmap_index=698316 安全）。若日后要 4bpp 高质量，须拆多个字库 + `--lv-fallback` 链（可选优化）
3. 编译 5.2MB 字库 .c 较慢（分钟级），sync 终端捕获易截断 → **async 跑 build** 拿完整输出
4. **LV_USE_FONT_COMPRESSED 未启用 → 压缩字库不渲染（空白，重要）**：lv_font_conv 默认输出 RLE 压缩位图（`bitmap_format=1` = LV_FONT_FMT_TXT_COMPRESSED），LVGL 解码在 `#if LV_USE_FONT_COMPRESSED` 内；未启用时 `lv_font_get_bitmap_fmt_txt` 走 else 只打警告不画位图 → **中文 label 空白（屏幕黑，ASCII 正常）**。修复：`sdkconfig.defaults` 加 `CONFIG_LV_USE_FONT_COMPRESSED=y`（改 defaults 需删 sdkconfig 重建）。排查依据：sdkconfig 里 `# CONFIG_LV_USE_FONT_COMPRESSED is not set`。**另注**：`CONFIG_LV_FONT_FMT_TXT_LARGE` 未设——日后若要 bpp=4 全量，可启用 LARGE（bitmap_index 扩 32 位）解除 ~1MB 限制

## 1.10 性能测量（已完成 2026-08-07）
- demo 屏（lv_timer 200ms 动态刷新 + 环形进度动画）运行时：
  - perf monitor 屏显：**22FPS / CPU 20~30%**（对比 1.6 静态冒烟 25FPS/1%CPU——动态持续重绘占 CPU 正常）
  - console `mem`：Free heap **7829KB**（内部 271KB）；PSRAM 已分配 622KB（≈双缓冲 600KB + demo 对象）
  - console `psram`：8MB 总 / **free 7581KB** / largest 7552KB
  - app 分区：68% free（2.72MB；字库占 FLASH ~700KB）
- 结论：内存/PSRAM 非常充足；SPI 全帧双缓冲下 FPS 受 33.5ms/帧上限（~30fps）限制，22FPS 为动态刷新实测值

## 1.11 收尾（已完成 2026-08-07）—— 阶段 1 完成
- build/flash/monitor 回归通过（demo 屏烧录目检已在 1.9 完成）；开发计划阶段 1 全勾选；0E 补 1.7~1.11；记忆同步；本地 commit（回公司 push）
- **阶段 1 里程碑达成**：LVGL 9.5 渲染链路 + 24px 全量 GB2312 字库（bpp=2）+ demo 屏 + 性能测量（22FPS / 内存充足）全部通过

## 阶段 2 参考（后续）
- 6 键按键 + `INPUT_EVENT` → LVGL keypad indev；阶段 A Modbus RTU 主站；阶段 B 主界面 UI（照 `盆底肌/中文UI/0707`）
- 注意：UI 中文标点用 ASCII（字库无全角标点）；若需全角标点，可把 GB2312 区 1-9 符号一并加入字库（`gen_gb2312_chars.py` 扩展）

## 待确认（跨会话）
1. ~~**node.js 是否已装**~~ **已解决（2026-08-07）**：`C:\Program Files\nodejs\node.exe` v22.20.0；`lv_font_conv` 可用（npx 缓存）
2. ~~esp_lvgl_port 与 LVGL 版本兼容~~ **已解决**：9.2.x 不兼容（缺 RGB565_SWAPPED）→ 已改用 9.5.0
