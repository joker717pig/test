# 0P 英文主菜单（两套独立 UI 架构）—— 阶段 E2

> 日期：2026-08-12
> 状态：✅ **E2 已定档**（架构分层重构 + 英文主菜单/设置/产后/评估 全接通，编译+烧录+目检通过）
> 关联：开发计划「后续」；两套独立 UI 架构。
> 更新：本文档前 3~7 节为 E2 早期（仅英文主菜单，app 0x170970）；「八、E2 后续扩展」记录
>       同日完成的架构分层重构 + 英文设置/产后/评估页（最终固件 0x17a720 / 63% free）。

## 一、需求与决策（已与用户对齐）

| 决策点 | 结论 |
|---|---|
| 架构 | 中文/英文是**两套独立 UI**（布局+文字各自独立）；按 `g_app_lang` 进入不同页面树；中英文跳转/返回逻辑一致 |
| 第一批范围 | **只做英文主菜单**（验证两套架构跑通）；英文菜单点击**不跳转**（其余英文页面未做） |
| 切语言行为 | **立即生效并重建回主菜单**（自动按新语言进入对应菜单） |
| 设计稿 | 用户提供英文菜单布局代码 `Menu_widget_EN`（4 容器 200×110：Assessment/Therapy/Rehabilitation/Setting，复用 green/red/yellow/blue、PDPG/PDZL/CHXF/SZ、seleck 图标） |

## 二、关键调研结论（改前现状）

- 页面框架 = `Page_Clean()`（`basic.c:572`，清空 page_container）+ 重建；`Goto_MenuPage()`（`basic.c:583`）= `Page_Clean + Menu_ui`。
  `g_page_list[]/.jump()` 是死代码（PC 工程移植壳），实际导航全走"销毁重建"，**英文方案不迁就它**。
- 语言机制是"假开关"：`g_app_lang`（默认 LANG_ZH）+ NVS 已通（阶段 E1），但 `ui_switch_language` **不重绘界面**。
- 入口：`app_main.c::ui_init_timer_cb` 里 `Frame_ui()`（建顶栏 + `g_Ui.page_container`）→ `Menu_ui()`。
- 字体 6 个含 ASCII/数字（`lv_font_Btn` 等），英文标签**零新字体**；菜单图标图片全部已有，**零新资源**。路径 `C:/...` 由 app_fs 翻译到 /spiffs。
- 同事无英文 UI 可抄（lang.c EN 列全空）；中文字符串硬编码 ~110 处。

## 三、架构（两套独立 UI，最小侵入）

- **分叉点集中在"回主菜单"**：`basic.c` 新增 `Ui_EnterMenu()` = `Page_Clean()` + 按 `g_app_lang` 调 `Menu_ui()`（中文）或 `Menu_En_ui()`（英文）；`Goto_MenuPage()` 内部改调 `Ui_EnterMenu()`。
- 中文 `ui_page/`（同事原装）**零改动**；英文 `ui_page_en/` 新目录独立实现（避免同事更新覆盖）。
- 所有"返回菜单"路径（各页 BackToMenu / Goto_MenuPage）自动按语言进对应菜单 → 跳转/返回逻辑天然一致。
- 切语言：`lang.c::ui_switch_language()` 在 `lang_set + NVS 保存` 后调 `Ui_EnterMenu()` 立即重建回菜单。

## 四、命名规则（用户确认）

- `.c/.h` 统一 `*_En` 唯一命名（英文菜单 = `menu_En_ui.c/h`），**避免与中文同名头 include 歧义**（两个目录都在 INCLUDE_DIRS，同名头会让其它文件 include 拿错）。
- 内部符号统一 `Menu_En_*` 前缀（`Menu_En_ui` / `Menu_En_Widget_t` / `Menu_En_button_event_cb` / `Menu_En_LeaveGroup`），防链接冲突。
- 后续英文页面（评估/设置/产后/治疗）按同规则：`.c/.h` 加 `_En` + 符号带 `En`，全部放 `ui_page_en/`。

## 五、实现清单

