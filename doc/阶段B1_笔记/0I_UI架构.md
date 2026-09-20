# 0I — UI 架构重构（ui_port / ui_page 分层 + 'C' 盘路径翻译）

> 阶段 B1 架构演进 | 完成 2026-08-11 | 编译通过 + 烧录目检通过

## 目标

之前 B1（0H）把同事代码改路径后平铺在 `main/ui/`，会导致：
- 同事每次更新代码 → 我们要重改路径，diff 巨大
- 平台层（FS/按键/LVGL 初始化）和同事 UI 层混在一起

本次重构目标：**让同事交付的 `EDA_EMG_LVGL_PC` 代码原样可用、更新时零适配**。

---

## 一、新目录结构

```
main/
├── ui_port/               # 平台层（ESP32 特有，我们维护）
│   ├── app_lvgl.c/h       # LVGL 初始化（esp_lvgl_port 接入 ST7365）
│   ├── app_keypad.c/h     # 按键输入 → LVGL keypad indev
│   └── app_fs.c/h         # 'C' 盘 FS 驱动：同事 Windows 路径 → SPIFFS + RAM 缓存
├── ui_page/               # 同事代码（原装，路径不改）
│   ├── basic.c/h          # 控件工厂
│   ├── Frame.c/h          # 顶栏
│   ├── Frame_enent.c/h    # 时间刷新
│   ├── lang.c/h           # 多语言 stub（lang_get_str 返回 ""）
│   └── MenuPage/          # 主菜单（路径保持 C:/Users/zqt/... 原样）
│       ├── menu_ui.c/h
│       └── menu_ui_event_cb.c/h
└── ui/font/               # 编译字库 lv_font_simhei_24px（占位页/stub 用）
```

**核心约定**：
- `ui_page/` 只放同事代码，路径原样（`C:/Users/zqt/Desktop/PDJ_LVGL/png/...`）
- 同事更新代码 → 整目录替换，**我们零适配**
- 平台适配全部收敛在 `ui_port/`

---

## 二、'C' 盘路径翻译驱动（app_fs.c）

### 原理

同事路径 `C:/Users/zqt/Desktop/PDJ_LVGL/png/<dir>/<file>`：
1. LVGL 解析 src 时剥掉 `C:` → open_cb 收到 `/Users/zqt/.../png/...`
2. `translate_path()` 把 `/png/<dir>/<file>` 翻译成 SPIFFS 实际路径
3. open 时整文件读入 RAM，之后 read/seek/tell 走内存（渲染零文件 I/O）

### 翻译规则（2026-08-11 起：镜像 png/，替代原扁平分类）

| 同事路径 | SPIFFS | 说明 |
|---|---|---|
| `/png/<dir>/<file>` | `/spiffs/png/<dir>/<file>` | **1:1 镜像**（保留目录） |
| `/spiffs/...` | 原样 | 兼容直接 S:/ 路径 |

**为何改镜像**：旧扁平映射（font/image/icon 分类）存在**真实同名冲突**——`png/menu/Women.bin`（菜单产后按钮）
与 `png/PosRehPage/Women.bin`（B4 产后页主图）都压到 `/spiffs/icon/Women.bin` 必错图。镜像结构天然隔离，
同事加任何新目录零适配。

**特殊改名**：`lv_font_honorans_medium_14.bin` → `lv_font_14.bin`（SPIFFS 32 字符限制，见 0G 坑1）。

**资源迁移**（2026-08-11 完成，烧录验证通过）：`tools/reorg_spiffs_png.py` 从 `LVGL_PC/png/` 镜像到
`spiffs_data/png/`（含字体改名+图片头校验）；`tools/check_spiffs_refs.py` 校验代码引用↔SPIFFS 一致性（B2~B5 复用）。
顺带修：`menu_ui.c` 产后按钮 `png/menu/Women.bin`→`png/menu/CHXF.bin`（同事原装即 CHXF）；一处 B1 遗留变量名笔误
`PostRec_Obj`→`PosReh_Obj`（本次编译暴露）。旧 `spiffs_data/image/`（battery2/heart_image2 等）代码无引用，已弃。

