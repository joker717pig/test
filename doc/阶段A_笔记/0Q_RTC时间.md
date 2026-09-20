# 阶段 RTC 笔记 · STM32 RTC 时间上报 + 设置页对时（全链路）

> 2026-08-12 · 实现完成（代码）；待烧录实测。
> 关联：STM32 侧 `d:\GitLab\EDA_EMG\doc\通信协议\RTC时间上报_设计说明.md` + `MODBUS_寄存器映射.html`。
> 前情：阶段 E1 参数存储时明确「日期时间不做（无 RTC）」——本期补上 STM32 RTC。

## 一、需求与协议（用户确认）

- 上报 = **只读寄存器**（0x0305~0x0309 年/月/日/时/分，**不传秒**），仿 KEY_STATUS 批量 FC03 读。
- 对时 = **双向**：设置页滚轮草稿 → `0x030A RTC_CTRL`=1 提交 STM32 RTC。
- 范围 = 全链路（STM32 + ESP32 顶栏 + 设置页对时 中/英）。
- 串口/USART 不动（已调通）。

## 二、改动文件

| 文件 | 内容 |
|---|---|
| `main/events/app_events.h` | 新增 `APP_EVENT_RTC_POLL` 事件 ID |
| `main/MODBUS/app_modbus_reg.h` | 加 0x0305~0x030A 六宏 |
| `main/time/app_time.c/h`（新） | esp_timer 周期触发→只发事件；事件循环 handler 里读 STM32 RTC → `clock_settime` 同步 C 时钟；`app_time_commit()` FC10+FC06 |
| `main/app_main.c` | `app_modbus_init()` 后 `app_time_init()`（注册 handler + 建 esp_timer） |
| `main/CMakeLists.txt` | SRC_DIRS / INCLUDE_DIRS 加 `"time"` |
| `main/ui/Frame_enent.c` | **无需改**：`update_time_cb` 已用 `time()+localtime_r`，C 时钟同步后自动显示真实时间 |
| `main/ui_page_cn/SetPage/PelFlo_Set_ui.c` + `_event_cb.c` | 滚轮 VALUE_CHANGED 捕获 → 草稿；确定提交；滚轮初值=草稿 |
| `main/ui_page_en/SetPage/set_En_ui.c` + `_event.c` | 同上（英文） |

## 三、实现要点 / 坑（重要）

1. **时钟同步**：`mktime` → `clock_settime(CLOCK_REALTIME, &ts)`（勿用 `stime`/`settimeofday` 依赖不确定）。TZ 未设置（UTC），而 STM32 RTC 存本地墙钟 → `localtime_r` == 所设墙钟，两端一致。
2. **周期读取 = esp_timer + 事件框架（零新增任务）**：esp_timer 回调跑高优先级定时器任务、**绝不能阻塞** → 回调里只 `esp_event_post(APP_EVENT_RTC_POLL)`（非阻塞）就返回；modbus 事务在**事件循环任务**的 handler 里执行（可阻塞，健康 ~20ms/次、5s 一次；采集期间 `MB_ERR_BUSY` 立即返回）。不要直接在 esp_timer/LVGL 回调里跑 modbus。
3. **采集冲突**：push 模式事务被拒返回 `MB_ERR_BUSY` → 跳过本轮；因 C 时钟已同步，顶栏本地推算继续走秒不卡。
4. **草稿跨页**：日期页 → 时间页要共享「待提交时间」，放 `app_time` 模块 `s_pending`（`app_time_get_pending/set_pending`），首次初始化 = 当前 RTC。滚轮初值 = 草稿（`Set_Roller_SyncFromPending`）。
5. **滚轮选项表修正**（[我们]）：CN 原 `Hour` 1~24 点 / `Minu` 含 `140分`、`58`缺分、`60分` 笔误；EN 原 1~24 / 1~60 → **统一两位补零 00~23 / 00~59**（否则选不出 0 时/0 分，且 24/60 提交会被 STM32 拒绝）。
6. **滚轮事件挂载**：Date 页原只有 Month 挂了 VALUE_CHANGED（且 user_data=NULL），年/日没挂 → 全挂上并把 user_data 传 `widget`，否则滚轮变化不更新草稿。
7. **程序化 set_selected 不发事件**（LVGL9）：初值同步用 `lv_roller_set_selected(..., LV_ANIM_OFF)` 直接定位即可，不需事件。
8. **提交阻塞**：`app_time_commit` 在 LVGL 事件回调里做 2 次事务（FC10+FC06），正常链路 ~40ms，可接受；失败仅 LOGW 不弹窗。
9. **构建坑（重要）**：`app_modbus_write_registers` 签名是 `(addr, values, count)`，不是 `(addr, count, values)` —— 写成后者直接 `-Wint-conversion` 编译失败（IntelliSense 不报，真编译器才报）。
10. **构建坑**：事件文件 CLICKED 分支内联插入代码后要核对括号配对——曾少一个函数收尾 `}`，导致后续所有函数报 `invalid storage class for function` + `expected declaration or statement at end of input`（一个缺失括号引发一串假错误）。
11. **🔥 运行坑（烧录实测发现，最重要）**：默认**事件循环任务栈仅 2304B**（`CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE`），在事件 handler 里跑 modbus 事务（`app_modbus_read_registers` 栈上 **256B 响应缓冲** `resp[MB_RESP_BUF_SIZE]` + printf 调用链）→ 启动即栈溢出崩溃（`vApplicationStackOverflowHook` → abort）。**修法**：`sdkconfig.defaults` 加 `CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE=8192`。
12. **坑（sdkconfig.defaults 语法）**：`CONFIG_xxx=y  # 行内注释` 的**行内注释会被 Kconfig 当成值的一部分**（值含非数字 → `invalid ... assignment ignored` 静默失效）→ 注释必须**单独一行**写在配置项上方。
13. **坑**：`esp_event_loop_set_default()` **不是公开 API**（编译隐式声明报错）→ 不要想自定义大栈事件循环 + set_default；直接改 sdkconfig 栈即可（`esp_event_loop_create_default()` 会用 `CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE`）。

## 四、验证（待烧录）

1. `idf.py build` 0 错误（main 新加 time 目录需 `idf.py reconfigure` 一次，会自动生成）。
2. console `mb read 305 5` 读到时间（与 PC 一致；首启为 STM32 默认 2026-01-01 00:00 或已对时值）。
3. 设置页（中/英）改日期/时间 → 确定 → `mb read 305 5` 回读一致。
4. 顶栏显示真实时间（非开机计时假时间）；走秒正常（本地推算）。
5. `mb push on` 采集期间：时间轮询跳过不卡（每 5s 一次 LOG 跳过或静默）。

## 五、后续

- STM32 VBAT 有无 → 决定 RTC 掉电是否保持；无则建议 ESP32 开机自动对时（检测 RTC 无效时 push 本地时间）。
- 顶栏日期行（`update_time_cb` 里注释的 Date_Label）如需显示日期可放开。