| 文件 | 动作 |
|---|---|
| `main/ui_page_en/menu_En_ui.c/.h` | **新建**：`Menu_En_Widget_t`（4 按钮+4 选中图）+ `Menu_En_ui(parent)`（用户布局）+ `Menu_En_LeaveGroup()` + `Menu_En_button_event_cb`（FOCUSED 高亮 / KEY 2D 导航 / CLICKED 预留不跳转） |
| `main/ui_page/basic.c` | `Ui_EnterMenu()`（Page_Clean + 按语言分发）+ `Goto_MenuPage()` 改调它；include `menu_En_ui.h` |
| `main/ui_page/basic.h` | 声明 `Ui_EnterMenu()` |
| `main/ui_page/lang.c` | `ui_switch_language()` 末尾加 `Ui_EnterMenu()`（切语言立即重建）；include `basic.h` |
| `main/app_main.c` | `ui_init_timer_cb` 里 `Menu_ui(...)` → `Ui_EnterMenu()`（**保留 `Frame_ui()`**，它建 page_container） |
| `main/CMakeLists.txt` | SRC_DIRS/INCLUDE_DIRS 加 `"ui_page_en"` |

## 六、坑 / 注意

1. **app_main 误删 `Frame_ui()`**：`Frame_ui()` 不只建顶栏，还创建 `g_Ui.page_container`（所有页面的父容器）。分叉入口 `Ui_EnterMenu` 里的 `Page_Clean`/`Menu_*` 都依赖它。**启动顺序必须是 `Frame_ui()` 先、`Ui_EnterMenu()` 后**。
2. **终端环境变量不跨命令持久**（受限 shell）：每次 build/flash/monitor 需在同命令用 .NET `[Environment]::SetEnvironmentVariable` 设 `IDF_PATH`/`IDF_TOOLS_PATH`/`IDF_PYTHON_ENV_PATH` + `PATH`（cmake/ninja/xtensa/venv Scripts），否则 idf.py 报 ~/.espressif python_env 不存在 / ninja 找不到。
3. 英文菜单 CLICKED 暂不跳转：英文子页面未做，事件回调只留 `ESP_LOGI`；预留 `Menu_En_PageID_t` 分发，等英文页面补齐后接 `Menu_En_Page_Load`（与中文 `Menu_Page_Load` 同构）。
4. 英文菜单下无法再进设置页切回中文（点击不跳转）→ 验证用 console `lang` 命令（app_console.c 已有）或重启 + NVS。

## 七、验证记录

- [x] `idf.py build` 零错误：app **0x170970** / 64% free；`menu_En_ui.c.obj` 已链接（map：`basic.c (Menu_En_ui)` 引用确认分叉调用连上）。
- [x] 已烧录（app 100% 校验通过 + storage 写完硬复位）。
- [ ] 待目检：
  1. 中文开机 → 中文菜单（回归，布局不变）
  2. 中文设置 → 语言页 → 选 English → **立即回英文主菜单**（4 项英文标签 + 图标正常、无崩溃）
  3. 英文菜单按键 2D 导航/高亮正常；点击 4 项不跳转
  4. 断电重启（NVS lang=EN）→ 直接进英文菜单；`console lang zh` + 重启回中文
  5. 评估/设置/产后页返回都能回对应语言菜单

## 八、E2 后续扩展（2026-08-12 同日完成，英文全链路）

### 8.1 架构分层重构（用户决策）
- 目标：`main/ui/`（公共操作：basic/Frame/lang/font）+ `main/ui_page_cn/`（中文纯页面）+ `main/ui_page_en/`（英文纯页面）。
- 决策（用户确认）：① 共享控件（TreIns/ParSet/Treat/Warn）**每语言各一份**（cn 留 ui_page_cn，en 放 ui_page_en），不走 lang 参数化；② 中文目录**改名 ui_page_cn**（与 ui_page_en 对称）；③ 先搬公共层再继续做英文页面。
- 执行：`ui_page` → `ui_page_cn`（Move-Item）；`basic/Frame/Frame_enent/lang` 4 组 .c/h → `ui/`；`CMakeLists.txt` SRC_DIRS/INCLUDE_DIRS 更新（ui_page→ui_page_cn 各子目录、加 `"ui"`）。**0J 指南路径同步改为 ui_page_cn（见 0J 头部更新）**。

### 8.2 英文设置页（set_En_ui.c/h + set_En_ui_event.c/h）
- 复制中文 SetPage 直译：7 子页（列表/音量/亮度/语言/日期/时间/完成）；音量/亮度滑条 ↔ `app_params` 同步；语言页中文/English 按钮 → `ui_switch_language`。
- 英文主菜单 Setting 接入 `Set_En_ui()`。

