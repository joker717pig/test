# 0Y OTA 界面整合联调 — 分步实施计划（新会话接续用）

> 创建 2026-09-04。本文档把"OTA 界面逻辑整合"拆成 6 个可独立完成/联调的步骤，
> **新会话可只认领其中某一步**，先读本文件对应小节 + 相关代码即可接续。
> 前置背景：OTA 后端（`main/ota/app_ota.c` 独立任务 + 命令队列、WiFi `app_wifi.c`、
> STM32 Boot 主动拉取 `app_modbus.c`）均已写好并在 console 验证过；本阶段只做"接线 + UI 逻辑"。

## 0. 总体架构（务必先理解）

- **OTA 重活全在独立 `ota` 任务**（16KB 栈，`app_ota_task_start()` 已在 app_main 启动）。
  UI/console 通过 `app_ota_request(cmd)` 往队列投命令，任务串行执行。
- **UI 不跨任务操作 LVGL**：ota 任务只写一个全局状态结构 `g_ota_ui`（在 app_ota.c），
  UI 侧用 `lv_timer`（~150ms）在 **LVGL 任务上下文**轮询 `app_ota_ui_get_state()` 刷新控件。
  工程未使用 `lvgl_port_lock`，轮询方案最稳，不要在 ota 任务里直接调 lv_*。
- WiFi：`app_wifi.c` 状态机 IDLE/CONNECTING/CONNECTED/FAILED，`app_wifi_connect()` 非阻塞。

## 1. 用户已定决策（2026-09-04，勿再改）

1. **版本号统一两位（主.次）**：云端 `version.json` 三目标版本都用两位（如 `"1.0"`）；
   下位机 STM32/ESP32 本就是 16 位 `0xMMmm`（高字节主、低字节次）。比对按 (主,次) 逐段。
2. **进 OTA 页立即读 STM32 版本**（FC03 读 `0x0302`），**不等 WiFi**；WiFi 连接同时并行发起。
   界面三行进度条前**增加版本号字符串**（当前 vX.Y → 最新 vX.Y / ✓最新）。
3. **图片版本 = storage 槽内版本文件**：`spiffs_data/image_ver.txt`（内容如 `1.0`）随 SPIFFS
   镜像打包；读当前挂载槽 `/spiffs/image_ver.txt` 得当前图片版本。图片升级整刷非激活槽后
   重启切槽，新槽自带新版本文件，无需 NVS。
4. **完成按钮 = 重启**：三项全跑完后底部按钮（原"返回"）显示"升级完成（点击重启）"，点击 `esp_restart()`。
5. **失败处理（2026-09-04 补充）**：升级中底部按钮**始终"升级中…"禁用**；某项失败只把
   **该项进度条标红**并继续跑后续项，**直到三项全部结束**才按汇总结果设置按钮
   （全成功→"升级完成(点击重启)"；有失败→"返回"+失败项红条，可重试）。

## 2. 按钮 / 进度条状态机

| 阶段 | 底部按钮(原返回) | "是"按钮 | 进度条 |
|---|---|---|---|
| 进页面/检查中 | "返回"(可点) | 置灰"检查中…" | 0% |
| 联网失败 | "返回" | "重试" | — |
| 已是最新 | "返回" | 置灰"已是最新" | — |
| 有更新待确认 | "返回" | **"开始升级"**可点 | 0% |
| 升级中 | **"升级中…"禁用** | "升级中…"禁用 | STM32→图片→ESP32 依次；失败项**标红** |
| 全部完成(全成功) | **"升级完成(点击重启)"**可点→esp_restart | 隐藏/置灰 | 三条 100% |
| 全部结束(有失败) | "返回"(可点) | "重试" | 失败项红条 |

升级顺序：**STM32**（下载→写参数→0x020C→Boot 拉块→DONE→STM32 自重启）
→ **图片**（下载整刷非激活槽 + NVS 切槽标记）
→ **ESP32**（下载写双区 + set_boot_partition，不立即重启）→ 停在完成态等客户点重启。

## 3. 分步任务清单（每步可独立接续）

### 步骤 1 — 版本号两位 + 比较函数 + 打包脚本 ✅ 已完成(2026-09-04)
- [x] `tools/build_ota_package.py`：`--esp32-ver/--image-ver/--stm32-ver` 默认值改两位（`"1.0"`）；
      help 文本同步；`build_version_json` 无需改（透传字符串）。
