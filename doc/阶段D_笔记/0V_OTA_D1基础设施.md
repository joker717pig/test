# 0V OTA D-1 基础设施层实施笔记

> 阶段 D 子阶段 D-1：ESP32 分区表改造 + WiFi/HTTP 栈接入 + OTA 框架。
> 交叉引用：计划文档 `doc/阶段D_笔记/0U_OTA在线升级计划.md`（§9 阶段拆分）。
> 状态：**已实现 + 编译通过 + 上板验证通过**（2026-09-01：wifi 连网 + ota manifest 拉取跑通）。
> 更新（2026-09）：云端为 **HTTP 明文**（dgbuysmart），OTA 已从 esp_https_ota 改造为 esp_ota_* 流式写入。

---

## 1. 已完成清单

| 文件 | 内容 | 状态 |
| --- | --- | --- |
| `partitions.csv` | OTA 双区布局（nvs/phy_init/otadata/ota_0/ota_1/storage_a/storage_b） | ✅ |
| `CMakeLists.txt`（根） | `spiffs_create_partition_image(storage_a/b)` 双镜像 | ✅ |
| `main/app_spiffs.{h,c}` | 按 NVS `imgslot` 选槽挂载 + `get/set_active_slot` | ✅ |
| `main/common/mem_map.h` | otadata/storage_a/storage_b 偏移常量（保留 factory 历史） | ✅ |
| `sdkconfig.defaults` | SPIFFS 双分区 + SoftAP 关 + HTTPS/bundle + 回滚使能 | ✅ |
| `main/ota/app_wifi.{h,c}` | WiFi STA 连接（固定热点 `EDA_EMG_OTA`/`12345678`） | ✅ |
| `main/ota/app_ota.{h,c}` | version.json 拉取 + esp_ota_* 流式 OTA + 图片整刷 + STM32 下载 + **独立 ota 任务(16KB 栈)** | ✅ |
| `main/CMakeLists.txt` | SRC_DIRS/INCLUDE_DIRS 加 `ota`；REQUIRES 补 wifi/netif/http/ota | ✅ |
| `main/app_main.c` | `app_console_start()` 后启动 `app_ota_task_start()` | ✅ |
| `main/console/app_console.c` | 加 `wifi` / `ota` 命令（运行验证用） | ✅ |

编译产物：`eda_emg_esp32.bin` ~1.03MB→**1.69MB**（HTTP 改造后 0x19ef80），`storage_a.bin`/`storage_b.bin` 各 6258688B。

---

## 2. 分区表（最终布局）

```
nvs        0x9000    0x5000   16K
phy_init   0xd000    0x1000    4K
otadata    0xe000    0x2000    8K    ← OTA 必需（滚动双区状态）
ota_0      0x10000   0x20000   2MB   ← app 槽 A
ota_1      0x210000  0x20000   2MB   ← app 槽 B
storage_a  0x410000  0x5F8000  5.97M ← 图片槽 A
storage_b  0xA08000  0x5F8000  5.97M ← 图片槽 B
```

要点：storage 从原来的 12MB 单区改为两个 5.97MB 双区；图片实际 4.26MB，余量足够（SPIFFS 开销内）。

---

## 3. 关键实现决策 / 坑

### 3.1 WiFi 不随上电初始化
- `app_wifi_start()` 有功耗开销，且 OTA 是 UI 动作触发。**不**在 `app_main` 里初始化 WiFi。
- 由「进入 OTA 页 → `app_wifi_connect()`」触发（D-6 页面阶段接入）。
- 运行验证用 console 的 `wifi connect` 手动触发。

### 3.2 必须先 `esp_netif_create_default_wifi_sta()`
- ESP-IDF v5 中不创建默认 STA netif 时，`esp_wifi_connect()` 会因无接口绑定而失败。
- 顺序：`esp_netif_init()` → `esp_netif_create_default_wifi_sta()` → 注册事件 → `esp_wifi_init` → `set_mode(STA)` → `set_ps(NONE)` → `esp_wifi_start`。

