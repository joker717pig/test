# 0T UI 对接 Modbus 真实治疗控制（阶段C 核心）— 治疗控制模板

> 状态：**治疗控制模板已完成 + 编译通过（2026-08-19）**；烧录 + 真实设备联调待做。
> **定位：本实现是模板**——用 **处方3疗程1** 把「下发真实治疗命令 + 30 分钟会话 + UI 显示」全链路做通，
> 同事照着本模板接入其余疗程/处方（见 §2 模板使用指南）。
> 目标平台：`d:\GitLab\EDA_EMG_ESP32`（ESP32-S3 + LVGL 9.5 + app_modbus 主站）
> 下位机：`d:\GitLab\EDA_EMG`（STM32 Modbus 从机）
> 交叉引用：`main/therapy/therapy_rx.c`（处方表格）、`main/therapy/app_therapy.h/.c`（模板本体）、
> `EDA_EMG_PC/src/ui/main_window.py::_on_stim_params`（下发参考）、`main/voice/voice.c`（异步样板）

## 1. 模板范围（用户拍板 2026-08-19）
- 用 **处方3: 盆底肌治疗 疗程1**（`rx3_c1`）联调：步骤0（4Hz/500us 刺激28s 休息2s）→ 步骤1（50Hz/300us 8s/2s）
  → **顺序循环**，总时长 30 分钟（单轮 40s：步骤0 30s + 步骤1 10s；45 轮 = 1800s = 30min）。
- 进入治疗前 **Page4 两步设强度**：步骤0 强度、步骤1 强度分别设置。
- Page5 显示**当前步骤强度**，± 调当前步骤强度并实时下发。
- 强度**严格按设置**：档位 0 → mA=0 不输出（无默认回退）。
- 加 console 时间缩放命令（默认真实速度，联调验证用）。

## 2. 模板使用指南（同事接入新疗程）
1. **处方数据**：已在 `main/therapy/therapy_rx.c` 的 `g_rx_all[]`（处方1/2/3/4，每处方多疗程；
   每疗程 = 有序步骤数组 `rx_step_t`）。
2. **选择疗程**：改两处调用点传入 `&g_rx_all[RX_x]->schemes[疗程idx]`：
   - UI：`PelFlo_TheChild_event.c` 的 `ChildPage_P5Btn_Event_cb` Start 分支（当前传 `RX_3->schemes[0]`）
   - console：`app_console.c` 的 `cmd_therapy` start 分支（同上）
3. **会话时长**：`app_therapy.h` 的 `THERAPY_TOTAL_MS`（默认 30 分钟，按需改）。
4. **步骤执行（模板核心，无需改）**：按 `scheme->steps[]` 顺序循环——每个 STIM 步骤
   刺激 `on_s` 秒 → 休息 `off_s` 秒 → 下一步，循环到总时长结束；通道按 `step.ch`（CH1/CH2/双通道）自动下发。
5. **强度映射**：步骤的 `stim_stage`（强度阶段）→ `intens1`(1) / `intens2`(2)；
   **单位即 mA（0~90 原值直传，见 §7.2；旧的「档 0~10，mA=档×9」已废弃）**。
   ⚠️ 旧版「步骤0/步骤1 固定对应」的说法也已废弃——现按 `step.stim_stage` 查表（§7.3 `cur_stim_stage()`）。
6. **非 STIM 步骤**（ACQ/TRAIN/COND/WAIT：采集/训练/条件刺激/等待）模板未实现，同事按需在
   `advance_one_session_sec()` 推进处扩展。

## 3. 实现
### 3.1 新建 `main/therapy/app_therapy.h/.c`（模板本体）
- 会话状态机：IDLE / RUNNING{PH_ON, PH_OFF} / PAUSED / END；`step_idx` 遍历 `scheme->steps[]` 顺序循环。
- 计时：`esp_timer` 1s(真实) → `APP_EVENT_TREATY_POLL` → 事件任务推进 `scale` 个会话秒
  （默认 1=真实速度；`therapy_set_scale(30)` = 1 真实秒 = 30 会话秒）→ 状态机 + Modbus 下发，不卡 LVGL。