- [x] `main/ota/app_ota.c/.h`：新增
      - `int app_ota_ver_compare(const char *a, const char *b)`（解析 "主.次"，返回 -1/0/1；容错第三位 patch 忽略）。
      - `void app_ota_ver_bcd_to_str(uint16_t bcd, char *out, size_t len)`（0x0100→"1.0"）。
- [x] 修正 `app_ota.h` 中 `APP_FW_VERSION` 注释（与 `idf_component.yml` 版本口径对齐说明）。
- [ ] console `ota` 命令可加 `vercmp` 自测（可选，未做）。
- 验证：编译通过。

### 步骤 2 — 进页面连 WiFi + 联网状态 + 拉清单 + 版本号显示 ✅ 已完成(2026-09-04)
- [x] `app_ota.c/.h`：新增全局 UI 状态 `ota_ui_state_t`（phase、wifi_state、ip、三目标
      local_ver/cloud_ver/ver_known/has_update/progress/fail、all_ok）+ `app_ota_ui_get_state()`
      （返回 const 指针）+ `app_ota_ui_reset()`。
- [x] 新增命令 `OTA_CMD_BEGIN_CHECK`：读本地三版本（ESP32 宏 / STM32 FC03 读 0x0302 /
      图片读 `/spiffs/image_ver.txt`）→ `app_wifi_init+connect` → `app_wifi_wait_connected(15s)`
      → `app_ota_fetch_manifest`（缓存到 `s_cached_manifest`，供步骤5 UPGRADE_ALL 复用）
      → 填 cloud_ver + 比对 has_update 到全局状态；失败分 WIFI_FAIL / MANIFEST_FAIL 两态。
- [x] `app_wifi.c/.h`：补 `app_wifi_wait_connected(timeout_ms)`（ota 任务阻塞轮询 state；
      FAILED/IDLE 时自动 `app_wifi_connect()` 重连，直到 CONNECTED 或超时）。
- [x] UI（`PelFlo_Set_ui.c/.h` + `PelFlo_Set_ui_event_cb.c`）：
      - `OTA_Widget_t` 加：`Label_Net`（联网状态）、`Label_Ver_STM32/Bin/ESP32`（三行版本号）。
      - 进 `Page_OTA`：`PelFloSet_Page7_widget` 末尾调 `OTA_Page_Enter(widget)`（=reset+投
        BEGIN_CHECK+建 150ms lv_timer）。WiFi init/connect 在 ota 任务的 BEGIN_CHECK 里做，UI 不直接调。
      - 假 `OTA_Timer_cb` 改为真状态轮询：刷联网 Label（联网中蓝/成功绿/失败红）、三条进度条+版本号
        （检查中 `vX.Y` → 有更新 `vX.Y→vA.B` / 无更新 `vX.Y ✓`）、主提示、两按钮文案/使能。
      - 离开页面（返回/ESC）走 `OTA_Page_Leave()`：停 lv_timer + 移出 group + `app_wifi_disconnect()` + `Set_BackToList()`。
      - 布局重排：账号/密码(8/40)、联网状态(250,40)、主提示(50,78)+Sure按钮(330,74,96×28)、
        三目标行 y=120/154/188（名称50 + 条110(200×20) + 版本号318）、Back 按钮底部 160×30。
- 验证：编译通过（`eda_emg_esp32.bin` 18% free）。**目检待上板**：进页面"联网中…"→"联网成功"；
      三行 当前→云端 版本；无更新"已是最新"置灰、有更新"开始升级"可点。
      ⚠️ 注意：点"开始升级"投的是 `OTA_CMD_UPGRADE_ALL`，**该命令步骤 5 才实现**，
      目前 ota_task 走 default 只打 "unknown ota cmd" 日志（不崩溃），属预期分步。

### 步骤 3 — 图片版本文件机制 + 图片升级真实进度 ✅ 已完成(2026-09-04)
- [x] 新增 `spiffs_data/image_ver.txt`（内容 `1.0\n`）。SPIFFS 镜像由顶层 CMakeLists
      `spiffs_create_partition_image(storage_a/b ${CMAKE_SOURCE_DIR}/spiffs_data ...)` 整目录打包，
      根目录文件落到 `/spiffs/image_ver.txt`，storage_a/storage_b 双镜像都含。
- [x] `app_ota.c`：`static bool read_image_version(char* out, len)`（BEGIN_CHECK 用）读
      `/spiffs/image_ver.txt`，fgets 去 `\r\n`；无文件返回 false（版本显示 `?.?`、ver_known=false）。
- [x] 图片升级进度回调：`image_http_event` 已有 `cb(IMAGE, read*100/total)`；UPGRADE_ALL 传入
      `ota_ui_progress_cb` 写 `g_ota_ui.progress[IMAGE]`。
