# 0N 参数存储（NVS）—— 阶段 E

> 日期：2026-08-12
> 状态：代码完成 + 编译通过（app 0x1701e0 / 64% free）+ **烧录验证通过（2026-08-12，往返持久化实测）**
> 关联：开发计划「阶段 E：评估/生物反馈、亮度音量、充电/电量」的参数存储部分；
> 混合方案：设置/治疗参数 → NVS（本次）；历史记录/疗程数据 → SPIFFS（后续阶段 D/E）。

## 一、需求与决策（已与用户对齐）

| 决策点 | 结论 |
|---|---|
| 存储介质 | **混合方案**：本次先做 NVS 部分（设置+治疗参数）；历史记录/疗程数据留 SPIFFS |
| 参数范围 | **设置类**（音量/亮度/语言）+ **治疗类**（3 功能 × 脉宽/频率/强度1/强度2/阶段） |
| 保存时机 | **离开页面/点确认时存**（非变更即存，减少 NVS 磨损） |
| 排除项 | 日期/时间（无 RTC 仅 UI 显示）、治疗剩余次数（结构体字段未用）、上次功能/处方、评估历史 |

## 二、调研结论（改前现状）

- 全工程无 `nvs_flash_init` 调用；`nvs` 16KB 分区在分区表预留但从未使用（`mem_map.h:82` 标注"校准/配置"）。
- SPIFFS `/spiffs`（storage ~12MB）只存图片资源，无任何写文件封装（`app_spiffs.c` 只有 info/list/cat）。
- 所有参数仅 RAM 全局/结构体：`Set_Widget.Value_Volume/Value_Bright`（int32 0~5）、`g_app_lang`（LANG_EN/ZH）、
  `PosReh_Param_Data1/2/3`（freq=1/2/3, pulse=500, intens=0, stage=1）。
- `Param_Data_t`（`menu_ui.h:151`）含 `lv_obj_t*`/`char*` 指针 → **不能直接持久化原结构**，需 POD。
- 语言 stub `ui_switch_language()` 是空实现（`lang.c:35`），切语言根本没生效，只是 UI 状态。

## 三、设计

### 3.1 数据结构（POD）
```c
/* app_params.h */
typedef struct {
    int32_t intens1;   /* 阶段1强度 0~10 */
    int32_t intens2;   /* 阶段2强度 0~10 */
    int32_t freq;      /* 频率 Hz（默认 1/2/3） */
    int32_t pulse;     /* 脉宽（默认 500） */
    int8_t  stage;     /* 阶段 1/2 */
} treat_param_t;
```
- NVS namespace `"app"`，键：`vol` / `bri` / `lang`（i32）+ `treat0/1/2`（blob，17→20B）。
- 默认值：vol=1, bri=1, lang=1(ZH)；treat[i] = {intens 0/0, freq i+1, pulse 500, stage 1}（与 UI 静态默认一致）。

### 3.2 模块与 API
- 新文件 `main/params/app_params.c/h`（SRC_DIRS/INCLUDE_DIRS 加 `"params"`，REQUIRES 加 `"nvs_flash"`）。
- 内存副本（`s_vol/s_bri/s_lang/s_treat[3]`）+ 显式 save 落盘，set 不立即写。
- API：`app_params_init / get/set_volume|bright|lang / get/set_treat / save_settings / save_treat / save_all / print / reset`。

### 3.3 接入点（保存时机 = 离开页/点确认）
| 位置 | 动作 |
|---|---|
| `app_main.c` | `app_params_init()`（`app_spiffs_init` 后）→ NVS 初始化+加载；`ui_init_timer_cb` 里 `lang_set(app_params_get_lang())` + `PosReh_Param_ApplySaved()` |
| `PelFlo_Set_ui.c` | 音量/亮度页创建后 `lv_slider_set_value(..., app_params_get_volume/bright(), LV_ANIM_OFF)`（触发 VALUE_CHANGED 自动回填 `Set_Widget`+右侧数字）；`Set_BackToList()/Set_BackToMenu()` 保存设置 |
| `lang.c` | `ui_switch_language()` → `lang_set()` + 立即 `app_params_set_lang + save_settings()` |
| `PosReh_Page_ui.c` | `PosReh_Param_ApplySaved()`（首次进入应用已存值，一次性 flag 防覆盖进行中编辑）；`PosReh_Param_SaveAll()`（`PosReh_BackToMenu` 前调） |
| `PosReh_Page_ui_event.c` | `PosReh_P4SetBtn_Event_cb` CLICKED 里保存当前功能治疗参数（`PosReh_Get_CurFunc` 索引） |
| `menu_ui.c` | `ParSet_Widget` 滑条初值 = 已存强度（`data->Value_Stage==2 ? Intens2 : Intens1`） |
| `app_console.c` | `param` 命令组：`print / save / reset / set`（set 仅调试：改内存副本，需再 save 落盘；`vol|bri|lang <v>`、`treat <idx> <i1> <i2> <freq> <pulse> <stage>`） |

## 四、实测坑（编译阶段）

1. **`app_params.h` 注释里的 `*/` 提前终止注释（关键）**：注释写 `Param_Data_t 含 lv_obj_t*/char* 指针` →
   `*/` 直接结束注释，`char*` 变成代码 → `-Werror` 一堆解析错误（`expected '=' before '*'`、`treat_param_t` 无类型）。
   **教训：C 注释里不要出现 `*/` 序列（`/` 紧跟 `*`），POD 结构体说明改用"LVGL 对象与字符串指针"等文字。**
