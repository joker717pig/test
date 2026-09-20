# 0H — LVGL FS 驱动 + basic 移植 + Frame 顶栏 + Menu 主菜单 + 按键导航

> 阶段 B1 | 完成 2026-08-11 | 编译通过，待烧录目检

## 目标

把同事的 LVGL PC 端 UI 框架移植到 ESP32，实现：
1. LVGL FS 驱动（letter='S'→SPIFFS `/spiffs`）
2. `basic.h/c` 控件工厂（所有路径适配 SPIFFS）
3. Frame 顶栏（480×30）+ 页面容器（480×290）
4. Menu 主菜单（4 按钮：评估/治疗/产后修复/设置）
5. 按键导航（Modbus INPUT_EVENT → LVGL keypad indev + group）

---

## 一、文件变更清单

### 新建（~11 个文件）

| 文件 | 用途 | 来源 |
|---|---|---|
| `main/ui/basic.h` | 全局常量/结构体/样式/函数声明 | `EDA_EMG_LVGL_PC\basic.h` |
| `main/ui/basic.c` | 控件工厂 + 样式初始化 + 页面管理 | `EDA_EMG_LVGL_PC\basic.c` |
| `main/ui/Frame.h` | Frame 顶栏头 | `EDA_EMG_LVGL_PC\Frame\Frame.h` |
| `main/ui/Frame.c` | Frame 顶栏实现 | `EDA_EMG_LVGL_PC\Frame\Frame.c` |
| `main/ui/Frame_enent.h` | 时间刷新回调头 | `EDA_EMG_LVGL_PC\Frame\Frame_enent.h` |
| `main/ui/Frame_enent.c` | 时间刷新回调实现 | `EDA_EMG_LVGL_PC\Frame\Frame_enent.c` |
| `main/ui/menu_ui.h` | Menu 头 | `EDA_EMG_LVGL_PC\MenuPage\menu_ui.h` |
| `main/ui/menu_ui.c` | Menu 实现（4 按钮） | `EDA_EMG_LVGL_PC\MenuPage\menu_ui.c` |
| `main/ui/menu_ui_event_cb.h` | Menu 事件回调头 | `EDA_EMG_LVGL_PC\MenuPage\menu_ui_event_cb.h` |
| `main/ui/menu_ui_event_cb.c` | Menu 事件回调（仅导航部分） | `EDA_EMG_LVGL_PC\MenuPage\menu_ui_event_cb.c` |
| `main/ui/app_keypad.c/h` | LVGL keypad indev + group | 新写 |

### 修改（3 个文件）

| 文件 | 变更 |
|---|---|
| `sdkconfig.defaults` | 加 `CONFIG_LV_USE_FS_STDIO=y` + `CONFIG_LV_FS_STDIO_LETTER=83` + `CONFIG_LV_FS_STDIO_PATH="/spiffs"` + `CONFIG_LV_FS_STDIO_CACHE_SIZE=4096` |
| `main/CMakeLists.txt` | 无需改（SRC_DIRS 已有 `"ui"`） |
| `main/app_main.c` | 替换 demo UI 为 `Frame_ui()` + `Menu_ui()` + `app_keypad_init()` |

---

## 二、核心适配点

### 2.1 LVGL FS 驱动：用内置 STDIO，不手写

**决策**：启用 `CONFIG_LV_USE_FS_STDIO=y` + Kconfig 选项（LETTER=83/PATH="/spiffs"），LVGL 内置 `lv_fs_stdio.c` 自动注册驱动，**无需手写** `app_lvgl_fs.c`。

原因：
- ESP-IDF VFS 已透明转发 `fopen("/spiffs/...")` 到 SPIFFS
- LVGL 内置 `lv_fs_stdio.c` 完整实现了 open/close/read/write/seek/tell 回调
- `lv_binfont_create("S:/font/xxx.bin")` → LVGL 剥掉 `S:` → `fopen("/spiffs/font/xxx.bin")` → VFS→SPIFFS

**坑**：最初手写了 `app_lvgl_fs.c` 注册空回调的 driver，但 `LV_USE_FS_STDIO` 启用后 LVGL 内置驱动也尝试注册 letter，因 `LV_FS_STDIO_LETTER` 未设触发 `#error "Invalid drive letter"`。解决：删手写文件，给 `CONFIG_LV_FS_STDIO_LETTER=83`（ASCII 'S'）。

### 2.2 路径映射（Windows → SPIFFS）