- 流程备注（发布时）：改图片版本 → 改 `spiffs_data/image_ver.txt` → `idf build` 重打 storage 镜像
      → `build_ota_package.py --image-ver X.Y` 打包（镜像内版本文件与 version.json 的 image.version 要一致）。

### 步骤 4 — STM32 升级真实进度（含 modbus 拉块）✅ 代码完成(2026-09-04)，真机联调待做
- [x] `app_modbus.c/.h`：新增 `static volatile uint32_t s_ota_served`，REQ 下发 DATA 后
      `s_ota_served = max(s_ota_served, off+blk_len)`；serve_start/stop 复位 0；
      新增 `uint32_t app_modbus_ota_progress(void)`（serve 未激活返回 0）。
- [x] `app_ota.c::app_ota_upgrade_stm32` 两段进度：HTTP 下载段 cb 报 **0~40%**（`read*40/total`）；
      serve 等待循环里每 50ms 按 `app_modbus_ota_progress()/size` 报 **40~99%**（`40+served*60/size`，
      未收 DONE 不报满）；收到 DONE 成功才报 **100%**。
- [ ] **真机联调（待上板）**：前提 SWD 已烧 Boot + 新布局 APP（见 0X 笔记）。跑 STM32 升级，
      看进度条 0→40（下载）→40~100（拉块）平滑、DONE 后 STM32 自重启。

### 步骤 5 — ESP32 升级 + 顺序升级 + 按钮状态机 + 失败标红 + 完成重启 ✅ 代码完成(2026-09-04)
- [x] `app_ota.c`：新增命令 `OTA_CMD_UPGRADE_ALL`：
      - 门控：**仅 phase==READY 受理**（防快速双击命令排队后在 DONE 态重复升级）；清单无效则转 BEGIN_CHECK。
      - 置 phase=UPGRADING，清 progress/fail；按 **STM32→图片→ESP32** 顺序，每项独立 try，
        **无更新项跳过（进度保持 0，不标红）**；失败只置 `fail[t]=true` 标红、**不中断后续**。
      - 全部跑完汇总 `all_ok = 无任一 fail`，phase=DONE。
      - 三个升级函数都传 `ota_ui_progress_cb`（写 `g_ota_ui.progress[target]`）。
- [x] ESP32 升级：`esp32_http_event` 下载段 cb 报 0~100；成功后 `esp_ota_set_boot_partition`，
      **任务里不重启**，等客户点"升级完成(点击重启)"按钮 → `esp_restart()`。
- [x] UI（`PelFlo_Set_ui_event_cb.c`）：
      - 进度条标红：`ota_bar_refresh()` 每轮按 `fail[i]` 设 INDICATOR 色（红 `FONT_RED_COLOR` / 蓝 `FONT_BLUE_COLOR`）。
      - "是"点击：READY→`app_ota_request(UPGRADE_ALL)`；其余可点态（WIFI_FAIL/MANIFEST_FAIL/DONE有失败）→reset+BEGIN_CHECK（=重试）。
      - 轮询 phase=DONE：全成功→底部按钮"升级完成(点击重启)"（点击 `esp_restart()`）、Sure"已完成"禁用；
        有失败→底部"返回"、Sure"重试"。
      - **升级中(UPGRADING)两按钮都"升级中…"禁用，Back 点击直接 return，绝不提前变"返回"**。
      - Back 点击：UPGRADING 忽略；DONE+all_ok→esp_restart；其余→`OTA_Page_Leave()`（停表+移出group+关WiFi+返回）。

### 步骤 6 — 全链路联调 + 文档收尾
- [x] 整链路：进页面→联网→比对→开始→STM32→图片→ESP32→完成→重启。**2026-09-04 真机全链路通过**
      （含 LCD flush 异步修复，进度条平滑走完不再冻死；FORCE 开关已切 0 走真实版本比较）。
- [ ] 异常路径：联网失败重试、中途断电、某目标 sha/crc 失败标红、STM32 拔线超时。
- [x] 更新本文件勾选状态 + 补“踩坑记录”小节（LCD 冻死坑见上 / `0D` 坑 #9）；同步记忆。
- [x] 正式包打包并上传生产 FTP（云端 esp32=2.1/image=1.1/stm32=1.1 = 本地 2.0/1.0/1.0 **次版本+1**，固件内部号不 bump，测试期可反复升级）。

## 4. 关键文件 / 常量速查

