/**
  ******************************************************************************
  * @文件名称   app_ota.h
  * @文件描述   OTA 在线升级模块（阶段 D）
  *            负责：获取云端版本清单(version.json) → 比较版本 → 触发下载/升级。
  *            - ESP32 固件：esp_https_ota 整包下载 → 双区切换 → 重启
  *            - 图片资源：下载 storage 镜像 → 整刷非激活槽 → 写 NVS 标记
  *            - STM32 固件：下载到 storage → 二次 Modbus 分块下发（后续子阶段）
  ******************************************************************************
  */

#ifndef APP_OTA_H
#define APP_OTA_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 本地固件版本（16 位 0xMMmm：高字节=主版本, 低字节=次版本；0x0200 = v2.0）。
 * 与云端 version.json 的两位 "主.次" 字符串同口径（第三位 patch 忽略）。
 * 注意：改版本时同步 main/idf_component.yml 的 version 字段。 */
#define APP_FW_VERSION          0x0204  /* v2.4：修复阶段切换强度滞后(mA 与 UI 不同步)；强度口径 0~90mA 原值直传 */

/* 云端版本清单 URL（正式云端：health.eda-global.cn 公网 HTTP，非 HTTPS）
 * 2026-09-07 由 dgbuysmart 测试地址迁移到正式地址。 */
#define APP_OTA_BASE_URL        "http://health.eda-global.cn/pelvicFloorMuscleUpgradeKit/fota/eda_emg"
#define APP_OTA_MANIFEST_URL    "http://health.eda-global.cn/pelvicFloorMuscleUpgradeKit/fota/eda_emg/version.json"

/* 版本清单（云端 version.json 的单目标条目） */
typedef struct {
    char    version[16];   /* "0.1.0" */
    char    url[160];      /* 固件下载地址（相对或绝对） */
    char    sha256[65];    /* 校验和（app/image 用 sha256；stm32 用 crc32 填这里） */
    uint32_t size;         /* 字节数 */
} ota_target_info_t;

/* 升级目标类型 */
typedef enum {
    OTA_TARGET_ESP32 = 0,
    OTA_TARGET_IMAGE,       /* 图片 storage 镜像 */
    OTA_TARGET_STM32,
    OTA_TARGET_COUNT
} ota_target_t;

/* 升级进度回调 */
typedef void (*ota_progress_cb_t)(ota_target_t target, int progress_percent);

/* OTA 异步命令（console / UI → ota 任务队列） */
typedef enum {
    OTA_CMD_FETCH_MANIFEST = 0,   /* 拉取 version.json 并打印三目标清单 */
    OTA_CMD_UPGRADE_ESP32,        /* 升级 ESP32 固件 */
    OTA_CMD_UPGRADE_IMAGE,        /* 升级图片 storage 槽 */
    OTA_CMD_UPGRADE_STM32,        /* 下载 STM32 固件到 spiffs */
    OTA_CMD_BEGIN_CHECK,          /* [UI] 进 OTA 页：读本地版本 + 连 WiFi + 拉清单比对 */
    OTA_CMD_UPGRADE_ALL,          /* [UI] 点"开始升级"：顺序 STM32→图片→ESP32 */
} ota_cmd_t;

/* OTA UI 阶段（ota 任务写 g_ota_ui.phase，UI 轮询刷新） */
typedef enum {
    OTA_UI_IDLE = 0,        /* 未开始检查 */
    OTA_UI_CHECKING,        /* 检查中（读本地版本 / 连 WiFi / 拉清单） */
    OTA_UI_WIFI_FAIL,       /* WiFi 连不上 */
    OTA_UI_MANIFEST_FAIL,   /* WiFi 通但拉 version.json 失败 */
    OTA_UI_READY,           /* 检查完成（看 has_update[]） */
    OTA_UI_UPGRADING,       /* 升级中（看 progress[] / fail[]） */
    OTA_UI_DONE,            /* 三项全部结束（看 all_ok / fail[]） */
} ota_ui_phase_t;

/* OTA UI 全局状态：ota 任务单写、UI 任务单读（短字符串/整型，轮询无需锁）。
 * UI 侧用 lv_timer(~150ms) 调 app_ota_ui_get_state() 刷新，切勿在 ota 任务调 lv_*。 */