| 原始路径 | SPIFFS 路径 | 说明 |
|---|---|---|
| `C:/Users/zqt/.../font/lv_font_honorans_medium_14.bin` | `S:/font/lv_font_14.bin` | 文件名超长→改名（0G 坑1） |
| `C:/Users/zqt/.../font/lv_font_Btn.bin` | `S:/font/lv_font_Btn.bin` | 文件名不变 |
| `C:/Users/zqt/.../font/lv_font_Title.bin` | `S:/font/lv_font_Title.bin` | |
| `C:/Users/zqt/.../font/lv_font_Text.bin` | `S:/font/lv_font_Text.bin` | |
| `C:/Users/zqt/.../font/lv_font_Unit.bin` | `S:/font/lv_font_Unit.bin` | |
| `C:/Users/zqt/.../font/lv_font_30.bin` | `S:/font/lv_font_30.bin` | |
| `C:/Users/zqt/.../png/menu/log.bin` | `S:/icon/log.bin` | 同事代码分目录，SPIFFS 全平铺在 icon/ |
| `C:/Users/zqt/.../png/menu/battery.bin` | `S:/icon/battery.bin` | |
| `C:/Users/zqt/.../png/menu/green.bin` | `S:/icon/green.bin` | |
| `C:/Users/zqt/.../png/menu/red.bin` | `S:/icon/red.bin` | |
| `C:/Users/zqt/.../png/menu/blue.bin` | `S:/icon/blue.bin` | |
| `C:/Users/zqt/.../png/menu/yellow.bin` | `S:/icon/yellow.bin` | |
| `C:/Users/zqt/.../png/menu/PDPG.bin` | `S:/icon/PDPG.bin` | |
| `C:/Users/zqt/.../png/menu/PDZL.bin` | `S:/icon/PDZL.bin` | |
| `C:/Users/zqt/.../png/menu/SZ.bin` | `S:/icon/SZ.bin` | |
| `C:/Users/zqt/.../png/menu/Women.bin` | `S:/icon/Women.bin` | |
| `C:/Users/zqt/.../png/menu/seleck.bin` | `S:/icon/seleck.bin` | |

### 2.3 LVGL 9.x API 差异

| 问题 | 解决 |
|---|---|
| `Set_Chinese_Font(obj, font)` 参数类型 `lv_obj_t*` → `const lv_font_t*` | 修正 basic.h 中 3 个函数签名（Creat_Label/Creat_Label2/Creat_TextLabel） |
| `lv_font_montserrat_26` 不存在 | 改为 `lv_font_montserrat_14`（LVGL 9.x 内置仅此一款） |
| `#include "lvgl/lvgl.h"` | 全部改为 `#include "lvgl.h"`（ESP-IDF 组件规范） |
| `static enum { ... } PAGE_NAME;` | 改为 `typedef enum { ... } PAGE_NAME_t;`（避免头文件 static 复制） |
| `localtime_s(&tm, &now)` | 改为 `localtime_r(&now, &tm)`（POSIX，ESP32 newlib 可用） |

### 2.4 按键映射

来自 Modbus 按键帧 bit 位号（详见 0F 笔记）：
```
bit0 = END   → LV_KEY_ENTER
bit1 = UP    → LV_KEY_UP
bit2 = RIGHT → LV_KEY_RIGHT
bit3 = LEFT  → LV_KEY_LEFT
bit4 = DOWN  → LV_KEY_DOWN
bit5 = ESC   → LV_KEY_ESC
bit6 = PWR   → LV_KEY_HOME（暂不使用）
```

架构：`INPUT_EVENT handler → FreeRTOS queue → keypad_read_cb → LV_KEY_*`

---

## 三、编译结果

| 阶段 | 固件大小 | vs 增量 | 分区余量 |
|---|---|---|---|
| 阶段 A（Modbus，基线） | 0x152040（~1.32MB） | - | 67% |
| B1.0（basic，字库从 FLASH 移到 SPIFFS） | 0x9f370（~638KB） | **-700KB** | 84% |
| B1.1+1.2（+Frame+Menu） | 0xa31c0（~654KB） | +17KB | 84% |
| B1.3（+keypad） | 0xa36e0（~656KB） | +2KB | 84% |

**关键收益**：字库从编译进固件（~700KB）改为 SPIFFS 运行时加载，固件缩小 50%。

---

## 四、排除范围（留后续阶段）

- 评估/治疗/设置/产后修复子页面（B2~B5）→ 当前 stub 空函数
- `lang.h/c` 多语言模块 → 当前只用中文，`lang_get_str` 仅内部 stub
- 治疗页/滑动条事件回调（B3+）
- `lv_font_chinese.bin` 加载（同事代码未使用，SPIFFS 中保留但不加载）
- 现有 LVGL 初始化、ST7365 驱动、Modbus、SPIFFS 挂载 → 均未修改

---

## 五、待验证（烧录目检）