| 用途 | 位置 |
|---|---|
| OTA 任务/命令/下载/双区/图片/STM32 | `main/ota/app_ota.c/.h` |
| WiFi STA | `main/ota/app_wifi.c/.h`（SSID `EDA_EMG_OTA` / PASS `12345678`） |
| STM32 拉块服务 / 版本寄存器 | `main/MODBUS/app_modbus.c/.h`、`app_modbus_reg.h`（`MB_REG_FW_VERSION=0x0302`） |
| OTA 界面 UI | `main/ui_page_cn/SetPage/PelFlo_Set_ui.c/.h`（`PelFloSet_Page7_widget`）、`PelFlo_Set_ui_event_cb.c`（`SetPage_OTABtn_Event_cb`/假`OTA_Timer_cb`） |
| 进度条工厂 | `main/ui/basic.c::Create_Bar(parent,color,range)`（INDICATOR 上色；标红改 LV_PART_INDICATOR bg_color） |
| 打包脚本 | `tools/build_ota_package.py`（version.json 三目标；stm32 用 crc32，其余 sha256） |
| 云端 | **正式（2026-09-07 起）**：`http://health.eda-global.cn/pelvicFloorMuscleUpgradeKit/fota/eda_emg/version.json`（HTTP 明文，**无 FTP，打包后手动上传** eda_emg/ 整夹）。旧测试地址 `http://www.dgbuysmart.com/fota/lzq3496900/eda_emg/`（有 FTP，已弃用）。地址宏见 `app_ota.h::APP_OTA_BASE_URL/MANIFEST_URL` |
| STM32 版本 | 下位机 `FW_VERSION 0x0100`（=v1.0，两位），寄存器 0x0302 |
| ESP32 本地版本 | `app_ota.h::APP_FW_VERSION`（16 位 0xMMmm） |

## 5. 踩坑记录（随做随补）

- **LVGL 9 禁用控件用 state 不是 flag**：`LV_OBJ_FLAG_DISABLED` 在 LVGL9 已不存在（编译报
  undeclared，提示 `LV_OBJ_FLAG_CLICKABLE`）。正确写法：禁用 `lv_obj_add_state(obj, LV_STATE_DISABLED)`，
  使能 `lv_obj_clear_state(obj, LV_STATE_DISABLED)`。
- **`Creat_Label(parent, char* text, font)` 形参是 `char*` 非 `const char*`**：用 `const char*`
  数组传字符串字面量会报 `-Wdiscarded-qualifiers`。本工程 C 语言下字符串字面量可隐式转 `char*`，
  结构体里把 `name` 字段声明成 `char*`（不要 `const char*`）即可。
- **`ota_data` 是独立全局，不被 `Set_Page_Load` 的 `memset(&Set_Widget,...)` 清零**：重进 OTA 页
  旧控件指针/`Timer` 会残留。已在 `PelFloSet_Page7_widget` 开头 `memset(widget,0,sizeof(*widget))`。
- **UI 侧用 LVGL 必须在 LVGL 任务**：ota 任务只写 `g_ota_ui`（短字符串/整型，轮询无需锁）；
  所有 `lv_*` 都在 150ms `lv_timer` 回调里。工程未用 `lvgl_port_lock`，沿用既有轮询模式。
- **WiFi 断连不自动重连**：`app_wifi.c` 事件里 STA_DISCONNECTED 只置 FAILED。`app_wifi_wait_connected()`
  内部在 FAILED/IDLE 时主动 `app_wifi_connect()` 兜底重连（STA_START 也会触发一次）。
- **阻塞式 modbus 读版本必须在 ota 任务**：`app_modbus_read_registers` 带 mutex+重试，
  BEGIN_CHECK 在 ota 任务里读 STM32 0x0302，绝不能放 LVGL 定时器（会卡 UI）。
- **离开 OTA 页要手动移出 group**：`Set_LeaveGroup()` 只清 `Set_Widget`，不含 `ota_data` 的两个按钮；
  `OTA_Page_Leave()` 里单独 `lv_group_remove_obj`（带 `lv_obj_is_valid` 门控防悬垂）。
- **UPGRADE_ALL 必须门控 phase==READY**：ota 任务串行处理队列，若用户快速双击"开始升级"，
  两条 UPGRADE_ALL 排队；第一条跑完 phase=DONE 后第二条若不门控会**重复升级**。已在命令入口判
  `phase != READY → 忽略`。UI 侧按钮在 UPGRADING/DONE 也已禁用，双保险。
- **无更新项进度条保持 0、不标红**：UPGRADE_ALL 只升级 `has_update[t]` 为真的目标，跳过项
  progress 不动（0%）、fail=false（蓝）。只有真正尝试且失败的才红条。