### RAM 文件内容缓存（本次新增，修复按键响应慢）

**坑 7.1：按键响应慢（"按键一下，显示半天"）**

- **现象**：主菜单按键后要 2s+ 才响应。
- **根因**：LVGL 渲染 image 时会反复 open（动画帧/重绘每 ~330ms 一次），而旧 open_cb 每次都重新从 SPIFFS 读整个文件（44KB 大图 ×4 按钮背景），SPIFFS 随机读极慢 → 渲染任务被拖垮，按键事件排队。
- **修复**：`app_fs.c` 增加 `s_cache[CFS_CACHE_MAX=32]` 文件内容缓存——首次 open 读入 RAM 常驻，之后 open 直接返回缓存指针，**零 SPIFFS 重复读取**。菜单/字体共 ~20 个文件、~300KB，PSRAM 8MB 足够。
- **附加**：cache 命中日志降为 `ESP_LOGD`（大量日志在 115200 串口刷屏本身也占 CPU）。

**验证**：烧录后日志 `C:load ... (SPIFFS)` 每个文件仅首次出现，之后全部 `(cache)` 命中；按键响应立即。

---

## 三、本次修复的反色回归（坑 7.2）

**坑 7.2：选中反色回归**

- **现象**：重构用同事原装代码后，按键选中按钮图标又反色。
- **根因**：同事原装 `Create_ImaButton`（basic.c）+ `menu_ui.c` 4 处按钮都加了 `LV_OBJ_FLAG_CHECKABLE` → LVGL9 切 CHECKED 态反色（0H 坑 6.4 已修，同事代码重新带回来）。
- **修复**：移除 CHECKABLE（basic.c 的 Create_ImaButton + menu_ui.c 4 处），标注 `[我们]` 注释说明。选中高亮仍走 seleckImg（FOCUSED 事件），不受影响。

**验证**：烧录后按键移动高亮，图标无反色。

---

## 四、main/CMakeLists.txt

```cmake
idf_component_register(SRC_DIRS "." "events" "debug" "lcd" "console" "MODBUS"
                                  "ui_port" "ui_page" "ui_page/MenuPage" "ui/font"
                       INCLUDE_DIRS "." "common" "events" "debug" "lcd" "console"
                                    "ui" "ui/font" "MODBUS"
                                    "ui_port" "ui_page" "ui_page/MenuPage"
                       REQUIRES ...)
```

---

## 五、main/app_main.c 初始化顺序

```
app_lvgl_init()                     # LVGL 初始化
ui_init_timer_cb (LVGL 任务上下文):
    app_fs_init()                   # 先注册 'C' 盘驱动（chinese_font_init 用 C:/ 路径）
    chinese_font_init()             # 加载中文字库（C:/ 路径经 app_fs 翻译）
    style_checked_init()            # 全局样式
    app_keypad_init()               # 按键 indev
    Frame_ui()                      # 顶栏
    Menu_ui(g_Ui.page_container)    # 主菜单
```

---

## 六、清理

- **删除死代码 `app_lvgl_smoke_test()`**：阶段 1.8 的调试自检函数（三基色/渐变/文字），已无调用，从 `app_lvgl.c/h` 删除，连带删除 `app_lvgl.c` 里 `extern lv_font_simhei_24px`（只被死代码用）。
- `lv_font_simhei_24px` 仍在（menu_ui.c 占位页 `Show_StubPage` 的"功能开发中/返回菜单"文字用），等 B2~B5 真页面实现后可移除整个 `ui/font/`。

---

## 七、编译结果

- `idf.py build` 零错误；固件 0x15ea40（约 1.36MB）/ 4MB factory **66% free**
- 编译警告均为同事原装代码未使用变量（Frame.c line_1、Frame_enent.c time_str 等），不影响功能，按原样保留

## 八、验证

1. 烧录后 'C' 盘翻译全部生效：6 字体 + 全部菜单图标 `C:load` 成功，无 `open fail`
2. 按键响应立即（RAM 缓存命中，无 SPIFFS 重复读）
3. 选中无反色（CHECKABLE 移除）
4. UI init complete（Frame + Menu + keypad）
