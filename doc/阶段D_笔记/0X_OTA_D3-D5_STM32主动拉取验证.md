# 0X OTA D-3/D-4/D-5 STM32 固件主动拉取升级 · 真机验证笔记

> 阶段 D 子阶段 D-3（STM32 Bootloader + VTOR）、D-4（OTA 参数 + 块烧写 + CRC32）、D-5（ESP32 Modbus 二次传输 serve 状态机）。
> 交叉引用：计划 `doc/阶段D_笔记/0U_OTA在线升级计划.md`；D-1 基础设施 `0V_OTA_D1基础设施.md`；D-2 下载/双区 `0W_OTA_D2双区切换验证.md`。
> STM32 侧协议定稿：`EDA_EMG/doc/通信协议/OTA升级_设计说明.md`（v2 主动拉取）。
> 状态：**✅ 真机全链路验证通过**（2026-09-03，手机热点 + 生产云端）。

---

## 1. 验证结论（摘要）

| 环节 | 结果 | 关键证据 |
| --- | --- | --- |
| 云端下载 stm32 固件 | ✅ | `ota stm32` → HTTP 下载 52828 B → 落盘 `/spiffs/fw/stm32_fw.bin` |
| 触发 STM32 进 Boot | ✅ | FC06 写 0x0313~0x0317（请求标志+size+CRC32）+ 0x020C 重启，STM32 跳 Boot |
| Boot 主动拉流（v2 协议） | ✅ | `01 41` REQ 帧 offset 从 0 线性 +0xF0，~70ms/块，**221 块无 offset 重发** |
| ESP32 serve 状态机 | ✅ | 每块回 `01 42` DATA（240B + Modbus CRC16），Boot 全部校验通过 |
| 心跳超时改造 | ✅ | OTA 总线活跃期间 FC03 心跳**零下发**；结束静默 3s 后自动恢复 |
| 固件完整性 | ✅ | Boot 端 CRC32 与云端 `f9395b23` MATCH（不 MATCH 不会跳 APP） |
| 新固件运行 | ✅ | RTT 显示 Build time 从 `Sep 3 15:53:02`（J-Link 开发版）变为 `Sep 3 14:50:01`（云端 OTA 包），BAT/ADS/NNC 初始化全 OK |

固件包：size = 52828 B（0xCE5C），CRC32 = `0xF9395B23`，221 块（240B/块，末块 28B）。

---

## 2. v2 协议帧（raw UART，Modbus CRC16 poly 0xA001，从机地址 0x01）

| 帧 | 方向 | 功能码 | 格式 | 长度 |
| --- | --- | --- | --- | --- |
| REQ_BLOCK | Boot → ESP32 | `0x41` | `01 41 [off:4B BE][len:2B BE] crc16LE` | 10B |
| DATA_BLOCK | ESP32 → Boot | `0x42` | `01 42 [off:4B BE][len:2B BE] data[len] crc16LE` | 10+len |
| DONE | Boot → ESP32 | `0x43` | `01 43 [status:1B] crc16LE`（0=成功 1=失败） | 5B |

- 块大小 240B（0xF0）；offset 大端、len 大端。
- 固件整体校验：**CRC32 reflected（poly 0xEDB88320）= zlib.crc32**，打包端 `tools/build_ota_package.py` 计算，Boot 端 `CRC32_Calc` 复核。
- Boot 流程（`EDA_EMG_BootLoad/code/user_main.c OtaPull_Run`）：读 SOTA 参数 → 校验 size → 开 USART3 + CRC32 → **3 次尝试**：擦 APP 区（sector4~7，先擦后拉）→ 逐块拉取（每块 5 次重试、2000ms 超时）→ 写 Flash → CRC32 比对 → MATCH：发 DONE(0) + 清 SOTA + `NVIC_SystemReset`；失败：发 DONE(1) 并 HALT。

## 3. 触发序列（ESP32 console 实测）

```
mb log on            # 开帧日志
ota stm32            # 下载固件到 /spiffs/fw/stm32_fw.bin 并自动触发：
  TX 01 06 03 13 00 01 .. ..   # OTA 请求标志
  TX 01 06 03 14 00 00 .. ..   # (保留/状态)
  TX 01 06 03 15 CE 5C .. ..   # size 高字 = 0xCE5C
  TX 01 06 03 16 F9 39 .. ..   # CRC32 高字
  TX 01 06 03 17 5B 23 .. ..   # CRC32 低字（0xF9395B23）
  TX 01 06 02 0C 00 01 .. ..   # 重启 → STM32 跳 Boot
# 之后 Boot 主动发 0x41，ESP32 进入 serve 模式回 0x42
```

## 4. 关键坑与修复