1. 顶栏 480×30 深色背景 + logo + 电池 + 时间 HH:MM 每秒刷新
2. 页面容器 480×290 灰色背景位于 y=30
3. 4 个菜单按钮（绿/红/蓝/黄彩图背景 + 评估/治疗/设置/产后修复中文）正确渲染
4. 物理按键（经 Modbus→INPUT_EVENT→keypad indev）：焦点移动 + 选中高亮 + 确认跳转
5. console `mem/psram/stack` 水位正常

---

## 六、烧录目检 + 调试修复记录（2026-08-11）

> 全部通过烧录目检后追加。修复了 7 个问题，均为阶段 B1 实测暴露。

### 坑 6.1：LVGL perf monitor 导致 99% CPU / 11FPS

**现象**：主菜单静止画面，perf monitor 显示 11FPS / 99% CPU / 620ms。

**根因**：perf monitor 每帧刷新 FPS/CPU 文本 → 区域失效 → LVGL 强制重绘 → 触发 SPIFFS 图片/字体重新解码（文件路径源每次重绘都走文件 I/O）→ I/O 跑满。

**验证**：`CONFIG_LV_USE_PERF_MONITOR=n` 后，console `top`（`vTaskGetRunTimeStats`，需 `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y` + `CONFIG_FREERTOS_RUN_TIME_STATS_USING_ESP_TIMER=y`）显示 taskLVGL 稳定后 ≈0.6% CPU，系统空闲。**修复：默认关 perf monitor**；诊断 CPU 用 console `top` 命令（本会话新增）。

### 坑 6.2：SPIFFS 图片渲染白屏 30 秒+

**现象**：开机白屏 ~35s 才显示图片（日志 UI 对象 4.5s 就创建完）。

**根因**：`lv_image_set_src("S:/icon/xxx.bin")` 只存路径，LVGL **渲染时**逐块 seek+fread 读 SPIFFS（SPIFFS 随机小读极慢且不稳定：4KB 文件 320ms / 44KB 文件 30ms）。14 张图累计 30s+。

**修复（阶段 B1 核心优化）**：`basic.c` 新增 `Creat_ImageFromFile()`——整文件一次 `fread` 进 RAM（大块读快 10 倍+），构造 `lv_image_dsc_t` 内存源（header 在前 + 像素跟随），渲染零文件 I/O。**白屏 35s → 6s**。同路径缓存复用（`s_img_cache[48]`）。

**坑**：`fopen("S:/...")` 不认识 LVGL 驱动字母 `S:` → 需转换 `"S:/icon/x"` → `"/spiffs/icon/x"`（`strncmp(path,"S:",2)==0` 时拼 `/spiffs%s`, path+2）。

### 坑 6.3：LVGL9 keypad 方向键不导航

**现象**：按键到了 indev（日志 `INPUT bit=X -> LV_KEY=Y`），但焦点不动，只有第一个按钮显示选中。

**根因**：LVGL 9.x keypad 只对 **`LV_KEY_NEXT`/`LV_KEY_PREV`** 自动 `lv_group_focus_next/prev`；`LV_KEY_UP/DOWN/LEFT/RIGHT` 只是发给当前焦点对象的 `LV_EVENT_KEY`（button 不处理则无动作）。

**修复**：菜单按钮回调加 `LV_EVENT_KEY` 分支，按 2×2 布局手动 `lv_group_focus_obj()` 实现 2D 导航（评估↔治疗↔设置↔产后）。

**附坑**：`keypad_event_handler` 跑在 esp_event 任务上下文（非 ISR），原先 `xQueueSendFromISR` 是错误用法 → 改 `xQueueSend`。

### 坑 6.4：CHECKABLE 导致图标反色

**现象**：按键后图标「选中」并反色。

**根因**：按钮带 `LV_OBJ_FLAG_CHECKABLE`，按键触发 CHECKED 态 → LVGL 反色显示。而我们的选中高亮走 seleckImg（FOCUSED 事件），CHECKABLE 冗余。

**修复**：`Create_ImaButton` 去掉 CHECKABLE；`menu_ui.c` 移除 4 处手动加 CHECKABLE。

### 坑 6.5：确定后空白页卡死

**现象**：主菜单按确定进入子页面 → 空白（子页面是 stub）。

**修复**：`menu_ui.c` 新增 `Show_StubPage()`——显示「XX 功能开发中」+ 绿色「返回菜单」按钮（加入 keypad group 自动聚焦），确定/ESC 均可 `Goto_MenuPage()` 返回。子页面留 B2~B5。

**注意**：stub 用编译字库 `simhei_24px`（extern 引用）→ 固件 +700KB（1.36MB / 66% free）。子页面真实现后可考虑换 SPIFFS 字库减体积。

### 坑 6.6：时间冒号缺失/乱码（问题 2）

**现象**：时间 `03T15`（冒号变 T）。