2. **`PosReh_Page_ui.c` 用 `ESP_LOGI(TAG,...)` 但无 `TAG` 定义**：新增 `static const char *TAG = "PosReh";`（IntelliSense 不报，真编译才暴露）。
3. 无关预存警告：`PelFlo_Set_ui.c` 的 `ChildPage_jump` unused-function（`-Wno-error=unused-function`，非错误，历史遗留）。

## 四·五、UI 目检发现 bug（2026-08-12 修复）

### ① 音量/亮度滑条改动不保存（返回再进、断电均丢）——用户目检反馈
- **根因**：`SetPage_Slider_Event_cb` 的 VALUE_CHANGED 只写 `Set_Widget.Value_Volume/Value_Bright`，
  **从未同步到 `app_params` 内存副本 `s_vol/s_bri`**。而 `Set_BackToList()/Set_BackToMenu()` 调 `app_params_save_settings()`
  保存的是没更新的 `s_vol/s_bri`（还是旧默认值 1）→ 返回再进/断电全丢。
- **修复**：VALUE_CHANGED 分支补 `app_params_set_volume(value)` / `app_params_set_bright(value)`（同步内存副本），
  退出时 `save_settings` 就能落盘。**通用教训：滑条/滚轮这类控件值写入的是 UI 结构体字段，必须同步到存储层内存副本，
  否则"离开页保存"存的是旧值。**

### ② 滑条位置记住但右侧数值/强度标签不刷新（**LVGL 9.5 关键坑**）——用户目检反馈
- 现象：加载已存值后滑条位置对，但 `Label_VolValue`/`Label_Intens` 仍显示创建初值（1 或 10）。
- **根因（lv_bar.c 源码确认）**：**LVGL 9.5 的 `lv_slider_set_value(obj, val, LV_ANIM_OFF)` 只移动滑条 + invalidate，
  不发 `LV_EVENT_VALUE_CHANGED`**（`lv_bar_set_value_with_anim` ANIM_OFF 分支无 send_event；
  事件只在用户拖拽/按键/旋钮时由 lv_slider 自身发）。→ 程序化 set_value 后依赖 VALUE_CHANGED 刷新全部失效。
- **修复（4 处）**：set_value 后手动同步——
  ① `PelFloSet_Page1/2_widget`：`widget->Value_Volume/Bright = app_params_get_*()` + `lv_label_set_text_fmt`；
  ② `menu_ui.c::ParSet_Widget`：手动更新 `Label_Intens`；
  ③ `PosReh_Page4_Key_cb` 左右调档位：手动更新 `Value_Intens1/2` + `Label_Intens`（原注释"触发 VALUE_CHANGED 自动更新"是错的）。
- **通用教训：LVGL 程序化 `lv_slider_set_value(ANIM_OFF)` 不发事件，需联动刷新时必须手动同步值/标签。**
- 已烧录（app 0x170460 / 64% free）待用户复测。

## 五、验证记录

- [x] `idf.py build` 零错误：app **0x1701e0**（~1.5MB）/ factory 4MB **64% free**；`app_params.c.obj` 已链接（map 符号齐全）。
- [x] **烧录 + 硬件实测（2026-08-12，COM5，含 console 往返测试）**：
  1. 首启：`app_params: init OK (vol=1 bri=1 lang=1)` + `param print` → 默认值（treat×3 freq=1/2/3 pulse=500 intens=0 stage=1）✓
  2. `param set vol 5` / `param set treat 1 3 4 2 500 2` → 内存副本更新 ✓
  3. `param save` → 日志全部 `(OK)`（vol/bri/lang + treat0/1/2）✓
  4. esptool `flash_id` 硬复位重启 → 启动日志 `app_params: init OK (vol=5 bri=1 lang=1)` + `param print` 显示 **vol=5、treat1 i1=3 i2=4 stage=2 持久化成功** ✓
  5. `param reset` → `params reset to defaults` + 全部 `(OK)`；再次复位重启 → `init OK (vol=1 ...)` 干净回默认 ✓
  6. 全程无崩溃/无 Guru；`lang set to 1` + `PosReh: treat params applied from NVS` 启动应用正常 ✓
- [ ] **UI 目检（需物理按键，用户操作）**：
  1. 设置页改音量/亮度滑条 → 返回 → 断电重启 → 滑条显示已存值（`Set_Page_Load` 建页时已从 NVS 取）
  2. 产后修复 Page4 调强度 → 「设置完成」→ 退出 → 重启 → ParSet 滑条显示已存强度
  3. 语言页切中文/英文 → 重启 → `g_app_lang` 保持（log 确认）

## 六、遗留 / 后续

- 亮度值暂未接背光 PWM（`app_lcd_backlight` 只有 GPIO 开关）→ 只存值，接硬件留阶段 E。
- 日期/时间设置未接入（无 RTC）→ 本次不存，留时钟接入时做。
- 语言 `lang_get_str` 仍是 stub（只返回少量中文）→ 多语言真正切换留后续；本次保证 `g_app_lang` 值存取。
- UI 档位(0~10) ↔ 寄存器 mA(0~90) 量纲换算未定义 → 留阶段 C（处方引擎/治疗状态机）定。
- 疗程剩余次数（`Cnt_DiaRecti` 等）、评估历史、上次功能/处方 → 阶段 D/E 混入 SPIFFS 文件。