### 4.1 【根因】APP→Boot 跳转后 Boot 哑巴（VTOR 未切回）
- 症状：STM32 收到重启标志跳进 Boot 后，USART3 完全无输出，ESP32 收不到任何 `0x41` REQ。
- 根因：APP 运行时把 `SCB->VTOR` 指向 APP 向量表（0x08010000）；`JumpToBootLoader()` 跳转前**没有复位 VTOR**。Boot 是被"跳转"进来而非复位进来，VTOR 仍指向 APP → Boot 开 USART3 RXNE/IDLE 中断后，中断向量取到 APP 的 ISR（或无效地址），REQ 发不出/中断跑飞。
- 修复（`EDA_EMG_BootLoad/Core/Src/main.c`）：`main()` 开头 `__disable_irq()` 之后、任何初始化/开中断之前，加：
  ```c
  SCB->VTOR = FLASH_BASE;   /* 非复位跳转(APP→Boot)进来时 VTOR 仍指向 APP(0x08010000),
                               必须切回 Boot 向量表, 否则 USART3 中断跑 APP 的 ISR, REQ 发不出 */
  ```
- 验证：修复后 Boot 每 ~70ms 稳定发一帧 `01 41`，offset 线性递增无重发。

### 4.2 心跳改为"总线静默超时"才下发（OTA 期间不再空发）
- 背景：v2 里 Boot 升级时以 ~70ms/块持续主动通信，ESP32 再每秒固定发 FC03 心跳纯属总线干扰。
- 改造（`main/MODBUS/app_modbus.c` + `main/heartbeat/app_heartbeat.c`）：
  - `mb_process_frame()` 在校验通过、地址过滤后记录 `s_last_rx_us = esp_timer_get_time()`（OTA REQ/DONE/响应/推送帧都算总线活跃）；
  - 新增 `int64_t app_modbus_ms_since_last_rx(void)`；
  - 心跳 1s 定时器仍触发，但 `poll_once()` 里先判断：`ms_since_last_rx() < 3000ms` 则**跳过本次下发**。
- 参数：`APP_HEARTBEAT_POLL_MS = 1000`（检查周期），`APP_HEARTBEAT_IDLE_MS = 3000`（静默阈值）。
- 实测：OTA 触发（318960）后到拉流结束，日志中**再无一条 FC03**；升级结束总线静默 3s 后心跳自动恢复并收到 APP 响应。

### 4.3 Boot 黑盒可观测性（RTT 打印）
- Boot 无 console，前期"哑巴"无法定位。在 `user_main.c` 关键路径加 RTT 打印：SOTA magic/req/size/crc、进入 OTA、每次 attempt 擦除、块失败重试、进度（每 40 块 + 结尾百分比）、`CRC32 calc=... expect=... MATCH/MISMATCH`、DONE/清 SOTA/复位。
- 教训：Boot 这种"跳转进入、失败即 HALT"的黑盒，必须在每个卡点留打印，否则只能靠猜。

## 5. 真机实测时序（2026-09-03）

1. `wifi init` → `wifi connect`（热点 `EDA_EMG_OTA`，拿到 `192.168.43.18`，rssi -27）。
2. `ota manifest` → stm32 ver=0.1.0 size=52828 sha=f9395b23 url=stm32/fw.bin。
3. `mb log on` → 先看到正常心跳 FC03（`01 03 03 05 00 0A` → 25B 响应）。
4. `ota stm32` → 下载 52828B 落盘（318480）→ FC06 触发序列（318660~318960）→ `[ota] serve start (fw=0x3c26c9dc size=52828)`。
5. 322670 起 Boot 拉流：`RX(10): 01 41 00 00 00 00 00 F0 .. ..` → offset 每帧 +0xF0，~70ms/块。
6. 拉流期间无 FC03 心跳（超时改造生效）。
7. 传完后 Boot CRC32 MATCH → 发 DONE(0) → 清 SOTA → 复位。
8. RTT（J-Link）看到新 APP 启动：Build time = **Sep 3 14:50:01**（云端包），替换了 J-Link 烧的 15:53:02 开发版；`[BAT] BQ25890 init chipOk=1`、`[ADS] ADS ID=0x73 OK`、`[NNC] OTP calib loaded` 全部正常。

> 判定依据：Boot 只有 CRC32 比对通过才会清 SOTA 并复位跳 APP；失败则发 DONE(1) 后 HALT，永远不会进 APP。现在 APP 正常运行且 Build time 变为云端版本 → 221 块全部写入、CRC32 MATCH、新固件完整可用。

## 6. 复测/回归要点

- [x] 升级后新 APP 功能正常（BAT/ADS/NNC 初始化 OK）。
- [ ] **断电再上电**：应直接进 APP、不触发 OTA（验证 SOTA 参数已清零，不会反复升级）。
- [ ] 异常路径：拔线/断电制造块失败，验证 Boot 5 次重试 + 3 次 attempt + DONE(1) HALT（不会刷成砖）。

## 7. 控制台命令速查（OTA 相关）

```
wifi init | connect | disconnect | state | ip
ota manifest | esp32 | image | stm32     # 均在独立 ota 任务(16KB 栈)异步执行，看日志
mb info | fw | status | read <addr> [n] | write <addr> <v> | write10 <addr> v..
mb push on|off [ch] | acq | log on|off
```

打包上传（PC，工程根目录）：`py tools/build_ota_package.py [--esp32-ver .. --image-ver .. --stm32-ver ..] [--upload]`。