**根因**：时间标签用 `Chinise_Font_Unit`（SPIFFS 二进制字库 `lv_font_Unit.bin`），该字库 ASCII 冒号字形缺失/错位。全角 `：` 也依赖该字库是否有 U+FF1A，不可靠。

**修复**：时间标签改用内置 **`lv_font_montserrat_14`**（含 ASCII 冒号，14px 适配 30px 顶栏），格式 `"%02d:%02d"` ASCII。先试 24px simhei（能显示但太大），最终定 14px montserrat。

### 坑 6.7：电池/logo/时间对齐

- 电池用绝对坐标 `set_pos` 与时间 `LV_ALIGN_RIGHT_MID` 不对齐 → 统一右对齐（电池 `LV_ALIGN_RIGHT_MID,-3,0`）
- logo/时间感觉偏低 → 上移（logo y=-5，时间 y=-2）

### 坑 6.8（重要）：切页时 task_wdt 死锁（group refocus 触发 FOCUSED 操作悬垂图）

**现象**（2026-08-11，菜单切到"盆底治疗"确认进入后，按键全部没反应）：
- monitor 见 `E task_wdt: Task watchdog got triggered ... CPU 0: taskLVGL`，无 panic，屏幕冻结。
- 回退栈：`Menu_button_event_cb(CLICKED)` → `Menu_Page_Load` → `Page_Clean` → `lv_obj_clean`
  → 删除**焦点按钮** → `lv_group_remove_obj` → `lv_group_refocus` → 向剩余按钮发 `LV_EVENT_FOCUSED`
  → `Menu_button_event_cb(FOCUSED)` → `Menu_Set_UnSel()` 操作**正在被删除/已释放的 seleck 图**（悬垂指针）
  → `lv_obj_invalidate` 遍历损坏对象树 → 死循环 → task_wdt。

**根因**：LVGL 删除 group 内焦点对象时自动 refocus 并发 `LV_EVENT_FOCUSED`；此时页面正在被 `Page_Clean`
整体销毁，`Menu_Widget.*seleckImg` 等指针已悬垂，FOCUSED 回调去 `lv_obj_add_flag` 它们 → use-after-free 死循环。

**修复**（`main/ui_page/MenuPage/menu_ui_event_cb.c`，均标 `[我们]`）：
1. **切页前先移出 group**：新增 `Menu_LeaveGroup()`，在 `LV_EVENT_CLICKED` 分支 `Menu_Page_Load()` 之前
   把 4 个菜单按钮 `lv_group_remove_obj()`（对象仍有效时移除，删除阶段不再触发 FOCUSED）。
   ⚠️ LVGL9.5 签名是 `lv_group_remove_obj(lv_obj_t *obj)`（**只有对象参数**，自动找所属 group），不是 `(group, obj)`。
2. `Menu_Set_UnSel()` 与 FOCUSED 分支加 `lv_obj_is_valid()` 防护（防悬垂，双保险）。

**教训（B2~B5 通用）**：任何带 keypad group 的页面，**切页销毁前必须先把自己组内的对象移出 group**，
否则删除焦点对象时 group refocus 会向正在销毁的对象发 FOCUSED → 悬垂崩溃。B2 评估页按钮/定时器同理。

### 新增 console 命令

- `top`：`vTaskGetRunTimeStats` 打印各任务 CPU%（配 `CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS` + `RUN_TIME_STATS_USING_ESP_TIMER`）

### 最终固件

- app 0x15e8d0（1.36MB）/ 4MB factory **66% free**（含编译字库 simhei_24px ~700KB）
- 开机到 UI 完整显示 ~6s（字体+图片一次性读 RAM）

### 代码变更清单（本会话）

- `sdkconfig.defaults`：`CONFIG_LV_USE_PERF_MONITOR=n`；`CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS=y` + `RUN_TIME_STATS_USING_ESP_TIMER=y`
- `main/console/app_console.c`：新增 `top` 命令
- `main/ui/basic.c`：新增 `Creat_ImageFromFile()` + `app_img_cache_t` 缓存；`Create_ImaButton` 去 CHECKABLE；`Set_Chinese_Font` 签名 `const lv_font_t*`
- `main/ui/basic.h`：`Creat_ImageFromFile` 声明
- `main/ui/Frame.c`：电池右对齐；时间用 montserrat_14 + ASCII 冒号
- `main/ui/Frame_enent.c`：`%02d:%02d` ASCII
- `main/ui/menu_ui.c`：`Show_StubPage()` 占位+返回；4 处去 CHECKABLE
- `main/ui/menu_ui_event_cb.c`：`LV_EVENT_KEY` 2D 导航
- `main/ui/app_keypad.c`：`xQueueSendFromISR`→`xQueueSend`