### 3.3 云端为 HTTP 明文 → 改用 esp_ota_* 手写 OTA（更新）
- 用户提供真实云端 `http://www.dgbuysmart.com/fota/lzq3496900/eda_emg/`，是 **HTTP 非 HTTPS**。
- `esp_https_ota` 依赖 TLS 证书握手，对纯 HTTP 地址不适用 → 改为：
  `esp_http_client`（流式 `HTTP_EVENT_ON_DATA` 回调）→ `esp_ota_write()` → `esp_ota_end()` → `esp_ota_set_boot_partition()` → `esp_restart()`（由调用方）。
- 移除 `#include "esp_https_ota.h"` / `#include "esp_crt_bundle.h"`，改 `#include "esp_ota_ops.h"`。
- 移除所有 `crt_bundle_attach` / `skip_cert_common_name_check` 字段（HTTP 无 TLS）。
- `main/CMakeLists.txt` REQUIRES 移除 `esp_https_ota`（`app_update` 已含 `esp_ota_ops`）。
- 顺序：`esp_ota_get_next_update_partition(NULL)` 取下一槽 → `esp_ota_begin(part, OTA_SIZE_UNKNOWN)` → HTTP 流式 `esp_ota_write` 累加 sha256 → 下载完校验 sha256 → `esp_ota_end` → `esp_ota_set_boot_partition`。
- 关键坑：HTTP 下载失败/HTTP 状态非 200/sha256 不匹配时，必须调 `esp_ota_abort(handle)` 释放句柄，否则残留内存。

### 3.3b（原 3.3 证书方案已弃用）
- ~~`esp_http_client_config_t.crt_bundle_attach = esp_crt_bundle_attach` + `skip_cert_common_name_check = true`~~（仅 HTTPS 需要，已移除）。

### 3.4 ~~esp_https_ota 用 `MALLOC_CAP_SPIRAM` 缓冲~~（已弃用）
- 改造后不再用 `esp_https_ota_config_t`；esp_http_client 的 `buffer_size=8192` 由 IDF 调小缓冲，流式分发到 `HTTP_EVENT_ON_DATA`。

### 3.5 图片 OTA = 「整刷非激活槽」策略
- 目标槽 = `app_spiffs_get_active_slot()` 的另一半（A↔B）。
- 流程：`esp_partition_erase_range` 整片擦 → HTTP 流式写入 + sha256 累加 → 校验 → `app_spiffs_set_active_slot(新槽)`。
- 断电安全：只动非激活槽，激活槽始终完好，失败重下即可。
- sha256 校验**必须**在 `mbedtls_sha256_finish()` 之后才 `mbedtls_sha256_free()`（早期版本曾提前 free 导致校验读到已释放上下文）。

### 3.6 STM32 固件先整包落盘
- 下载到 `/spiffs/fw/stm32_fw.bin`，二次 Modbus 下发在 D-5 子阶段实现（避免断网变砖）。

### 3.7 `%u` 格式化 uint32_t 会触发 `-Werror=format=`
- 32 位目标上 `uint32_t` = `unsigned long`，`printf` 必须用 `%lu` + `(unsigned long)` 强转。
- 否则 `-Werror=format=` 直接编译失败。

### 3.8 WiFi `esp_wifi_init` 报 `ESP_ERR_NO_MEM`（内部 RAM 不足）
- 症状：`wifi init` 时 `esp_wifi_init failed: ESP_ERR_NO_MEM`，内部 RAM 只剩 ~36KB，装不下 WiFi 静态+DMA 缓冲。
- 根因：带 PSRAM 时 `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL` 默认过大会把 LVGL 小对象也塞进内部 RAM，挤爆 WiFi 需要的内部 DMA 缓冲。
- 修复（`sdkconfig` + `sdkconfig.defaults` 同步）：
  - `CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=2048`（≤2048B 才用内部 RAM，大对象走 PSRAM）
  - `CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=65536`（专门给 WiFi 保留 64KB 内部 RAM）
- 效果：内部 RAM 36KB→**131KB**，`esp_wifi_init` OK。