- **STM32 升级成功会自重启**：DONE(status=0) 后 STM32 跳新 APP 自行复位；ESP32 侧继续跑
  图片/ESP32 升级（这俩走 WiFi HTTP，不占 modbus，无冲突）。STM32 进度到 100% 即该项完成。
- **STM32 进度两段映射**：HTTP 下载 0~40%，Boot 拉块 40~99%（收到 DONE 才 100%）。
  拉块进度来自 `app_modbus_ota_progress()`（modbus RX 任务在 REQ 响应里更新 `s_ota_served`），
  ota 任务在 serve 等待循环每 50ms 读一次报给 UI。
- **图片版本文件随镜像走**：`spiffs_data/image_ver.txt` 被打进 storage_a/storage_b 两个 SPIFFS 镜像；
  OTA 整刷非激活槽后切槽，新槽自带新版本文件，**无需 NVS 存图片版本**。打包时 version.json 的
  image.version 必须与镜像内 image_ver.txt 一致，否则比对逻辑错乱。
- **⚠️ OTA 页进度条/版本号"行↔目标"映射易错（2026-09-04 真机踩坑）**：`ota_target_t` 枚举顺序是
  **ESP32=0, IMAGE=1, STM32=2**，而 OTA 屏三行从上到下是 **STM32 / 图片 / ESP32**。轮询刷新时
  控件数组按"行"排 `{Bar_STM32, Bar_Bin, Bar_ESP32}`，若直接用 `st->progress[i]` 取值会**错位**：
  STM32 行显示成 ESP32 进度、ESP32 行显示成 STM32 进度（真机表现：明明后端 STM32 先升完，
  屏上却是最底 ESP32 条先满）。**必须用 `row_target[]={STM32,IMAGE,ESP32}` 把行号翻译成目标枚举**，
  再取 `st->progress[t]`/`fail[t]`/版本。后端升级顺序日志（target 2→1→0）始终正确，问题只在 UI 映射。
- **⚠️ OTA 专用字库在 SPIFFS(storage)，改字库必须"全量烧录"（2026-09-04 真机踩坑）**：
  `lv_font_OTA.bin` 由 `tools/gen_ota_font.js` 生成、放进 `spiffs_data/png/font/`，随
  **storage_a/storage_b 两个 SPIFFS 镜像**打包，**不在 app 分区**。若只点"烧录 app（快速）"，
  app 更新了但 storage 还是旧的 → `lv_binfont_create(lv_font_OTA.bin)` 打开失败返回 NULL，
  `basic.c` 静默回退到同事旧子集 `lv_font_Btn.bin`：常用字（账号/密码/升级/完成/图片/返回）正常，
  OTA 独有字（**重启/生效/点击/→**）仍是豆腐块。**改字库/改 spiffs_data 后必须用"烧录固件(flash 全量)"**
  （含 0x410000 storage_a + 0xa08000 storage_b）。已在 `basic.c` 加 `OTA font loaded OK/FAIL` 日志，
  上电看该日志即可确认字库是否加载成功。另：云端 OTA 包里的 image(storage) 也要重新打包上传，
  否则图片 OTA 切到云端旧槽后字库又会缺。
- **⚠️ HTTP 下载进度条不动、最后直接跳 100（2026-09-04 真机踩坑）**：图片/ESP32 两路是纯 HTTP
  流式下载，进度回调条件为 `if (cb && total)`，而 `total` 原先只在 `HTTP_EVENT_ON_CONNECTED` 里取
  响应头 **`Content-Length`**。生产服务器（`www.dgbuysmart.com`，FTP 映射 HTTP）对大文件**不返回有效
  Content-Length**（chunked/无头）→ `total=0`（或 -1 转超大）→ 回调整段不算进度，进度条停在 0，
  直到收尾 `st->progress[t]=100` 一下跳满。STM32 那路正常是因为它进度主要来自 **modbus 拉块循环**
  （按本地固件大小算 40~100%），不依赖 HTTP 头。**修复**：三个 writer 初始化时用清单已知的
  `info->size` 播种 `w.common.total` 兜底；`ON_CONNECTED` 里只有服务器返回 `Content-Length>0` 才覆盖。
  教训：**进度分母不要只信 HTTP 头，清单 version.json 里的 size 是可靠来源**。
