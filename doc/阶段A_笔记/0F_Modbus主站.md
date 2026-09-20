# 阶段 A · Modbus RTU 主站驱动（EDA_EMG_ESP32）

> 2026-08-10 · 编译通过，待接 STM32 硬件实测
> 关联：`doc/开发计划.md` 阶段 A；下位机 `d:\GitLab\EDA_EMG\code\MODBUS\MB_RegMap.h`；PC 参考 `d:\GitLab\EDA_EMG_PC\src\modbus_client.py`；按键协议 `d:\GitLab\EDA_EMG\doc\通信协议\按键状态上报_设计说明.md`

## 一、接线（用户 2026-08-10 确认）

| ESP32-S3 | 信号 | STM32F407 |
|---|---|---|
| **GPIO17 (UART1 TX)** | `MCU_U3_RX` | **PA3 (USART2_RX)** |
| **GPIO18 (UART1 RX)** | `MCU_U3_TX` | **PA2 (USART2_TX)** |

- 交叉相连，115200 8N1，无校验；console 走 USB-Serial-JTAG（GPIO19/20），不与 UART1 冲突
- 注：早期按原理图模块引脚推断为 GPIO43/44（TXD0/RXD0），**用户实测确认实际板子走 IO17/IO18**（ESP32-S3 UART1 默认引脚），以本表为准

## 二、协议摘要（对齐 MB_RegMap.h / PC 已验证解析）

