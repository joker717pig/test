# 0J — UI 代码移植改动点指南（同事更新时对照执行）

> 配套 `0I_UI架构.md` | 更新 2026-08-11 | 目的：同事每次交付新 UI 代码，按本清单逐项核对/打补丁，保证零遗漏

## 一、移植总原则

- **目录映射**：同事 `EDA_EMG_LVGL_PC\<模块>\` → ESP32 `main/ui_page_cn\<模块>\`（路径原样保留 `C:/Users/...`，由 `ui_port/app_fs.c` 'C' 盘驱动翻译）
  - ⚠️ **2026-08-12 架构分层**：中文目录已由 `ui_page` 改名为 **`ui_page_cn`**（英文目录为 `ui_page_en`，公共操作层 `ui/`：basic/Frame/lang/font）。同事更新中文 UI 一律落到 `ui_page_cn/`；以下所有 `ui_page` 均指 `ui_page_cn`。
- **平台层**（`ui_port/`）完全不用动：FS 翻译、按键、LVGL 初始化
- **同事代码只做"必要编译修正 + 平台行为补丁"**，每处改动都标 `[我们]` 注释，方便 diff
- 改完后必须过 **5 项验证**（见文末）

---

## 二、通用改动点（所有 ui_page_cn 文件）

### 2.1 include 头文件修正（每个文件必查）

| 同事原装 | 移植后 | 原因 |
|---|---|---|
| `#include "lvgl/lvgl.h"` | `#include "lvgl.h"` | ESP-IDF 组件规范，`lvgl` 在 include 路径 |
| `#include "./MenuPage/xxx.h"` | `#include "xxx.h"` | ui_page_cn 全目录在 INCLUDE_DIRS，扁平引用 |
| `#include "./Frame/Frame.h"` | `#include "Frame.h"` | 同上 |

**检查方法**：`grep -rn 'lvgl/lvgl\|\.\/' main/ui_page_cn/` 应无残留。

### 2.2 路径保持原样，禁止改动

- 所有 `"C:/Users/zqt/Desktop/PDJ_LVGL/png/..."` **原样保留**
- `app_fs.c::translate_path()` 负责映射：**1:1 镜像** `/png/<dir>/<file>` → `/spiffs/png/<dir>/<file>`（2026-08-11 起，
  替代原 font/image/icon 扁平分类——旧映射有跨目录同名冲突，如 `menu/Women.bin` vs `PosRehPage/Women.bin`）
- 特殊改名：`lv_font_honorans_medium_14.bin` → `lv_font_14.bin`（SPIFFS 32 字符限制）
- **同事新增资源目录**（如 `png/PosReh/`）：直接放 `spiffs_data/png/<dir>/` 即可，app_fs 无需改动（镜像规则通用）

---

## 三、逐文件改动点清单

### 3.1 basic.h
- [ ] include 修正（2.1）
- [ ] `static enum { ... } PAGE_NAME;` → `typedef enum { ... } PAGE_NAME_t;`（头文件 static 枚举会每个编译单元复制一份 → 链接冲突）
- [ ] 函数声明 `font` 参数类型：`lv_obj_t*`/`char*` → `const lv_font_t*`（`Set_Chinese_Font`/`Creat_Label`/`Creat_Label2`/`Creat_TextLabel`）
- [ ] 如有 `#if 1` 包裹 → 去掉（保留 include guard 即可）

### 3.2 basic.c
- [ ] include 修正（2.1）
- [ ] **`lv_font_montserrat_26` → `lv_font_montserrat_14`**（LVGL9.5 无 26，roller 用它；26 会链接失败）
- [ ] `Set_Chinese_Font` 等 `font` 参数加 `const`（与 basic.h 一致）
- [ ] **`Create_ImaButton` 移除 `LV_OBJ_FLAG_CHECKABLE`**（标 `[我们]`，防 LVGL9 CHECKED 反色——坑 6.4/7.2）

### 3.3 Frame.c
- [ ] include 修正（2.1）
- [ ] 时间标签：同事原装 `Creat_Label(top_obj, "09：02", Chinise_Font_Unit)`（全角冒号）——**验证** Unit 字库是否含全角冒号；若乱码/缺字，改 `lv_font_montserrat_14` + ASCII `"09:02"`（坑 6.6）
- [ ] 顶栏对齐微调：电池 `LV_ALIGN_RIGHT_MID,-3,0`、logo y=-5、时间 y=-2（坑 6.7，可选）

### 3.4 Frame_enent.c
- [ ] include 修正（2.1）
- [ ] **`localtime_s(&tm,&now)` → `localtime_r(&now,&tm)`**（Windows 专属 API，ESP32 用 POSIX）

### 3.5 lang.h / lang.c（stub）
- [ ] **只需接口**：`str_id_t`/`app_lang_t` 枚举 + `lang_get_str`/`lang_set`/`ui_switch_language` 声明
- [ ] `lang.c`：`lang_get_str()` 返回 `""`；`g_app_lang = LANG_ZH`（阶段 B1 仅中文，不移植字符串表）
- [ ] 若同事更新新增字符串 → 直接忽略（stub 返回空串，UI 文字用 `Creat_Label` 硬编码）

### 3.6 MenuPage/menu_ui.h
- [ ] include 修正（2.1）
- [ ] **补 `extern Menu_Widget_t Menu_Widget;`**（menu_ui.c 定义，事件回调文件引用；同事原装头文件可能漏 extern）