- **⚠️ OTA 升级中进度条冻死、后端却全部成功（2026-09-04 真机踩坑，根因在 LCD 驱动不在 OTA）**：
  现象：点开始升级后，串口日志显示 STM32/图片/ESP32 三目标**依次全部成功**，但屏上进度条停在
  中途（典型 stm32 100% / 图片 22% / esp32 未开始）再不往前走，像 UI 死机；重启后显示又正常。
  排查：后端 `g_ota_ui.progress[]` 一直在更新（日志可证），UI 150ms 轮询也在跑，问题出在**画面刷不出来**。
  根因：`main/lcd/app_lcd.c` 的 `st7365_draw_bitmap`（LVGL flush 路径）结尾发了一条
  `esp_lcd_panel_io_tx_param(io, LCD_CMD_NOP, NULL, 0)` 做 **polling 排空**，逼 LVGL 任务每帧阻塞等
  整帧 SPI DMA 传完。平时 DMA 快、阻塞短无感；但 **OTA 图片/ESP32 升级要擦写内部 flash，擦除期间
  跨核关 cache/stall CPU**，SPI 完成中断路径被拉长，LVGL 任务堵在排空点 → `lv_disp_flush_ready`
  不再被调用 → 画面永久冻死（进度数值其实在变，只是没 flush 上屏）。
  修复：**删掉 draw_bitmap 结尾的 NOP 排空**，回归 esp_lcd 官方面板纯异步用法——颜色事务走
  “队列(trans_queue_depth=4)+SPI 中断”，最后一带 DMA 结束由中断回调
  `on_color_trans_done → lv_disp_flush_ready` 通知 LVGL（esp_lvgl_port 对 SPI 面板不在 flush_cb 末尾
  主动 ready，全靠此回调）。缓冲复用安全由 4 槽环形 + FIFO 回收保证，无需手动排空。
  （`app_lcd_fill` 是上电清屏专用、不在 LVGL/OTA 路径，保留它的排空无妨。）
  教训：**LCD flush 路径绝不能加“等 DMA 完成”的阻塞/忙等**；DMA 源是内部 RAM `s_swap_buf`，
  flash 擦除关 cache 不影响已排队 DMA，但会卡死在 CPU 上轮询等 DMA 的代码。详见
  `doc/阶段0_笔记/0D_ST7365显示驱动.md` 坑 #2/#9。
- **串口日志里中文乱码（如 `鏍￠獙閫氳繃`）是控制台 GBK/UTF-8 解码问题**，固件内字符串本身正常，无害。
- **⚠️ 测试开关 `OTA_TEST_FORCE_UPGRADE`（`app_ota.c` 顶部）**：设为 **1** 时 `BEGIN_CHECK` 跳过版本号
  比较，只要云端清单某目标 URL 非空就强制 `has_update=true`（日志打 `TEST FORCE`）。UI 按钮门控不变。
  **2026-09-04 全链路真机联调通过后已改回 0**，恢复真实版本比较（日志改为 `[check] .. local=.. cloud=.. update=..`）。
- **✅ 版本号正式流程（2026-09-05 起，替代测试期假版本）**：`tools/build_ota_package.py` 现在**默认自动
  从固件提取真实版本号**，version.json 与设备自报版本严格一致，杜绝手填假版本：
  - ESP32：解析 `main/ota/app_ota.h` 的 `APP_FW_VERSION`（0xMMmm → "主.次"，0x0200→"2.0"）；
  - STM32：解析 `D:\GitLab\EDA_EMG\code\MODBUS\MB_RegMap.h` 的 `FW_VERSION`（0x0100→"1.0"）；
  - 图片：读 `spiffs_data/image_ver.txt`（"1.0"）。
  - 打包命令：`py tools/build_ota_package.py --upload`（版本号全自动；`--esp32-ver/--image-ver/--stm32-ver`
    仅在需临时覆盖时手动指定）。`--upload` 直传生产 FTP `203.170.59.165/eda_emg/`，会覆盖云端，确认无误再跑。
  - **发布新版流程**：bump 对应固件版本号（ESP32 改 `APP_FW_VERSION` 并重新 `idf.py build`；STM32 改
    `FW_VERSION` 并 IAR 重新编译出 hex；图片改 `spiffs_data/image_ver.txt` 并重新生成 storage 镜像），
    再跑打包脚本 → 云端 version.json 自动变高，旧设备据此判"云端 > 本地"升级，升级后版本相等不再提示。