typedef struct {
    ota_ui_phase_t phase;
    int      wifi_state;                 /* app_wifi_state_t 原值 */
    char     ip[16];
    char     local_ver[OTA_TARGET_COUNT][8];   /* 本地版本 "主.次" */
    char     cloud_ver[OTA_TARGET_COUNT][8];   /* 云端版本 "主.次" */
    bool     ver_known[OTA_TARGET_COUNT];     /* 本地版本是否成功读到 */
    bool     has_update[OTA_TARGET_COUNT];    /* 云端 > 本地 */
    int      progress[OTA_TARGET_COUNT];      /* 0~100 */
    bool     fail[OTA_TARGET_COUNT];          /* 该目标升级失败（进度条标红） */
    bool     all_ok;                          /* phase==DONE 且三项无失败 */
} ota_ui_state_t;

/* 获取 UI 全局状态（只读指针，永久有效） */
const ota_ui_state_t *app_ota_ui_get_state(void);

/* 复位 UI 状态到 IDLE（进 OTA 页时调用，清上轮残留） */
void app_ota_ui_reset(void);

/**
  * @brief  初始化 OTA 模块（注册事件/准备 HTTP 会话）
  * @retval ESP_OK 成功
  */
esp_err_t app_ota_init(void);

/**
  * @brief  启动独立 OTA 任务（16KB 栈）。
  *         HTTP 下载/JSON 解析/sha256/写 flash 都是重活，必须在独立任务中执行，
  *         不能同步跑在 console 任务(4KB) 里，否则栈溢出（阶段 D 踩坑）。
  * @retval ESP_OK 成功
  */
esp_err_t app_ota_task_start(void);

/**
  * @brief  向 OTA 任务投递一个异步命令（非阻塞）。
  *         队列满（有命令在跑）返回 ESP_ERR_INVALID_STATE。
  * @param  cmd  要执行的命令
  * @retval ESP_OK 已受理；失败表示 OTA 忙或未初始化
  */
esp_err_t app_ota_request(ota_cmd_t cmd);

/**
  * @brief  获取云端版本清单并解析为三目标条目
  * @param  out    输出数组（长度 OTA_TARGET_COUNT）
  * @retval ESP_OK 成功
  */
esp_err_t app_ota_fetch_manifest(ota_target_info_t out[OTA_TARGET_COUNT]);

/**
 * @brief  比较两个两位版本字符串（"主.次"，如 "1.0" / "2.3"）。
 *         容错：第三位 patch（"1.0.5"）忽略；非数字段按 0 处理。
 * @retval 负数 a<b；0 相等；正数 a>b
 */
int app_ota_ver_compare(const char *a, const char *b);

/**
 * @brief  把 16 位 BCD 版本（0xMMmm，高字节主/低字节次）转成 "主.次" 字符串。
 *         例：0x0100 -> "1.0"，0x0203 -> "2.3"。
 * @param  bcd  16 位版本
 * @param  out  输出缓冲（≥8 字节）
 * @param  len  缓冲长度
 */
void app_ota_ver_bcd_to_str(uint16_t bcd, char *out, size_t len);

/**
  * @brief  执行 ESP32 固件升级（esp_https_ota 整包 + 双区切换后重启）
  * @param  info   目标信息（含 url + sha256）
  * @param  cb     进度回调（可空）
  * @retval ESP_OK 成功（实际会重启，不返回）；失败返回错误码
  */
esp_err_t app_ota_upgrade_esp32(const ota_target_info_t *info, ota_progress_cb_t cb);

/**
  * @brief  执行图片升级（整刷非激活 storage 槽 + 写 NVS 标记，不自动重启）
  * @param  info   目标信息（url + sha256）
  * @param  cb     进度回调（可空）
  * @retval ESP_OK 成功（需调用方重启切换）
  */
esp_err_t app_ota_upgrade_image(const ota_target_info_t *info, ota_progress_cb_t cb);

/**
  * @brief  执行 STM32 固件升级（下载到 storage + 二次 Modbus 下发）
  *         本子阶段仅下载到 storage，Modbus 下发在后续子阶段实现。
  * @param  info   目标信息（url + crc32）
  * @param  cb     进度回调（可空）
  * @retval ESP_OK 成功
  */
esp_err_t app_ota_upgrade_stm32(const ota_target_info_t *info, ota_progress_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif /* APP_OTA_H */