- 步骤 ON 进入：FC10 写当前步骤 `[freq,pw,mA]`（按通道 CH1 0x0100 / CH2 0x0103 / 双通道 6 寄存器）
  → FC06 `0x0106=通道掩码` 启动（mA=当前强度阶段强度，**0~90 原值直传**；mA=0 时不启动）。
- 步骤 OFF 进入：FC06 `0x0106=0` 停。
- pause / resume：停波 / 重发当前步骤参数+启动（0T 方案A）。
- 强度实时调：`APP_EVENT_TREATY_INTENS` → 事件任务写 mA + 重发 `0x0106`（只置位触发渐变）。
- API：`app_therapy_init` / `therapy_start(scheme,i1,i2,ui)` / `pause/resume/stop` /
  `set_intensity` / `set_scale` / `active` / `remain_ms` / `status` / `current_intens` / `print_status`。

### 3.2 修改文件
| 文件 | 改动 |
|---|---|
| `main/events/app_events.h` | +`APP_EVENT_TREATY_POLL` / `APP_EVENT_TREATY_INTENS` |
| `main/app_main.c` | `voice_init()` 后 `app_therapy_init()` |
| `main/console/app_console.c` | +`therapy` 命令组（help/start/stop/pause/resume/status/scale） |
| `ui_page_cn/PelFlo_ThePage/Child_ThePage.h` | +`extern Param_Data_t The_Param_Data1`（Page4 强度1/2） |
| `ui_page_cn/PelFlo_ThePage/Child_ThePage.c` | ThePage_CtxData total/remain = 30min；`The_BackToMenu` + Page5 ESC 停/暂停引擎 |
| `ui_page_cn/PelFlo_ThePage/PelFlo_TheChild_event.c` | `Trest_Widget_timer_cb` 改**渲染型**（引擎驱动时不自减 + 强度标签跟随）；Start=新会话/恢复、Pause=暂停、Back=暂停（确认"是"→stop） |
| `ui_page_cn/MenuPage/menu_ui_event_cb.c` | `Treat_Widget_Event_cb` 强度± → `therapy_set_intensity`；`Back_Btn_Event_cb` 确认"是"→`therapy_stop` |
| `ui_page_cn/PelFlo_ThePage/PelFlo_The_ui.c` | `PelFlo_ThePage_Load` 切页安全网 `therapy_stop()` |

## 4. 验证
> ⚠️ 本节的「档位×9」等旧口径描述已过时，以 **§7（2026-09 修复记录）** 为准。
- [x] 编译零错误：`eda_emg_esp32.bin` 0xcc450（835KB），app 分区 80% free。
- [x] 烧录 COM5（app + storage），monitor 确认 `therapy template init OK` + UI 启动正常，**STM32 下位机在线**（心跳/RTC/电池/语音 Modbus 全部应答）。
- [x] **console 全链路验证（2026-08-19 实测）**：
  - `therapy scale 30` → `therapy start 0 0`（强度 0，不启波）：step0(4Hz/500) → step1(50Hz/300) 交替循环，
    FC10 `[f=4,pw=500,mA=0]`/`[f=50,pw=300,mA=0]` 帧正确、从机全应答；30 分钟会话 scale=30 下 60 真实秒跑完，
    `session END (总时长完成)` + 结束停波，全程无复位。
  - `therapy start 3 5`（强度 3→27mA）：FC10 `[f=4,pw=500,mA=27]` 正确、FC06 `0x0106=1` 启波帧已发出。
- [x] 回归：四宫格其它格 + 治疗页启动日志健康，无崩溃。
- [ ] **真实输出联调（阻塞）**：见 §5 坑——启动真实电刺激时 ESP32 欠压复位，需先解决供电。
- [~] **UI 目检（2026-08-19 多轮）**：时间 1 秒一跳（恢复 scale=1）✅；阶段参数切换显示已加（脉宽/频率随步骤切换）✅；
  **重复点击「开始」不再重置时间（运行中忽略）✅ 用户验证 OK**；± 实时改当前步骤 mA 待充电后验；
  **开始/暂停按钮交互方案待客户定**（见 §6）。