- **测试期“可反复升级”的版本号策略（2026-09-04，已废弃）**：当时固件内部号不 bump（2.0/1.0/1.0），
  只把云端 version.json 次版本 +1（2.1/1.1/1.1）强制判更新以便反复联调。2026-09-05 已改回正式真实版本号
  （云端恢复 2.0/1.0/1.0，与固件一致）。若日后联调还想强制升级，用 `OTA_TEST_FORCE_UPGRADE=1` 或临时
  `--xxx-ver` 覆盖，**不要再把假版本号留在生产云端**。
- **⚠️ 图片/ESP32 进度条定死在 0%、串口也看不到下载百分比，STM32 却正常（2026-09-05 真机）**：
  现象：STM32 那行进度条平滑走、百分比正常；图片、ESP32 两行进度条一直定在 0%（图片偶发跳到
  22% 后停住），且串口日志里**没有任何下载百分比打印**。三目标后端其实都在正常下载/写 flash。
  根因（两条叠加）：
  1. **进度条定死 = LVGL 锁被长持有**。冻死修复引入的 `app_lvgl_begin_flash_op()` 会取 LVGL 递归锁，
     持锁期间 LVGL 任务阻塞、`OTA_Timer_cb`（150ms 轮询重画进度条）不运行。STM32 的 0~40% 下载段虽也
     持锁，但固件仅 52KB、1~2 秒就过，40~100% 的 UART 拉块段在 `end_flash_op()` **之后**跑（锁已释放），
     所以它平滑；而图片是**下载前整片擦 6MB**（`esp_partition_erase_range(part,0,part->size)`）、
     ESP32 是 `esp_ota_begin(OTA_SIZE_UNKNOWN)` **begin 时整片擦 2MB**（见 `esp_ota_ops.c`：
     `OTA_SIZE_UNKNOWN`→begin 内 `erase_range(0, partition->size)`），且**整个下载写 flash 都在持锁区**，
     进度回调只写了 `g_ota_ui.progress[]` 却没人画 → 定死。
  2. **下载无打印**：图片/ESP32 的 HTTP `ON_DATA` 事件里原本**只有出错日志，没有进度 `ESP_LOGI`**
     （STM32 拉块段也没打，但它进度条正常所以没被注意）。
  修复（`main/ota/app_ota.c` + `main/ui_port/app_lvgl.c/.h`）：
  - **擦除摊到下载过程，不再长持锁整片擦**：
    - ESP32：`esp_ota_begin(part, OTA_WITH_SEQUENTIAL_WRITES, &h)`（替代 `OTA_SIZE_UNKNOWN`）。
      该模式 begin **不擦**，`esp_ota_write` 时按扇区增量擦（`esp_ota_ops.c` need_erase 分支），
      `esp_ota_end` 仍正常校验镜像。
    - 图片：删掉下载前 `erase_range(0, part->size)`；在 `image_http_event` 的 ON_DATA 里按
      `part->erase_size`（4KB）**只擦本块覆盖到、尚未擦的扇区**（`image_writer_t` 加 `erased` 偏移）。
  - **持锁擦写期间周期放行 LVGL 刷一帧**：新增 `app_lvgl_pump_flash_ui()`（安全区内调用）：
    `lvgl_port_unlock()` → `vTaskDelay(30ms)`（让同优先级 LVGL 任务取锁跑一轮 `lv_timer_handler`
    重画进度条、发 flush）→ `lvgl_port_lock(0)` 重新取锁 → `app_lcd_drain()` 排空在途 DMA。
    **末尾 drain 保证返回时总线空闲**，调用方随后擦/写 flash（关 cache、屏蔽非 IRAM 中断）时无在飞
    LCD 传输，**不破坏冻死修复**。
  - **统一下载进度助手 `ota_dl_progress()`**：更新 UI 状态 + 每 200ms pump 一次 LVGL + 每 500ms
    `ESP_LOGI` 打 `[esp32/image/stm32] 下载 N% (read/total B)`。三目标下载事件都改走它
    （STM32 下载段仍映射 0~40%）。
  教训：**`begin/end_flash_op` 之间不能做"整片长擦除 + 整段下载"这种长持锁操作**，否则 LVGL 全程不刷新；
  要么把擦除摊薄（增量擦），要么在持锁区周期性 `pump_flash_ui()` 放行一帧（pump 末尾必须 drain 排空 DMA）。
  > 更正：本条上方 2026-09-04 那条"删 draw_bitmap 结尾 NOP 排空即修复冻死"的结论**后被真机复测证伪**
  > （删了仍冻死）。冻死真根因是关 cache 屏蔽非 IRAM 的 LCD 完成中断/回调，真修复是
  > `app_lvgl_begin/end_flash_op()`（取锁停渲染 + drain 排空 DMA），详见 `0D_ST7365显示驱动.md` 坑 #9。
  - **✅ 2026-09-05 真机复测通过**：图片 `下载 0%→99%` 平滑（6.2MB/~107s）、ESP32 `0%→99%` 平滑
    （1.7MB/~29s）、串口百分比齐全、全程无冻死；图片 `sha256 verified` + 切槽成功。增量擦 +
    `pump_flash_ui` 方案有效。