### 3.9 `ota manifest` 触发栈溢出 abort（console 4KB 栈不够 → 独立 OTA 任务）
- 症状：`ota manifest` 直接 abort，backtrace 落到 `vApplicationStackOverflowHook`（`app_debug.c:26`）。
- 根因链：console 任务只有 **4KB** 栈，同步跑 `fetch_json`（栈上 `char[1024]` + `ota_target_info_t[3]` ≈741B + cJSON 递归解析 + `_vfprintf_r` 格式化缓冲）把栈打穿；栈溢出 hook 里再 `ESP_LOGE` 触发二次崩溃 → `abort()`。
- **方案（治本，非临时改大 console 栈）**：新增**独立 `ota` 任务（16KB 栈）**，HTTP 下载/JSON 解析/sha256/写 flash 全在 ota 任务里跑；console 的 `ota` 命令只 `app_ota_request()` 派发一条队列消息（非阻塞）即返回，队列天然串行化（一次只跑一个 OTA）。
- 涉及：`app_ota.{h,c}` 加 `ota_cmd_t` 枚举 + `app_ota_task_start()` + `app_ota_request()`；`app_main.c` 在 console 后启动任务；console 栈保持 4096。
- 验证：`ota manifest` 跑通，三目标 version/size/sha256/url 全部正确打印，无栈溢出。

---

## 4. 运行验证命令（console）

> ✅ **2026-09-01 上板验证通过**：`wifi init`（内存修正后 init OK）→ `wifi connect`（拿到 `192.168.43.17`）→ `ota manifest`（三目标 version/size/sha256/url 全正确），全程无栈溢出。独立 ota 任务日志 `app_ota: ota task started (stack=16384)`。

上板后在 monitor 里执行：

```
> wifi init         # 初始化 WiFi（netif + wifi + 事件）
> wifi connect      # 连接固定热点（需先开手机热点 EDA_EMG_OTA / 12345678）
> wifi state        # 查连接状态（CONNECTED=2）
> wifi ip           # 查拿到的 IP

> ota manifest      # 拉取 version.json 并打印三目标
> ota esp32         # 升 ESP32 固件（HTTP + esp_ota_* 流式）
> ota image         # 升图片存储（整刷非激活槽）
> ota stm32         # 下载 STM32 固件到 /spiffs/fw
```

验证前提：
1. 手机开热点 `EDA_EMG_OTA` / `12345678`（WiFi 2.4G；ESP32-S3 不支持 5G 热点）。
2. 云端地址已改为 `http://www.dgbuysmart.com/fota/lzq3496900/eda_emg/`（`app_ota.h` 里 `APP_OTA_BASE_URL`）。
   - 需上传 `version.json` 与对应固件到该路径，见 §5。

---

## 5. version.json 格式（云端）

```json
{
  "targets": {
    "esp32": { "version": "0.2.0", "url": "esp32/eda_emg_esp32.bin", "sha256": "<hex>", "size": 1699712 },
    "image": { "version": "0.1.1", "url": "image/storage_a.bin", "sha256": "<hex>", "size": 6258688 },
    "stm32": { "version": "0.0.5", "url": "stm32/fw.bin", "sha256": "<hex>", "size": 49833 }
  }
}
```

- **三个目标版本号各自独立**（esp32/image/stm32 分别判断升级），无顶层统一 `version` 字段。
- `url` 为相对路径时自动拼 `APP_OTA_BASE_URL` 前缀；绝对 URL 直接使用。
- `sha256`：三个目标均用 sha256（脚本 `build_ota_package.py` 统一计算）；
  stm32 的二次校验（D-5 Modbus 下发时）再单独打通。
- 生成：`py tools/build_ota_package.py --esp32-ver 0.2.0 --image-ver 0.1.1 --stm32-ver 0.0.5`。

---

## 6. 待办（后续子阶段）

- [ ] D-2：云端真实 `version.json` + PC 模拟服务，本地验证下载/校验/双区切换。
- [ ] D-3：STM32 Bootloader（8KB）+ VTOR + 链接脚本。
- [ ] D-4：STM32 OTA 寄存器 + 块烧写 + CRC。
- [ ] D-5：ESP32 Modbus 二次传输状态机。
- [ ] D-6：OTA 页面（中/英）+ 版本号打通 + `app_main` 集成 WiFi/OTA 触发。