## 5. 坑 / 注意
- **🔴 供电坑（联调阻塞，2026-08-19 实测发现）**：心跳显示电池 `volt=2304mV`（2.3V，几乎没电）。
  `therapy start 3 5` 启动真实电刺激（FC06 `0x0106=1` 后）→ ESP32 **欠压复位**（monitor `ClearCommError` + 设备重启）。
  根因推断：27mA 电刺激瞬间高压电流把共享供电（低电量电池）拉垮 → 欠压复位。**解决：插充电/接电源后再做真实输出联调**；
  强度 0（不启波）全链路可正常跑完（本次验证路径）。
- **坑（2026-08-19 目检修复）**：运行中重复点「开始」原逻辑会再次走 `therapy_start` → 时间重置回 30:00。
  修复：Start 分支按 `therapy_status()` 分派——PAUSED=resume、非 RUNNING=新会话、RUNNING=忽略（不重置）。
  用户验证 OK（2026-08-19）。
- STM32 波形驱动无内置 on/off 循环 → ESP32 必须自管启停（写 0x0106）。
- 事件任务（非 LVGL 线程）只写 int32 裸字段（`remain_sec`/`status`/`Intens_Value`），LVGL 线程只读渲染，跨任务裸写可接受。
- Page5 初始 `Label_RemTime` 硬编码"已完成"（同事原设计），Start 后由渲染定时器覆盖。
- 强度档×9 线性映射（0T 决策）；写 mA 后重发 0x0106 触发渐变（只置位不置反位）。
  > ⚠️ **已废弃（v2.4）**：现为 0~90 mA 原值直传，见 §7.2。
- 新建 `.c` 后需 `idf.py reconfigure`（SRC_DIRS GLOB 缓存坑）。
- 高 scale（如 30）下相位切换量化到 1 真实秒/步（esp_timer 1s 粒度），适合快速看流程；**精确时序用 scale=1**。

## 6. 待办 / 遗留
- 烧录 + 真实设备联调（下位机在线，见验证清单）——**这是本模板的验收目标**。
- **🔶 待客户定（2026-08-19 目检反馈）**：治疗页 Page5 **开始/暂停按钮交互方案**——
  播放图标左侧有独立"暂停"按钮；运行中是否要中间按钮切为暂停图标 / 是否保留独立暂停按钮，
  **由客户拍板**。当前保持同事原版（独立开始/暂停按钮，无图标切换），不擅自改 UI 操作。
- 其余处方/疗程接入（按 §2 指南）；英文治疗页；ACQ/TRAIN/COND/WAIT 步骤扩展；
  治疗状态轮询显示（0x0107/0x0300）；电极脱落/过流告警。

---

## 7. 2026-09 修复记录（v2.2 → v2.4）

### 7.1 回主菜单仍有电流输出（v2.2 → v2.3，`40850d3`）
- **现象**：客户反馈治疗中「返回主菜单」后仍有电流输出。
- **根因**：同事提交 `768d5841`（zhouqiuting, 2026-09-04）注释掉了 `The_BackToMenu()` 里的
  `therapy_stop()`，只留 `emg_force_stop_acq()`；而 **STM32 波形驱动无通信超时/看门狗自保**
  （NNC6521 硬件 AWG 自由循环，一直输出）→ 停波令没送达 → 电流不停。
- **修复（三重兜底）**：
  1. `The_BackToMenu()` 恢复 `therapy_stop()`（引擎停 → 发停波令）。
  2. `Ui_EnterMenu()` 加**全局兜底** `therapy_emergency_stop()`（幂等：无条件停采集 + 停波 + 回 IDLE），
     为 UI 的任意返回/切页路径兜底。
  3. `wave_stop()` **可靠化**：
     - 推流（push）期：`app_modbus_write_register_nowait(0x0106, 0)` **连发 2 次**插队（推流帧间隙）。
     - 非推流期：先确认写（`mb_transact`），失败则 `nowait` **补发 3 次**。
