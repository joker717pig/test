# 0W OTA D-2 下载/校验/双区切换验证笔记

> 阶段 D 子阶段 D-2：真实云端 `version.json` 拉取 + 下载 + sha256 校验 + 双区切换 + 落盘验证。
> 交叉引用：计划 `doc/阶段D_笔记/0U_OTA在线升级计划.md`（§9）；D-1 基础设施 `doc/阶段D_笔记/0V_OTA_D1基础设施.md`。
> 状态：**✅ 三个升级目标全部上板验证通过**（2026-09-01）。
> 实测设备：ESP32-S3-N16R8 @ COM10，云端 `http://www.dgbuysmart.com/fota/lzq3496900/eda_emg/`。

---

## 1. 验证结论（摘要）

| 目标 | 命令 | 结果 | 关键证据 |
| --- | --- | --- | --- |
| 版本清单 | `ota manifest` | ✅ | 三目标 version/size/sha256/url 全打印正确 |
| ESP32 固件 | `ota esp32` | ✅ | 下载 1.7MB → sha256 verified → set_boot_partition → 重启后 `App FW ver : 2.0` |
| 图片镜像 | `ota image` | ✅ | 擦除+下载 6.2MB → sha256 verified → `active storage slot flip to storage_b` |
| STM32 固件 | `ota stm32` | ✅ | 下载 49KB → 落盘 `/spiffs/fw/stm32_fw.bin` |

---

## 2. 完整实测时序（2026-09-01）

### 2.1 版本号提升（制造可观测差异）
- `APP_FW_VERSION` `0x0100` → `0x0200`（`main/ota/app_ota.h`）
- `idf_component.yml` `version: 0.1.0` → `0.2.0`
- `app_main.c` banner 加 `App FW ver : %u.%u`（BCD 拆解打印）→ 运行时显示 `App FW ver : 2.0`
- 编译产物 `eda_emg_esp32.bin` 1701232 B（旧 0.1.0 为 1699712 B 附近，仅 banner 变化）

### 2.2 打包 + 上传
- `py tools/build_ota_package.py --esp32-ver 0.2.0 --image-ver 0.1.0 --stm32-ver 0.1.0 --upload`
- 上传 4 个文件到云端 `ftp://203.170.59.165:21/eda_emg/`：`version.json` + `esp32/eda_emg_esp32.bin` + `image/storage_a.bin` + `stm32/fw.bin`
- 云端 HTTP 200 可达性验证通过（`Invoke-WebRequest` version.json）

### 2.3 上板（monitor @ COM10）
1. `wifi connect` 首次失败 → 重试成功（见 §3.1）
2. `ota manifest` → esp32=0.2.0 / image=0.1.0 / stm32=0.1.0
3. `ota esp32` → 下载 1.7MB（~14s）→ sha256 verified → `boot partition switched, reboot to apply`
4. **手动重启**（重启 monitor 触发 `USB_UART_CHIP_RESET`）→ bootloader `Loaded app from partition at offset 0x210000`（ota_1）→ `App FW ver : 2.0`
5. 重连 WiFi → `ota image` → 擦除 6MB + 下载 6.2MB（~60s）→ sha256 verified → active slot 切 storage_b
6. `ota stm32` → 49KB 落盘 `/spiffs/fw/stm32_fw.bin`，`spiffs ls` 确认 49833 B

---

## 3. 踩坑记录

### 3.1 WiFi 首次 `esp_wifi_connect` 失败，重试一次即成功（时序问题）
- 症状：`wifi connect` 返回 `ESP_ERR_WIFI_CONN`；日志 `sta is connecting, return error` → `auth → assoc → run → init`（跑了一遍连上又断），state 变 FAILED。
- 根因：首连时 `esp_wifi_connect` 与内核连接状态机手速竞争（`sta is connecting` 说明上电后初次 connect 被内部状态冲突拒绝）。非信号/密码问题。
- 解决：**再 `wifi connect` 一次**即成功（`wifi:connected with EDA_EMG_OTA, rssi -28`，拿到 `192.168.43.17`）。
- 影响：后续 D-6 页面接入时，OTA 触发逻辑需对 `ESP_ERR_WIFI_CONN` 做**一次自动重试**，或 `connect` 前先 `disconnect`/等待状态回 IDLE。

### 3.2 OTA 成功后**不自动重启**（设计如此，需调用方重启）
- esp32：`esp_ota_set_boot_partition()` 后仅 `ESP_LOGI("wait reboot (调用方重启)")`，**不调 `esp_restart()`**。
- image：`app_spiffs_set_active_slot()` 后 `ESP_LOGI("reboot to switch slot")`，同样不重启。
- 含义：`set_boot_partition` / `set_active_slot` 只是**写标记**，重启后 bootloader / `app_spiffs_init` 才会真正切到新槽。实测必须断电/复位/重连 monitor 触发 `USB_UART_CHIP_RESET` 才生效。
- 待定（见 §4 D-6）：是否在 UI 层升级成功后提示用户「重启生效」并调 `esp_restart()`。

### 3.3 image 整刷无进度打印，擦除+下载 ~60s，易误判卡死
- 症状：`ota image` 后只打印一行 `[image] OTA to storage_b (size=6258688) from ...`，之后**长时间无任何输出**（约 60s），像卡死。
- 根因：`app_ota_upgrade_image()` 流程 = `esp_partition_erase_range`（整片擦 5.97MB flash，~45s，无日志）→ HTTP 流式下载 6.2MB（~15s）→ sha256 校验。期间 `on_data` 回调里的进度 `cb` 为 NULL（console 没传进度回调），故无任何打印。
- 实测时间轴：`I (69889) [image] OTA to storage_b` → `I (130169) sha256 verified`，间隔约 60.3s。
- 改进建议（D-6）：image 升级由 UI 发起时传进度回调 + 在 `erase_range` 前后加日志（`erase %s start/done`），否则用户以为死机。

### 3.4 串口被 SSCOM 占用导致 monitor 打不开
- 症状：`idf.py monitor` 报 `PermissionError(13, '拒绝访问')`，但 `Get-PnpDevice` 显示 COM10 状态 OK。
- 根因：SSCOM 串口助手（进程 `sscom5.13.1`）占着 COM10。
- 解决：关闭 SSCOM 后 monitor 正常。实测前先确认无串口助手占用（`Get-Process sscom*`）。

---

## 4. 待办（后续子阶段）

- [x] D-3：STM32 Bootloader（48KB，sector0~2）+ VTOR + 链接脚本。→ 见 `0X_OTA_D3-D5_STM32主动拉取验证.md`
- [x] D-4：STM32 OTA 参数区（sector3）+ 块烧写 + CRC32。→ 见 `0X_OTA_D3-D5_STM32主动拉取验证.md`
- [x] D-5：ESP32 Modbus 二次传输状态机（v2 Boot 主动拉取 serve：0x41/0x42/0x43）。→ 见 `0X_OTA_D3-D5_STM32主动拉取验证.md`（2026-09-03 真机全链路通过）
- [ ] D-6：OTA 页面 + WiFi 首连自动重试（§3.1）+ 升级完成「重启生效」提示/自动重启（§3.2）+ image 进度显示（§3.3）。