### 3.7 MenuPage/menu_ui.c
- [ ] include 修正（2.1）+ 新增 `#include "app_keypad.h"`、`#include "esp_log.h"`（`[我们]`）
- [ ] **移除 4 处 `lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE)`**（标 `[我们]`，防反色）
- [ ] **`[我们]` 段：Menu_widget 末尾注册按钮到 keypad group**（`lv_group_add_obj` 4 按钮）
- [ ] **`[我们]` 段：`Show_StubPage(title)`** 占位页（"XX 功能开发中" + 返回按钮，B2~B5 真页面实现后移除）

### 3.8 MenuPage/menu_ui_event_cb.c
- [ ] include 修正（2.1）
- [ ] **裁剪**：只保留 `Menu_button_event_cb`；删除 `Warn_Btn_Event_cb`/`TreIns_Widget_event_cb`/`Treat_Widget_Event_cb`/`Child_Slider_Event_cb`/滚动回调（治疗页专属，B3 移植时再加回）
- [ ] **`[我们]` 段：`Menu_Set_UnSel()`** 隐藏全部 seleckImg
- [ ] **`[我们]` 段：`LV_EVENT_FOCUSED`** 分支（焦点按钮显示对应 seleckImg，其余隐藏）
- [ ] **`[我们]` 段：`LV_EVENT_KEY`** 分支（2D 导航：评估↔治疗↔设置↔产后，`lv_group_focus_obj`）
- [ ] 保留同事原 `LV_EVENT_CLICKED` 分支（`Menu_Page_Load`）+ `lv_obj_remove_state(obj, LV_STATE_CHECKED)`

---

## 四、平台层（ui_port，不用动）

| 文件 | 职责 | 是否受同事更新影响 |
|---|---|---|
| `app_fs.c/h` | 'C' 盘路径翻译 + RAM 文件缓存 | 否（除非新增子目录，见下） |
| `app_keypad.c/h` | 按键→LVGL keypad indev | 否 |
| `app_lvgl.c/h` | LVGL 初始化 | 否 |

**例外**：若同事新增资源子目录（非 font/image/menu），在 `app_fs.c::translate_path()` 加映射规则。

---

## 五、工程集成（每次新增模块必做）

### 5.1 main/CMakeLists.txt
```cmake
idf_component_register(SRC_DIRS "." "events" "debug" "lcd" "console" "MODBUS"
                                  "ui_port" "ui_page_cn" "ui_page_cn/MenuPage" "ui/font"
                       INCLUDE_DIRS "." "common" "events" "debug" "lcd" "console"
                                    "ui" "ui/font" "MODBUS"
                                    "ui_port" "ui_page_cn" "ui_page_cn/MenuPage"
                       ...)
```
- **新增页面目录**（如 B2 的 `ui_page_cn/PelFlo_AssPage`）→ SRC_DIRS + INCLUDE_DIRS **各加一行**
- ⚠️ ESP-IDF `SRC_DIRS` **不递归子目录**，每个子目录都要显式列出

### 5.2 main/app_main.c 初始化顺序
```
app_fs_init()          # 必须最先（chinese_font_init 用 C:/ 路径）
chinese_font_init()    # 加载字库
style_checked_init()   # 全局样式
app_keypad_init()      # 按键
Frame_ui()             # 顶栏
Menu_ui(...)           # 主菜单
# 后续 B2~B5: 各页面注册到 g_page_list
```

---

## 六、验证清单（每次移植后必过）

0. [ ] **对照 `doc/开发计划.md` 的「通用移植守则」逐条检查**（尤其退出/返回路径的 group 移出 + `lv_obj_is_valid` 防悬垂 + 定时器清理——坑 6.8 / B2-4 同族）
1. [ ] `grep -rn 'lvgl/lvgl\|\.\/' main/ui_page_cn/` 无残留 → include 已修
2. [ ] `grep -rn 'CHECKABLE' main/ui_page_cn/` 只出现 `[我们]` 注释，无 `lv_obj_add_flag(..., CHECKABLE)` 实际调用
3. [ ] `idf.py build` 零错误（警告可留，同事未使用变量不处理）
4. [ ] 烧录后：所有 `C:load ... (SPIFFS)` 首次成功、无 `open fail`
5. [ ] 目检：菜单 4 按钮 + 顶栏 + 按键导航（焦点移动/选中/进入/返回）+ 无选中反色 + 按键响应即时

---

## 七、常见坑速查（对应 0H/0I）

| 现象 | 原因 | 处理 |
|---|---|---|
| 编译报 `lv_font_montserrat_26` 未定义 | LVGL9.5 无 26 | 改 14 |
| 编译报 `PAGE_NAME` 重复定义 | 头文件 `static enum` | 改 `typedef enum` |
| 链接报函数 `font` 类型不匹配 | `const lv_font_t*` 缺失 | 补 const |
| 按钮选中反色 | CHECKABLE | 移除（basic.c + menu_ui.c） |
| 按键不导航（方向键） | LVGL9 只认 NEXT/PREV | menu_ui_event_cb.c 加 LV_EVENT_KEY 2D 导航 |
| 按键响应慢 2s+ | app_fs 无缓存反复读 SPIFFS | 用 ui_port 缓存版（无需动 ui_page_cn） |
| 图片白屏/慢 | FS 未缓存 | 同上 |
| 时间冒号乱码 | Unit 字库缺冒号 | 换 montserrat_14 + ASCII |