- 从站地址 `0x01`；功能码 `03` 读保持 / `06` 写单 / `10` 写多
- 寄存器三区：
  - 刺激控制 `0x0100~0x0107`（CH1/CH2 频率·脉宽·电流 + WAVE_CTRL + WAVE_STATUS）
  - 采集控制 `0x0200~0x0207`（SPS/SAMPLES/CHANNEL/CTRL/STATUS/COUNT/PUSH_CNT/ADS_STAT）+ 数据区 `0x0210`(CH1)/`0x0250`(CH2) 各 50 样本
  - 状态诊断 `0x0300~0x0304`（DEV_STATUS/ELEC_STATUS/FW_VERSION/ERROR_CODE/**KEY_STATUS**）
- 主动推送帧（STM32 发起，非标准请求/响应）：
  ```
  [01][03][BC][CNT_H][CNT_L][DATA...][CRCL][CRCH]
  BC = 2 + CNT × bps（双通道 4 / 单通道 2）
  CNT=0   → 按键帧（9B）：[01][03][02][00][00][KS_H][KS_L][CRC]
  CNT=1~50 → 采集帧：每样本 big-endian int16；双通道 = CH1[2B]+CH2[2B]
  ```
- 按键位图 KS：bit0=END / bit1=UP / bit2=RIGHT / bit3=LEFT / bit4=DOWN / bit5=ESC / bit6=PWR
- CRC16 Modbus 多项式 `0xA001`，低字节在前
- **采集期间主站不发请求**（push 模式拒绝事务），仅监听推送帧，对齐 PC 行为

## 三、驱动结构（新增 `main/MODBUS/`）

| 文件 | 内容 |
|---|---|
| `app_modbus.h` | 公开 API + 返回码 `mb_err_t` + `mb_sample_t` |
| `app_modbus.c` | 实现：UART + RX 任务（定帧/CRC）+ 事务层 + 推送解析 + 事件 + 环形缓冲 |
| `app_modbus_reg.h` | 寄存器/功能码/位图常量（对齐 MB_RegMap.h，仅 ESP32 需要部分） |

- **RX 任务**（栈 4096，优先级 6）：`uart_read_bytes` 读字节 → 累积缓冲 → **按「帧头校验 + 功能码长度规则」提取完整帧** → CRC → 分发
  - 长度规则：fc03 读响应/采集帧 `3+BC+2`；按键帧（无挂起请求且 CNT=0）`9B`；fc06/10 写响应 `8B`；异常响应 `5B`；帧头地址不符/未知功能码 → 逐字节重同步
  - 有挂起请求 → 校验功能码匹配后作为响应 → give 信号量
  - 无挂起 / 不匹配 → 推送帧解析
- **事务层**：互斥串行（`s_tx_mutex`）+ 150ms 超时 ×4 重试；**代际计数 `s_resp_gen`** 过滤陈旧响应（防超时后晚到响应被下一事务误收）；异常响应（fc|0x80）不重试
- **按键帧**：`esp_event_post(INPUT_EVENT, INPUT_EVENT_BUTTON_PRESSED, key_id=bit)`，timeout=0 非阻塞
- **采集帧**：解析后入环形缓冲 `MB_ACQ_RING_CAP=4096` 样本（`app_modbus_acq_read` 读走，阶段 D 波形用）
- **Kconfig**：`MODBUS_UART_NUM/TX/RX/BAUD/SLAVE_ID/RESP_TIMEOUT_MS/RETRIES/FRAME_GAP_MS`

## 四、console 命令

```
mb info                   # 配置/状态
mb fw                     # 读固件版本 0x0302（预期 0x0100）
mb status                 # 读 0x0300~0x0304
mb read <addr> [n]        # FC03，地址十六进制
mb write <addr> <value>   # FC06
mb write10 <addr> v1 ...  # FC10
mb push on|off [ch]       # 推送监听（ch 1/2/3；开启后事务被拒）
mb acq                    # 环形缓冲状态
mb log on|off             # TX/RX 十六进制帧日志（默认开）
```

## 五、验证结果（2026-08-10 硬件实测全部通过）

- [x] `idf.py build` 零错误（app 0x152040 / 67% free）；Kconfig 默认 UART1 17/18 115200 生效
- [x] 烧录 COM5 + monitor：`MODBUS init OK (UART1 TX=17 RX=18 @115200 ...)`
- [x] `mb fw` → `0x0100`（v1.0）；`mb status` 0x0300~0x0304 批读正确
- [x] FC03 读（单/批）、FC06 写单、FC10 写多（写后读回一致）
- [x] push 模式拒绝事务（`read failed: busy (push mode)`）
- [x] 采集推送：`ACQ frame CNT=10 ... (ring=4096)`，双通道 big-endian 解析正确（未接电极 → 满量程 ±32768，属正常）
- [x] 推送洪流中写操作正确区分推送帧与响应（FC06 响应在帧流中成功）
- [x] STM32 按键 → `KEY bit=N` + `INPUT_EVENT_BUTTON_PRESSED`（实测 5 键 END/UP/RIGHT/LEFT/DOWN 全通过）

## 六、坑（重要）

1. **uint32_t = unsigned long**（xtensa 工具链）：`printf("%u", (uint32_t)x)` 报 `-Werror=format` → 用 `(unsigned)x`（同宽）或 `%lu`
2. **uint8_t/uint16_t 提升为 int**：`%u`/`%X` 与 int 同宽不告警（GCC 规则），无需 cast；**宽类型才报错**（如上）
3. **CMakeLists REQUIRES** 需显式加 `esp_driver_uart`（REQUIRES 替换默认，driver 组件头文件不会自动注入）
4. **SRC_DIRS 不递归**：`main/MODBUS/`（大写目录名）显式列入 SRC_DIRS/INCLUDE_DIRS
5. **受限终端**：`$env:` 赋值与 `echo`/`Test-Path` 不可用 → 用 .NET（`[Environment]::SetEnvironmentVariable` / `[System.IO.File]::Exists`）；IDF 环境需在**同一条命令**里设好再跑（跨终端不持久）；`idf.py -p COM5 flash` 直接跑，勿加 `|` 管道（吞输出）
6. **响应/推送歧义**：响应与推送帧都以 `01 03` 开头 → 事务按「功能码匹配 + 期望字节数」校验，不匹配按推送处理并继续等
7. **陈旧响应竞态**：超时后晚到的响应可能污染下一事务 → `s_resp_gen` 代际计数，仅接受「发送后新到」的响应
8. 采集环形缓冲满时覆盖最旧样本（`app_modbus_acq_read` 消费后腾位）
9. **【实测】定帧不能用「空闲超时 + 长等待」**：20ms 间隔的推送帧会在 50ms 读等待内合并成粘包（141B）→ CRC 失败刷屏；须按「帧头校验 + BC 长度提取」（对齐 PC `01 03` 定帧），帧头不符/未知功能码逐字节丢弃重同步
10. **【实测】FC10 帧长漏算 CRC**：`mb_append_crc` 后总长 = 7 + count*2 + **2**，传 `9+count*2` 而非 `7+count*2`，否则发不带 CRC 的帧 → 从站静默丢弃 → 超时
11. **【实测】按键帧长度特殊（9B）**：`01 03 02 00 00 KS_H KS_L CRC`，其 `BC=02` **只含 KS 2 字节，CNT 的 2 字节不在 BC 内**（与采集帧不同）→ 不能一律用 `3+BC+2`；无挂起请求时按 `CNT==0` 判按键帧取 9B，有挂起请求时 fc03 为读响应仍用 `3+BC+2`（否则读回值为 0x0000 的寄存器也会被误判）
12. **【实测】采集操作顺序**：先配 SPS/SAMPLES/CHANNEL → `ACQ_CTRL=1` 启动 → **再** `mb push on`（push 模式会拒绝事务，若先开 push 则启动采集的写会被拒），停止时先 `mb push off` 再 `ACQ_CTRL=0`