- **结论**：**STM32 侧不保证自保，ESP32 必须保证停波令送达**（这是安全关键收口）。

### 7.2 强度口径：档位×9 换算 → 0~90 mA 原值直传（v2.3 → v2.4）
- **背景**：UI 已全部迁移为 **0~90**（设置页滑块 `Create_Sliders(..., 90)`；治疗中实时滑块
  `if (value >= 90) value = 90;`），最大挡=90 即不再需要换算。
- **根因**：引擎残留旧宏 `INTENS_TO_MA(i) ((i)*9)` → UI 选 1 被换算成 mA=9（输出偏大 9 倍）。
- **修复**：删除该宏，`step_ma()` 直接返回 `step_intens()`（单位即 mA），**原值下发 STM32**。
- 上板实测：STM32 日志 `mA=1` / `mA=2` 正确。

### 7.3 🔴 阶段切换强度滞后（mA 与 UI 不同步）（v2.4，`f22a041`）
- **现象**：UI 已显示「阶段2」，但实际打印/下发日志仍是阶段1 的 mA（慢一拍）。
- **根因（两处数据源不同步）**：
  - `step_intens()` 读的是 **缓存 `s_th.stim_stage`**，而该缓存只在 `advance_one_session_sec()`
    **开头**（此时 `step_idx` 还是旧值）赋值；
  - 步骤推进函数（`my_enter_next_on()` / `enter_next_data_acq()`）在**推进 `step_idx` 之后立刻**
    `wave_apply_start()` 下发 mA → 读出的是旧阶段强度；
  - 而 UI 的「当前阶段」读的是 `cur_step()->stim_stage`（**立即反映新步骤**）→ 两者不同源，表现不一致。
- **修复**：
  1. 新增 `cur_stim_stage()`：**直接查处方表** `s->stages[stage_idx][step_idx].stim_stage`，不用缓存；
  2. `step_intens()` / `therapy_set_intensity()` 一律走 `cur_stim_stage()`；
  3. 缓存同步从 `advance_one_session_sec()` **开头**移到 `enter_phase_treat()` **之后**：
     ```c
     enter_phase_treat(st);           /* 本秒推进（可能已切下一步/新阶段） */
     s_th.stim_stage = cur_stim_stage();  /* 推进后再同步缓存 */
     ```
- **安全性质**：`cur_stim_stage()` 在 `stage_idx >= stage_cnt`（越界/结束）或步骤越界时返回 **0**
  → `step_intens()` 返回 0 → `step_ma()` = 0 → `wave_apply_start()` 内 `ma==0` 不启波并写 STOP。**安全**。

### 7.4 ⚠️ 关键约定：两个「阶段」不要混淆
| 名称 | 维度 | 字段 | 作用 |
|---|---|---|---|
| 执行阶段 `stage_idx` | `rx_scheme_t` | `stage_cnt` / `steps_per_stage[]` / `stage_time[]` | 采集 / 刺激 / 训练 / 条件刺激，按 `stage_time[]` 推进 |
| 强度阶段 `stim_stage` | `rx_step_t` | `stim_stage` | 决定用 `intens1`(1) 还是 `intens2`(2)；**UI「当前阶段:N」显示的就是它** |

- 处方**固有休息间隔**：每个 STIM 步骤 `on_s` 刺激 → `off_s` 休息（`enter_off()` → `0x0106=0`），
  在阶段内 `step_idx` 取模循环至 `stage_remain_ms` 耗尽。**这不是 bug**。
- 执行阶段切换还会**额外插一次 off**（`my_enter_next_on()` 的 `last_stage != stage_idx` 分支）。

### 7.5 版本与提交
- v2.3：`40850d3`（停波兜底）；v2.4：`f22a041`（强度滞后修复 + 0~90 原值直传 + 设置页滑块笔误
  `if (value >= 90) value = 10` → `value = 90`）。三分支 `main`/`ui_by_zqt`/`ui_by_zcl` 均已同步。