### 8.3 英文产后页（ui_page_en/PosReh_Page/ + shared_En_ui）
- `shared_En_ui.c/h`：英文共享控件 `Warn_En_Window`(Yes/No)/`Treat_En_Widget`(Pulse/Frequency/Intensity/Pause/Start/Back)/`ParSet_En_Widget`(Pulse Width/Stage Setting/Frequency)；**TreIns_Widget 无硬编码中文直接复用中文版**；事件回调复用中文语言无关版（menu_ui_event_cb.h）。
- `PosReh_En_ui.c/h` + `PosReh_En_ui_event.c/h`：独立英文数据实例（Diastasis Recti / Uterine Involution / Lactation）+ 5 子页 + 倒计时 timer（"Done/Great job!/Remaining Time"）+ 按键导航；与中文共享 NVS 参数存储 + 页面管理结构（lv_my_ui_pagePosReh_t 等 extern）。
- 英文主菜单 Postnatal Rehabilitation 接入 `PosReh_En_Page_Load(PosRehPage1)`。

### 8.4 英文评估页（ui_page_en/PelFlo_AssPage/Ass_En_*）
- 复用中文全局状态（`Ass_Widget` / chart 系列 / ecg 采样数组 / 滚动偏移），仅 UI 文本翻译（Assessment/History/Start Assessment/Max EMG/Instant EMG/Remaining/Next Preview/Abdomen/Report/Stage/Pre-rest/Fast Contraction/Sustained Contraction/Post-rest/Total Score + 表格 Parameter/Reference/Result/Score + Health Records）。
- 事件回调：语言无关的中文回调直接复用（BackToMenu/Ass_Page_Esc/scroll/Draw_EcgTable/Mydraw）；跳转相关 4 个用英文版（`Ass_En_Page1_Btn_FocEvent_cb`/`Ass_En_report_timer_cb`/`Ass_En_Aemg_add_data`/`Ass_En_Record_Menu_event_cb`）。
- ⚠️ 英文自有 `s_en_page_esc_obj`（中文 `s_page_esc_obj` 是 static 不可见，英文各自记录并清理，防 group 悬垂）。
- 英文主菜单 Pelvic Floor Assessment 接入 `Ass_En_Page_Load(AssPage1)`。

### 8.5 🔥 坑 E2-1：英文产后参数设置页（Page4）崩溃（"10 Sessions 进不去"）
- **现象**：英文产后页点「10 Sessions/单次治疗」→ 参数设置页进不去（页面卡死/崩溃）。
- **根因**：`ParSet_Widget`/`ParSet_En_Widget` 滑条初值同步块（`lv_slider_set_value` + `lv_label_set_text_fmt(data->Label_Intens,...)`）原来在**强度标签 Label4 创建赋值之前**执行 → 首次进参数页时 `data->Label_Intens` 为 NULL → `lv_label_set_text_fmt(NULL,...)` 内部解引用崩溃（LVGL `lv_label_set_text_vfmt` 对 NULL obj 不防御）。**中文同隐患（中文产后参数页此前未被实际走通，未暴露）**。
- **修复**：同步块下移到 `data->Label_Intens = Label4;` **之后**（shared_En_ui.c + menu_ui.c 两处）。
- **教训**：LVGL9.5 程序化 `lv_slider_set_value(ANIM_OFF)` 不发 VALUE_CHANGED，需手动同步关联 label —— 但**必须先创建并赋值 label 指针，再对它 set_text**（顺序颠倒 = NULL/悬垂指针崩溃）。

### 8.6 E2 定档验证
- [x] 架构分层后 `idf.py build` 通过（0x172950 仅重链接）。
- [x] 英文设置页编译 0x172950 烧录，切语言显示正常。
- [x] 英文产后页编译 0x175700 烧录；启动正常。
- [x] 英文评估页编译 0x17a720 烧录；**用户目检：英文评估页进入正常**。
- [x] 参数页崩溃修复（0x17a720）烧录；**用户目检：产后 10 Sessions 可进入参数设置页，验证通过**。
- 最终固件 0x17a720 / 63% free。

### 8.7 遗留 / 后续
- 英文主菜单 **Pelvic Floor Therapy（盆底治疗）** 尚未接通（英文治疗页未做，点击仅 log）。
- 中文产后参数页（ParSet_Widget）建议顺带目检回归（修复后应可进入）。
- `doc/开发计划.md` 阶段 E2 记录、repo 记忆已同步。