- **⚠️ `[esp32] sha256 mismatch: got <新> exp <旧>` 多半是"清单过期"不是固件坏（2026-09-05 真机）**：
  现象：升级最后 esp32 报 `ESP_ERR_INVALID_CRC`，但 `got` 是刚上传的新 bin sha、`exp` 是上一版 sha。
  根因：设备进 OTA 页 `BEGIN_CHECK` 拉到的是**旧 version.json**（进页面早于云端上传新包，或服务器/
  CDN 缓存了旧 json），而下载 URL 已指向**新 bin** → 清单 sha 与实际 bin 对不上，被 sha256 校验安全拦截。
  判别：看 `got` 是否等于云端最新 version.json 的 sha——是即清单过期（重进 OTA 页/点"重试"重新
  `BEGIN_CHECK` 拉新清单即可）；`got` 本身都不对才是下载/flash 损坏。
  注意：若 esp32 是用**全量烧录**（esptool 写 `ota_0`）上的最新版，则设备本就跑在最新 `ota_0`，
  OTA 这步失败（没 `set_boot`）不影响，重启仍是最新版；image/stm32 成功后**重启**即生效。
- **⚠️ LVGL 焦点死锁：焦点落在被 `LV_STATE_DISABLED` 的按钮上时，所有按键都被丢弃（2026-09-05 真机）**：
  现象：OTA 三目标全部成功、`all_ok=1` 后，"升级完成(点击重启)"键选不中、按确定/上下都没反应。
  根因（`managed_components/lvgl__lvgl/src/indev/lv_indev.c` ~828/856 行）：
  `const bool is_enabled = !lv_obj_has_state(lv_group_get_focused(g), LV_STATE_DISABLED);`
  之后 **ENTER / ESC / 普通方向键(UP/DOWN/LEFT/RIGHT) 全部包在 `else if(is_enabled)` 分支里**——
  焦点对象一旦 disabled，这些键**一个都不投递**；只有 `LV_KEY_NEXT/PREV` 在 is_enabled 判断之前处理。
  本设备 keypad 发的是原始 `LV_KEY_UP=17/LV_KEY_DOWN=18`（不是 NEXT/PREV），所以焦点在禁用键上时
  连"按 DOWN 切到另一个键"都做不到 → 彻底死锁。
  OTA 页触发路径：升级开始时焦点在 `Btn_Sure`，升级中两键都禁用；到 DONE/all_ok 后 `Btn_Sure`
  =“已完成”**继续禁用**、焦点没移走 → 死锁。READY 无更新时 `Btn_Sure`=“已是最新”永久禁用，同理
  （连"返回"都按不到）。
  修复（`PelFlo_Set_ui_event_cb.c`）：① 加 `ota_focus_back_if_enabled()`，在"确定键永久禁用"的两个
  状态（DONE/all_ok、READY/无更新）由 150ms 轮询主动 `lv_group_focus_obj(Btn_Back)` 把焦点切到可用的
  重启/返回键；② KEY 事件里 UP/DOWN 切焦点前加 `!lv_obj_has_state(target, LV_STATE_DISABLED)` 保护，
  避免切到禁用键再次死锁。
  通用教训：**任何"按钮会在运行中被禁用"的 LVGL 页面，禁用焦点按钮时必须同时把焦点移到另一个可用
  控件**，否则实体键/编码器用户会卡死（触摸屏点不到禁用键反而不会触发，所以容易漏测）。
  - **✅ 2026-09-05 真机验证通过（OTA 端到端全通）**：焦点修复版(sha `be324998`，1714128B)打包上传云端后，
    设备重进 OTA 页三目标全部 `sha256 verified`、`all_ok=1`；完成态按 ENTER 成功 `esp_restart()`
    （日志 `rst:0xc (RTC_SW_CPU_RST)`），重启后 bootloader `Loaded app from partition at offset 0x210000`
    即从 **ota_1**（新固件）启动，且 `[spiffs] partition 'storage_b' mounted`（图片新槽生效）。
    至此 STM32/图片/ESP32 三目标 OTA + 进度条 + 焦点 + 切槽 + 重启**端到端真机全部打通**。
