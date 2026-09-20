/**
  ******************************************************************************
  * @文件名称   app_ota.c
  * @文件描述   OTA 在线升级模块实现（阶段 D）
  *            1) HTTP 拉取 version.json 解析三目标版本清单
  *            2) ESP32：HTTP 流式下载 + esp_ota_* 写入 + 双区切换（完成后重启）
  *            3) 图片：流式下载 → 整刷非激活 storage 槽 → 写 NVS 标记
  *            4) STM32：下载到 /spiffs/fw/，二次 Modbus 下发后续子阶段实现
  *            云端为 HTTP（非 HTTPS），不使用证书/加密。
  ******************************************************************************
  */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "cJSON.h"
#include "mbedtls/sha256.h"
#include "app_ota.h"
#include "app_spiffs.h"
#include "app_modbus.h"
#include "app_modbus_reg.h"
#include "app_wifi.h"
#include "app_lvgl.h"   /* app_lvgl_begin/end_flash_op：擦写内部 flash 前静默 LCD */

static const char *TAG = "app_ota";

/* ============================================================================
 * 【测试开关】OTA_TEST_FORCE_UPGRADE = 1 时，跳过版本比对，只要云端清单拉到
 * 就把三目标(STM32/图片/ESP32)全部标记"有更新"，强制走完整升级（联调用）。
 * = 0 时走正式版本比较：仅当云端版本 > 本地版本(app_ota_ver_compare>0)才升级。
 * 2026-09-04 全链路上板联调通过(含 LCD flush 异步修复)，转正式版本比较，置 0。
 * 版本号口径(2026-09-05 起)：云端 version.json 由 tools/build_ota_package.py 自动从
 *     固件提取(ESP32 APP_FW_VERSION / STM32 FW_VERSION / 图片 image_ver.txt)，与设备
 *     自报版本严格一致；要发布新版时 bump 对应固件版本号后重新打包上传，设备据此
 *     判"云端 > 本地"才升级，升级后版本相等不再提示。
 * ==========================================================================*/
#define OTA_TEST_FORCE_UPGRADE  0

/* ============================================================================
 * UI 全局状态（ota 任务单写、UI 任务单读；短字符串/整型，轮询无需锁）
 * ==========================================================================*/
static ota_ui_state_t g_ota_ui;

const ota_ui_state_t *app_ota_ui_get_state(void)
{
    return &g_ota_ui;
}

void app_ota_ui_reset(void)
{
    memset(&g_ota_ui, 0, sizeof(g_ota_ui));
    g_ota_ui.phase = OTA_UI_IDLE;
}

/* 读图片槽自带版本文件 /spiffs/image_ver.txt（内容 "主.次"）。无文件返回 false。 */
static bool read_image_version(char *out, size_t len)
{
    FILE *fp = fopen("/spiffs/image_ver.txt", "r");
    if (fp == NULL) return false;
    char buf[16] = {0};
    bool ok = false;
    if (fgets(buf, sizeof(buf), fp) != NULL) {
        char *nl = strpbrk(buf, "\r\n");
        if (nl) *nl = '\0';
        if (buf[0] != '\0') { strlcpy(out, buf, len); ok = true; }
    }
    fclose(fp);
    return ok;
}

/* 读三目标本地版本（ESP32 宏 / STM32 FC03 0x0302 / 图片版本文件），填入 g_ota_ui */
static void read_local_versions(ota_ui_state_t *st)
{
    /* ESP32：编译期 16 位版本 */
    app_ota_ver_bcd_to_str(APP_FW_VERSION, st->local_ver[OTA_TARGET_ESP32],
                           sizeof(st->local_ver[OTA_TARGET_ESP32]));
    st->ver_known[OTA_TARGET_ESP32] = true;

    /* STM32：FC03 读 0x0302（进页面立即读，不等 WiFi） */
    uint16_t vals[1];
    if (app_modbus_read_registers(MB_REG_FW_VERSION, 1, vals) == MB_OK) {
        app_ota_ver_bcd_to_str(vals[0], st->local_ver[OTA_TARGET_STM32],
                               sizeof(st->local_ver[OTA_TARGET_STM32]));
        st->ver_known[OTA_TARGET_STM32] = true;
        ESP_LOGI(TAG, "[check] stm32 local ver = 0x%04X (%s)",
                 (unsigned)vals[0], st->local_ver[OTA_TARGET_STM32]);
    } else {
        strlcpy(st->local_ver[OTA_TARGET_STM32], "?.?",
                sizeof(st->local_ver[OTA_TARGET_STM32]));
        st->ver_known[OTA_TARGET_STM32] = false;
        ESP_LOGW(TAG, "[check] read stm32 version failed (offline?)");
    }

    /* 图片：storage 槽自带版本文件 */
    if (read_image_version(st->local_ver[OTA_TARGET_IMAGE],
                           sizeof(st->local_ver[OTA_TARGET_IMAGE]))) {
        st->ver_known[OTA_TARGET_IMAGE] = true;
    } else {
        strlcpy(st->local_ver[OTA_TARGET_IMAGE], "?.?",
                sizeof(st->local_ver[OTA_TARGET_IMAGE]));
        st->ver_known[OTA_TARGET_IMAGE] = false;
    }
}

/* ---- 通用下载上下文（流式 + 进度 + sha256） ---- */
typedef struct {
    ota_target_t            target;    /* 进度归属目标 */
    ota_progress_cb_t       cb;        /* 进度回调 */
    uint32_t                total;     /* 内容总长（0 未知/分块） */
    uint32_t                read;      /* 已读 */
    uint32_t                last_pump_ms;  /* 上次放行 LVGL 刷新时间戳（节流） */
    uint32_t                last_log_ms;   /* 上次串口打印百分比时间戳（节流） */
    const char             *name;          /* 日志目标名 "esp32"/"image"/"stm32" */
    mbedtls_sha256_context  sha;       /* sha256 累加 */
} ota_dl_ctx_t;

/* 下载进度上报（节流）：更新 UI 状态 + 持锁期间周期放行 LVGL 刷一帧进度条 + 串口打印百分比。
 * 在 begin/end_flash_op 之间的 HTTP 下载事件里调用：app_lvgl_pump_flash_ui 临时解锁让
 * LVGL 任务重画进度条，再重新取锁并 drain 排空 DMA，保证随后擦/写 flash 的关 cache
 * 窗口里无在飞 LCD 传输（不破坏冻死修复）。STM32 拉块段(40~100%)在解锁后跑，不走这里。 */
#define OTA_DL_UI_PUMP_MS   200     /* 进度条放行刷新节流 */
#define OTA_DL_LOG_MS       500     /* 串口百分比打印节流 */
static void ota_dl_progress(ota_dl_ctx_t *c, int pct)
{
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    if (c->cb) c->cb(c->target, pct);
    uint32_t now = esp_log_timestamp();
    if ((int32_t)(now - c->last_pump_ms) >= OTA_DL_UI_PUMP_MS) {
        c->last_pump_ms = now;
        app_lvgl_pump_flash_ui();
    }
    if ((int32_t)(now - c->last_log_ms) >= OTA_DL_LOG_MS) {
        c->last_log_ms = now;
        ESP_LOGI(TAG, "[%s] 下载 %d%% (%u/%u B)", c->name ? c->name : "ota",
                 pct, (unsigned)c->read, (unsigned)c->total);
    }
}

static void sha256_hex(const unsigned char hash[32], char out[65])
{
    for (int i = 0; i < 32; i++) {
        snprintf(&out[i * 2], 3, "%02x", hash[i]);
    }
    out[64] = '\0';
}

/* ----- 拼接相对 URL：info->url 为相对路径时叠加 base ----- */
static void resolve_url(const char *in, char *out, size_t out_len)
{
    if (strncmp(in, "http://", 7) == 0 || strncmp(in, "https://", 8) == 0) {
        strlcpy(out, in, out_len);
    } else {
        snprintf(out, out_len, "%s/%s", APP_OTA_BASE_URL, in);
    }
}

/* ============================================================================
 * 版本号工具（两位 主.次；与下位机 16 位 0xMMmm 同口径）
 * ==========================================================================*/

/* 解析 "主.次[.patch]" 的前两段为整数；非法段按 0 */
static void ver_parse_mm(const char *s, int *major, int *minor)
{
    *major = 0;
    *minor = 0;
    if (s == NULL) return;
    while (*s == ' ') s++;
    *major = atoi(s);
    const char *dot = strchr(s, '.');
    if (dot) *minor = atoi(dot + 1);
}

int app_ota_ver_compare(const char *a, const char *b)
{
    int amaj, amin, bmaj, bmin;
    ver_parse_mm(a, &amaj, &amin);
    ver_parse_mm(b, &bmaj, &bmin);
    if (amaj != bmaj) return (amaj > bmaj) ? 1 : -1;
    if (amin != bmin) return (amin > bmin) ? 1 : -1;
    return 0;
}

void app_ota_ver_bcd_to_str(uint16_t bcd, char *out, size_t len)
{
    if (out == NULL || len == 0) return;
    snprintf(out, len, "%d.%d", (int)(bcd >> 8), (int)(bcd & 0xFF));
}

/* ============================================================================
 * 版本清单拉取
 * ==========================================================================*/

esp_err_t app_ota_init(void)
{
    /* 无全局状态；HTTP 明文，无需证书配置 */
    return ESP_OK;
}

/* 内存累积下载（用于 version.json 小文件） */
typedef struct {
    char  *buf;
    size_t len;
    size_t cap;
} mem_accum_t;

static esp_err_t mem_accum_append(mem_accum_t *a, const char *data, int len)
{
    if (a->len + (size_t)len + 1 > a->cap) {
        size_t ncap = (a->cap == 0) ? 4096 : a->cap;
        while (ncap < a->len + (size_t)len + 1) ncap *= 2;
        char *nb = realloc(a->buf, ncap);
        if (nb == NULL) return ESP_ERR_NO_MEM;
        a->buf = nb;
        a->cap = ncap;
    }
    memcpy(a->buf + a->len, data, len);
    a->len += (size_t)len;
    a->buf[a->len] = '\0';
    return ESP_OK;
}

static esp_err_t fetch_json(const char *url, char **out_buf, size_t *out_len)
{
    mem_accum_t acc = { 0 };

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 10000,
        .buffer_size = 2048,
        .event_handler = NULL,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) { free(acc.buf); return ESP_FAIL; }

    /* 用 esp_http_client_open + read 循环读取正文 */
    esp_err_t ret = esp_http_client_open(client, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "open %s failed: %s", url, esp_err_to_name(ret));
        esp_http_client_cleanup(client);
        free(acc.buf);
        return ret;
    }

    int content_len = esp_http_client_fetch_headers(client);
    if (content_len < 0) content_len = 0;

    char tmp[1024];
    int r;
    while ((r = esp_http_client_read(client, tmp, sizeof(tmp))) > 0) {
        mem_accum_append(&acc, tmp, r);
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (acc.buf == NULL || acc.len == 0) {
        free(acc.buf);
        ESP_LOGE(TAG, "empty response from %s", url);
        return ESP_FAIL;
    }

    *out_buf = acc.buf;
    *out_len = acc.len;
    return ESP_OK;
}

/* 解析 version.json：
 * {
 *   "version": "0.1.0",
 *   "targets": {
 *     "esp32": {"version":"0.1.0","url":"...","sha256":"...","size":123},
 *     "image": {...},
 *     "stm32": {...}
 *   }
 * }
 */
static esp_err_t parse_manifest(const char *json, ota_target_info_t out[OTA_TARGET_COUNT])
{
    cJSON *root = cJSON_Parse(json);
    if (root == NULL) {
        ESP_LOGE(TAG, "version.json parse error: %s", cJSON_GetErrorPtr());
        return ESP_FAIL;
    }

    static const char *target_keys[OTA_TARGET_COUNT] = { "esp32", "image", "stm32" };
    cJSON *targets = cJSON_GetObjectItemCaseSensitive(root, "targets");

    for (int i = 0; i < OTA_TARGET_COUNT; i++) {
        ota_target_info_t *o = &out[i];
        memset(o, 0, sizeof(*o));
        strcpy(o->version, "0.0.0");
        o->url[0] = '\0';
        o->sha256[0] = '\0';
        o->size = 0;

        cJSON *t = NULL;
        if (targets && cJSON_IsObject(targets)) {
            t = cJSON_GetObjectItemCaseSensitive(targets, target_keys[i]);
        }
        if (t == NULL || !cJSON_IsObject(t)) continue;

        cJSON *v = cJSON_GetObjectItemCaseSensitive(t, "version");
        if (cJSON_IsString(v) && v->valuestring) strlcpy(o->version, v->valuestring, sizeof(o->version));

        v = cJSON_GetObjectItemCaseSensitive(t, "url");
        if (cJSON_IsString(v) && v->valuestring) strlcpy(o->url, v->valuestring, sizeof(o->url));

        v = cJSON_GetObjectItemCaseSensitive(t, (i == OTA_TARGET_STM32) ? "crc32" : "sha256");
        if (cJSON_IsString(v) && v->valuestring) strlcpy(o->sha256, v->valuestring, sizeof(o->sha256));

        v = cJSON_GetObjectItemCaseSensitive(t, "size");
        if (cJSON_IsNumber(v)) o->size = (uint32_t)v->valuedouble;
    }

    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t app_ota_fetch_manifest(ota_target_info_t out[OTA_TARGET_COUNT])
{
    char url[200];
    strlcpy(url, APP_OTA_MANIFEST_URL, sizeof(url));

    char *json = NULL;
    size_t len = 0;
    esp_err_t ret = fetch_json(url, &json, &len);
    if (ret != ESP_OK) return ret;

    ret = parse_manifest(json, out);
    free(json);
    return ret;
}

/* ============================================================================
 * ESP32 固件升级（HTTP 流式下载 + esp_ota_* 手动写入）
 * ==========================================================================*/

typedef struct {
    ota_dl_ctx_t       common;
    esp_ota_handle_t   handle;
    const esp_partition_t *part;
} esp32_writer_t;

static esp_err_t esp32_http_event(esp_http_client_event_t *evt)
{
    esp32_writer_t *w = (esp32_writer_t *)evt->user_data;
    switch (evt->event_id) {
        case HTTP_EVENT_ON_CONNECTED: {
            int cl = (int)esp_http_client_get_content_length(evt->client);
            /* 仅当服务器返回有效 Content-Length(>0) 才覆盖；否则保留 init 播种的清单 info->size，
             * 避免 chunked/无头时 total=0(或 -1 变超大) 导致进度条不动、最后直接跳 100 */
            if (cl > 0) w->common.total = (uint32_t)cl;
            break;
        }
        case HTTP_EVENT_ON_DATA:
            mbedtls_sha256_update(&w->common.sha, (const unsigned char *)evt->data, evt->data_len);
            esp_err_t e = esp_ota_write(w->handle, evt->data, evt->data_len);
            if (e != ESP_OK) {
                ESP_LOGE(TAG, "[esp32] ota write failed: %s", esp_err_to_name(e));
                return e;
            }
            w->common.read += evt->data_len;
            if (w->common.total) {
                ota_dl_progress(&w->common, (int)(w->common.read * 100 / w->common.total));
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t app_ota_upgrade_esp32(const ota_target_info_t *info, ota_progress_cb_t cb)
{
    char url[200];
    resolve_url(info->url, url, sizeof(url));

    ESP_LOGI(TAG, "[esp32] OTA from %s (size=%u, sha=%s)", url, info->size, info->sha256);

    /* 目标 = 下一个 OTA 槽 */
    const esp_partition_t *part = esp_ota_get_next_update_partition(NULL);
    if (part == NULL) {
        ESP_LOGE(TAG, "[esp32] no OTA update partition");
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "[esp32] writing to partition %s (subtype %d, addr 0x%lx)",
             part->label, part->subtype, (unsigned long)part->address);

    esp32_writer_t w = { 0 };
    w.part = part;
    w.common.target = OTA_TARGET_ESP32;
    w.common.cb = cb;
    w.common.name = "esp32";
    w.common.total = info->size;   /* 清单已知大小兜底：服务器不给 Content-Length 时进度条仍能走 */
    mbedtls_sha256_init(&w.common.sha);
    mbedtls_sha256_starts(&w.common.sha, 0);

    /* 擦写内部 flash 前进入安全区：esp_ota_write 增量擦/写 ota 槽、set_boot_partition
     * 写 otadata 都在关 cache 窗口。下载期间 ota_dl_progress 会周期 pump 放行 LVGL 刷
     * 进度条（pump 末尾 drain 保证擦写瞬间总线空闲）；ESP32 是升级最后一项，完成后即重启。 */
    app_lvgl_begin_flash_op();

    /* 用 OTA_WITH_SEQUENTIAL_WRITES：esp_ota_write 时按扇区增量擦（不在 begin 整片擦
     * 2MB），把擦除摊到下载过程，配合 ota_dl_progress 周期放行 LVGL，进度条可平滑走。 */
    esp_err_t ret = esp_ota_begin(part, OTA_WITH_SEQUENTIAL_WRITES, &w.handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[esp32] esp_ota_begin failed: %s", esp_err_to_name(ret));
        mbedtls_sha256_free(&w.common.sha);
        app_lvgl_end_flash_op();
        return ret;
    }

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 15000,
        .buffer_size = 8192,
        .event_handler = esp32_http_event,
        .user_data = &w,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        esp_ota_abort(w.handle);
        mbedtls_sha256_free(&w.common.sha);
        app_lvgl_end_flash_op();
        return ESP_FAIL;
    }

    ret = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (ret != ESP_OK) {
        esp_ota_abort(w.handle);
        mbedtls_sha256_free(&w.common.sha);
        app_lvgl_end_flash_op();
        ESP_LOGE(TAG, "[esp32] download failed: %s", esp_err_to_name(ret));
        return ret;
    }
    if (status != 200) {
        esp_ota_abort(w.handle);
        mbedtls_sha256_free(&w.common.sha);
        app_lvgl_end_flash_op();
        ESP_LOGE(TAG, "[esp32] HTTP status %d", status);
        return ESP_FAIL;
    }

    /* 校验 sha256（manifest 提供了才校验） */
    if (info->sha256[0] != '\0') {
        unsigned char hash[32];
        mbedtls_sha256_finish(&w.common.sha, hash);
        char hex[65];
        sha256_hex(hash, hex);
        if (strcasecmp(hex, info->sha256) != 0) {
            ESP_LOGE(TAG, "[esp32] sha256 mismatch:\n  got %s\n  exp %s", hex, info->sha256);
            mbedtls_sha256_free(&w.common.sha);
            esp_ota_abort(w.handle);
            app_lvgl_end_flash_op();
            return ESP_ERR_INVALID_CRC;
        }
        ESP_LOGI(TAG, "[esp32] sha256 verified: %s", hex);
    }
    mbedtls_sha256_free(&w.common.sha);

    ret = esp_ota_end(w.handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[esp32] esp_ota_end failed: %s", esp_err_to_name(ret));
        app_lvgl_end_flash_op();
        return ret;
    }

    ret = esp_ota_set_boot_partition(part);
    app_lvgl_end_flash_op();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[esp32] set boot partition failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "[esp32] OTA success, wait reboot (调用方重启)");
    return ESP_OK;
}

/* ============================================================================
 * 图片升级：整刷非激活 storage 槽
 * ==========================================================================*/

/* 流式下载并顺序写入目标分区 + 同时计算 sha256 */
typedef struct {
    ota_dl_ctx_t           common;
    const esp_partition_t *part;
    size_t                 off;      /* 已写偏移 */
    size_t                 erased;   /* 已增量擦除到的偏移（按扇区对齐） */
} image_writer_t;

static esp_err_t image_http_event(esp_http_client_event_t *evt)
{
    image_writer_t *w = (image_writer_t *)evt->user_data;
    switch (evt->event_id) {
        case HTTP_EVENT_ON_CONNECTED: {
            int cl = (int)esp_http_client_get_content_length(evt->client);
            if (cl > 0) w->common.total = (uint32_t)cl;  /* 有效头才覆盖，否则用清单 info->size 兜底 */
            break;
        }
        case HTTP_EVENT_ON_DATA:
            mbedtls_sha256_update(&w->common.sha, (const unsigned char *)evt->data, evt->data_len);
            if (w->part) {
                /* 按需增量擦除：只擦本块覆盖到、尚未擦的扇区（替代下载前整片擦 6MB），
                 * 把长擦除摊到下载过程，配合 ota_dl_progress 周期放行 LVGL，进度条可动。 */
                size_t es = w->part->erase_size;
                size_t need_end = w->off + evt->data_len;
                size_t erase_end = (need_end + es - 1) / es * es;
                if (erase_end > w->part->size) erase_end = w->part->size;
                if (erase_end > w->erased) {
                    esp_err_t er = esp_partition_erase_range(w->part, w->erased, erase_end - w->erased);
                    if (er != ESP_OK) {
                        ESP_LOGE(TAG, "partition erase failed: %s", esp_err_to_name(er));
                        return er;
                    }
                    w->erased = erase_end;
                }
                esp_err_t e = esp_partition_write(w->part, w->off, evt->data, evt->data_len);
                if (e != ESP_OK) {
                    ESP_LOGE(TAG, "partition write failed: %s", esp_err_to_name(e));
                    return e;
                }
                w->off += evt->data_len;
            }
            w->common.read += evt->data_len;
            if (w->common.total) {
                ota_dl_progress(&w->common, (int)(w->common.read * 100 / w->common.total));
            }
            break;
        case HTTP_EVENT_ON_FINISH:
        case HTTP_EVENT_DISCONNECTED:
            break;
        default:
            break;
    }
    return ESP_OK;
}

esp_err_t app_ota_upgrade_image(const ota_target_info_t *info, ota_progress_cb_t cb)
{
    /* 目标 = 非激活槽 */
    uint8_t active = app_spiffs_get_active_slot();
    const char *label = (active == APP_STORAGE_SLOT_A) ? APP_STORAGE_LABEL_B : APP_STORAGE_LABEL_A;
    const esp_partition_t *part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, label);
    if (part == NULL) {
        ESP_LOGE(TAG, "target storage partition '%s' not found", label);
        return ESP_ERR_NOT_FOUND;
    }

    char url[200];
    resolve_url(info->url, url, sizeof(url));

    ESP_LOGI(TAG, "[image] OTA to %s (size=%u) from %s", label, info->size, url);

    image_writer_t w = { 0 };
    w.part = part;
    w.common.target = OTA_TARGET_IMAGE;
    w.common.cb = cb;
    w.common.name = "image";
    w.common.total = info->size;   /* 清单已知大小兜底：服务器不给 Content-Length 时进度条仍能走 */
    mbedtls_sha256_init(&w.common.sha);
    mbedtls_sha256_starts(&w.common.sha, 0);

    /* 擦写内部 flash 前进入安全区：增量擦/写 storage 槽 + NVS 切槽都在关 cache 窗口内，
     * 必须先停 LVGL 刷新并排空在途 SPI DMA，否则 LCD 完成中断（非 IRAM）被屏蔽会
     * 导致刷新永久冻死。下载期间 ota_dl_progress 周期 pump 放行 LVGL 刷进度条（pump
     * 末尾 drain 保证擦写瞬间总线空闲），故进度条不再定死。 */
    app_lvgl_begin_flash_op();

    /* 不下载前整片擦除 6MB（会长时间持锁、进度条定死）；改为下载写入时按扇区增量
     * 擦除（见 image_http_event），擦除摊到下载过程，配合 ota_dl_progress 周期放行 LVGL。 */
    esp_err_t ret;

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 15000,
        .buffer_size = 8192,
        .event_handler = image_http_event,
        .user_data = &w,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        mbedtls_sha256_free(&w.common.sha);
        app_lvgl_end_flash_op();
        return ESP_FAIL;
    }

    ret = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (ret != ESP_OK) {
        mbedtls_sha256_free(&w.common.sha);
        app_lvgl_end_flash_op();
        ESP_LOGE(TAG, "[image] download failed: %s", esp_err_to_name(ret));
        return ret;
    }
    if (status != 200) {
        mbedtls_sha256_free(&w.common.sha);
        app_lvgl_end_flash_op();
        ESP_LOGE(TAG, "[image] HTTP status %d", status);
        return ESP_FAIL;
    }

    /* 校验 sha256（manifest 提供了才校验） */
    esp_err_t verify = ESP_OK;
    if (info->sha256[0] != '\0') {
        unsigned char hash[32];
        mbedtls_sha256_finish(&w.common.sha, hash);
        char hex[65];
        sha256_hex(hash, hex);
        if (strcasecmp(hex, info->sha256) != 0) {
            ESP_LOGE(TAG, "[image] sha256 mismatch:\n  got %s\n  exp %s", hex, info->sha256);
            verify = ESP_ERR_INVALID_CRC;
        } else {
            ESP_LOGI(TAG, "[image] sha256 verified: %s", hex);
        }
    }
    mbedtls_sha256_free(&w.common.sha);
    if (verify != ESP_OK) {
        app_lvgl_end_flash_op();
        return verify;
    }

    /* 写 NVS 标记切到新槽（重启后生效，由调用方重启；NVS 也写内部 flash，仍在安全区内） */
    ret = app_spiffs_set_active_slot((active == APP_STORAGE_SLOT_A) ? APP_STORAGE_SLOT_B : APP_STORAGE_SLOT_A);
    app_lvgl_end_flash_op();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[image] set active slot failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "[image] upgrade OK, reboot to switch slot");
    return ESP_OK;
}

/* ============================================================================
 * STM32 固件升级：下载到 /spiffs/fw/stm32_fw.bin
 * ==========================================================================*/

typedef struct {
    ota_dl_ctx_t common;
    FILE        *fp;
} stm32_writer_t;

static esp_err_t stm32_http_event(esp_http_client_event_t *evt)
{
    stm32_writer_t *w = (stm32_writer_t *)evt->user_data;
    switch (evt->event_id) {
        case HTTP_EVENT_ON_CONNECTED: {
            int cl = (int)esp_http_client_get_content_length(evt->client);
            if (cl > 0) w->common.total = (uint32_t)cl;  /* 有效头才覆盖，否则用清单 info->size 兜底 */
            break;
        }
        case HTTP_EVENT_ON_DATA:
            mbedtls_sha256_update(&w->common.sha, (const unsigned char *)evt->data, evt->data_len);
            if (w->fp) fwrite(evt->data, 1, evt->data_len, w->fp);
            w->common.read += evt->data_len;
            if (w->common.total) {
                /* HTTP 下载段占整体 0~40%（Boot 拉块段 40~100% 在 serve 等待循环里报） */
                ota_dl_progress(&w->common, (int)(w->common.read * 40 / w->common.total));
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

/* ============================================================================
 * STM32 固件升级: 下载到 spiffs → 读入内存 → 写 size/crc 给 APP → 0x020C 触发
 * → Boot 主动拉取(REQ/DATA 帧, 由 app_modbus 服务) → 等 DONE 帧 → 收尾
 * ==========================================================================*/

esp_err_t app_ota_upgrade_stm32(const ota_target_info_t *info, ota_progress_cb_t cb)
{
    char url[200];
    resolve_url(info->url, url, sizeof(url));

    /* 确保 /spiffs/fw 目录存在 */
    struct stat st_dir;
    if (stat("/spiffs/fw", &st_dir) != 0) {
        mkdir("/spiffs/fw", 0755);
    }

    ESP_LOGI(TAG, "[stm32] download from %s (size=%u)", url, info->size);

    stm32_writer_t w = { 0 };

    /* 下载段 fwrite 落盘到 SPIFFS（内部 flash），同样有擦写关 cache 窗口，进入前静默
     * LCD（下载几秒内 0~40% 进度条静态）。后续 UART 拉块段(40~100%)不碰内部 flash，
     * 在 end 之后执行，进度动画照常。 */
    app_lvgl_begin_flash_op();

    w.fp = fopen("/spiffs/fw/stm32_fw.bin", "wb");
    if (w.fp == NULL) {
        ESP_LOGE(TAG, "[stm32] open /spiffs/fw/stm32_fw.bin failed");
        app_lvgl_end_flash_op();
        return ESP_FAIL;
    }
    w.common.target = OTA_TARGET_STM32;
    w.common.cb = cb;
    w.common.name = "stm32";
    w.common.total = info->size;   /* 清单已知大小兜底：服务器不给 Content-Length 时下载段进度仍能走 */
    mbedtls_sha256_init(&w.common.sha);
    mbedtls_sha256_starts(&w.common.sha, 0);

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 15000,
        .buffer_size = 8192,
        .event_handler = stm32_http_event,
        .user_data = &w,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        fclose(w.fp);
        mbedtls_sha256_free(&w.common.sha);
        app_lvgl_end_flash_op();
        return ESP_FAIL;
    }

    esp_err_t ret = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    fflush(w.fp);
    fclose(w.fp);
    mbedtls_sha256_free(&w.common.sha);
    app_lvgl_end_flash_op();   /* 下载落盘结束，恢复 LCD；之后读回/malloc/UART 拉块均不擦内部 flash */

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "[stm32] download failed: %s", esp_err_to_name(ret));
        return ret;
    }
    if (status != 200) {
        ESP_LOGE(TAG, "[stm32] HTTP status %d", status);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "[stm32] firmware saved to /spiffs/fw/stm32_fw.bin (%u B)", w.common.read);

    /* 读入内存 (Boot 拉取服务直接从内存取块, 避免每块 fopen/fseek) */
    uint8_t *fw;
    FILE *fp = fopen("/spiffs/fw/stm32_fw.bin", "rb");
    if (fp == NULL) {
        ESP_LOGE(TAG, "[stm32] reopen fw failed");
        return ESP_FAIL;
    }
    fw = malloc(info->size);
    if (fw == NULL) {
        ESP_LOGE(TAG, "[stm32] malloc %u failed", info->size);
        fclose(fp);
        return ESP_ERR_NO_MEM;
    }
    if (fread(fw, 1, info->size, fp) != info->size) {
        ESP_LOGE(TAG, "[stm32] read fw failed");
        fclose(fp);
        free(fw);
        return ESP_FAIL;
    }
    fclose(fp);

    uint32_t fw_crc = (info->sha256[0] != '\0') ? (uint32_t)strtoul(info->sha256, NULL, 16) : 0u;

    /* 写 OTA 参数到 APP (0x0313~0x0317), 再 0x020C 触发跳 Boot */
    app_modbus_stop_push();
    int e = app_modbus_write_register(MB_REG_OTA_TARGET, 1);
    e |= app_modbus_write_register(MB_REG_OTA_FW_SIZE_H, (uint16_t)(info->size >> 16));
    e |= app_modbus_write_register(MB_REG_OTA_FW_SIZE_L, (uint16_t)(info->size & 0xFFFF));
    e |= app_modbus_write_register(MB_REG_OTA_FW_CRC_H, (uint16_t)(fw_crc >> 16));
    e |= app_modbus_write_register(MB_REG_OTA_FW_CRC_L, (uint16_t)(fw_crc & 0xFFFF));
    if (e != MB_OK) {
        ESP_LOGE(TAG, "[stm32] write OTA params failed: %s", app_modbus_err_str(e));
        free(fw);
        return ESP_FAIL;
    }

    e = app_modbus_write_register(MB_REG_OTA_INIT, 1);
    if (e != MB_OK) {
        ESP_LOGE(TAG, "[stm32] OTA_INIT failed: %s", app_modbus_err_str(e));
        free(fw);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "[stm32] OTA triggered, serve mode on (fw=%u B)", info->size);

    /* Boot 主动拉取服务: 响应 REQ_BLOCK, 直到收到 DONE 帧或超时 */
    app_modbus_ota_serve_start(fw, info->size);
    if (cb) cb(OTA_TARGET_STM32, 40);   /* 下载完成，进入拉块段起点 */

    uint32_t t0 = esp_log_timestamp();
    while (1) {
        int done = app_modbus_ota_done_status();
        /* 拉块段进度映射到 40~100% */
        if (cb && info->size) {
            uint32_t served = app_modbus_ota_progress();
            int pct = 40 + (int)(served * 60 / info->size);
            if (pct > 99) pct = 99;     /* 未收到 DONE 前不报满 100 */
            cb(OTA_TARGET_STM32, pct);
        }
        if (done == 0) {
            ESP_LOGI(TAG, "[stm32] OTA success (Boot 校验通过, 已复位跳新 APP)");
            if (cb) cb(OTA_TARGET_STM32, 100);
            app_modbus_ota_serve_stop();
            free(fw);
            return ESP_OK;
        }
        if (done == 1) {
            ESP_LOGE(TAG, "[stm32] OTA failed (Boot 校验失败)");
            app_modbus_ota_serve_stop();
            free(fw);
            return ESP_FAIL;
        }
        if ((esp_log_timestamp() - t0) > 120000u) {   /* 120s 总超时 */
            ESP_LOGE(TAG, "[stm32] OTA timeout");
            app_modbus_ota_serve_stop();
            free(fw);
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ============================================================================
 * 独立 OTA 任务（阶段 D）
 * console 只做异步派发；HTTP 下载 / JSON 解析 / sha256 / 写 flash 全在此任务
 * 中执行。栈 16KB（char[1024] 读缓冲 + ota_target_info[3] + cJSON 递归 + _vfprintf_r
 * 格式化均在此，16KB 足够）。之前同步跑在 console(4KB) 里触发栈溢出 abort。
 * ==========================================================================*/

#define OTA_TASK_STACK_BYTES   (16 * 1024)
#define OTA_TASK_PRIO          4

#define WIFI_CONNECT_TIMEOUT_MS   15000   /* 进 OTA 页联网总超时 */

static QueueHandle_t s_ota_queue = NULL;   /* ota_cmd_t 消息队列 */

/* BEGIN_CHECK 拉到的清单缓存，供 UPGRADE_ALL 直接复用（避免升级时再拉一次） */
static ota_target_info_t s_cached_manifest[OTA_TARGET_COUNT];
static bool              s_manifest_valid = false;

/* 把 WiFi 当前状态/IP 同步到 UI 全局状态 */
static void sync_wifi_to_ui(ota_ui_state_t *st)
{
    st->wifi_state = (int)app_wifi_get_state();
    app_wifi_get_ip(st->ip, sizeof(st->ip));
}

/* 升级进度回调（HTTP 事件 / STM32 serve 循环在 ota 任务上下文调用）→ 写全局状态 */
static void ota_ui_progress_cb(ota_target_t target, int percent)
{
    if (target < 0 || target >= OTA_TARGET_COUNT) return;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    g_ota_ui.progress[target] = percent;
}

/* 执行异步命令（在 OTA 任务上下文中） */
static void ota_task_exec(ota_cmd_t cmd)
{
    ota_target_info_t m[OTA_TARGET_COUNT];

    switch (cmd) {
        case OTA_CMD_BEGIN_CHECK: {
            ota_ui_state_t *st = &g_ota_ui;
            memset(st, 0, sizeof(*st));
            st->phase = OTA_UI_CHECKING;
            s_manifest_valid = false;

            /* 1) 立即读本地三版本（STM32 FC03 不等 WiFi） */
            read_local_versions(st);

            /* 2) 开 WiFi 并等待联网（内部自动重连） */
            app_wifi_init();
            app_wifi_connect();
            sync_wifi_to_ui(st);
            esp_err_t wr = app_wifi_wait_connected(WIFI_CONNECT_TIMEOUT_MS);
            sync_wifi_to_ui(st);
            if (wr != ESP_OK) {
                ESP_LOGW(TAG, "[check] wifi connect failed");
                st->phase = OTA_UI_WIFI_FAIL;
                break;
            }
            ESP_LOGI(TAG, "[check] wifi connected, ip=%s", st->ip);

            /* 3) 拉云端清单 */
            esp_err_t e = app_ota_fetch_manifest(s_cached_manifest);
            if (e != ESP_OK) {
                ESP_LOGW(TAG, "[check] manifest fetch failed: %s", esp_err_to_name(e));
                st->phase = OTA_UI_MANIFEST_FAIL;
                break;
            }
            s_manifest_valid = true;

            /* 4) 填云端版本 + 比对 */
            static const ota_target_t order[OTA_TARGET_COUNT] = {
                OTA_TARGET_STM32, OTA_TARGET_IMAGE, OTA_TARGET_ESP32 };
            for (int k = 0; k < OTA_TARGET_COUNT; k++) {
                ota_target_t t = order[k];
                strlcpy(st->cloud_ver[t], s_cached_manifest[t].version,
                        sizeof(st->cloud_ver[t]));
#if OTA_TEST_FORCE_UPGRADE
                /* 测试模式：不看版本号，清单里有该目标(URL非空)就强制升级 */
                if (s_cached_manifest[t].url[0] != '\0') {
                    st->has_update[t] = true;
                }
                ESP_LOGW(TAG, "[check] TEST FORCE: %d local=%s cloud=%s update=%d",
                         (int)t, st->local_ver[t], st->cloud_ver[t],
                         (int)st->has_update[t]);
#else
                if (st->ver_known[t] &&
                    app_ota_ver_compare(s_cached_manifest[t].version,
                                        st->local_ver[t]) > 0) {
                    st->has_update[t] = true;
                }
                ESP_LOGI(TAG, "[check] %d: local=%s cloud=%s update=%d",
                         (int)t, st->local_ver[t], st->cloud_ver[t],
                         (int)st->has_update[t]);
#endif
            }
            st->phase = OTA_UI_READY;
            break;
        }
        case OTA_CMD_UPGRADE_ALL: {
            ota_ui_state_t *st = &g_ota_ui;

            /* 只允许从 READY 态启动：防快速双击导致命令排队，在上一轮跑完(DONE)后重复升级 */
            if (st->phase != OTA_UI_READY) {
                ESP_LOGW(TAG, "[upgrade] ignore UPGRADE_ALL in phase %d (only READY allowed)",
                         (int)st->phase);
                break;
            }

            /* 清单无效（异常直接点升级）→ 退回检查态 */
            if (!s_manifest_valid) {
                ESP_LOGW(TAG, "[upgrade] no cached manifest, re-check");
                app_ota_request(OTA_CMD_BEGIN_CHECK);
                break;
            }

            /* 进入升级态：清进度/失败，两按钮由 UI 置"升级中…"禁用 */
            st->phase = OTA_UI_UPGRADING;
            for (int i = 0; i < OTA_TARGET_COUNT; i++) {
                st->progress[i] = 0;
                st->fail[i] = false;
            }
            st->all_ok = false;

            /* 升级顺序：STM32 → 图片 → ESP32。每项独立 try，失败只标红不中断后续。 */
            static const ota_target_t order[OTA_TARGET_COUNT] = {
                OTA_TARGET_STM32, OTA_TARGET_IMAGE, OTA_TARGET_ESP32 };

            for (int k = 0; k < OTA_TARGET_COUNT; k++) {
                ota_target_t t = order[k];

                /* 无更新项跳过（进度保持 0，不标红） */
                if (!st->has_update[t]) {
                    ESP_LOGI(TAG, "[upgrade] target %d no update, skip", (int)t);
                    continue;
                }
                if (s_cached_manifest[t].url[0] == '\0') {
                    ESP_LOGE(TAG, "[upgrade] target %d url empty, mark fail", (int)t);
                    st->fail[t] = true;
                    continue;
                }

                esp_err_t e;
                switch (t) {
                case OTA_TARGET_STM32:
                    e = app_ota_upgrade_stm32(&s_cached_manifest[t], ota_ui_progress_cb);
                    break;
                case OTA_TARGET_IMAGE:
                    e = app_ota_upgrade_image(&s_cached_manifest[t], ota_ui_progress_cb);
                    break;
                default:
                    e = app_ota_upgrade_esp32(&s_cached_manifest[t], ota_ui_progress_cb);
                    break;
                }

                if (e == ESP_OK) {
                    st->progress[t] = 100;
                    st->fail[t] = false;
                    ESP_LOGI(TAG, "[upgrade] target %d OK", (int)t);
                } else {
                    st->fail[t] = true;
                    ESP_LOGE(TAG, "[upgrade] target %d FAILED: %s (continue next)",
                             (int)t, esp_err_to_name(e));
                }
            }

            /* 汇总：三项无失败才算全成功（含无更新跳过的项） */
            bool any_fail = false;
            for (int i = 0; i < OTA_TARGET_COUNT; i++)
                if (st->fail[i]) any_fail = true;
            st->all_ok = !any_fail;
            st->phase = OTA_UI_DONE;
            ESP_LOGI(TAG, "[upgrade] all done, all_ok=%d", (int)st->all_ok);
            break;
        }
        case OTA_CMD_FETCH_MANIFEST: {
            esp_err_t e = app_ota_fetch_manifest(m);
            if (e != ESP_OK) {
                ESP_LOGE(TAG, "manifest fetch failed: %s", esp_err_to_name(e));
                return;
            }
            static const char *names[OTA_TARGET_COUNT] = { "esp32", "image", "stm32" };
            ESP_LOGI(TAG, "=== version.json (3 targets) ===");
            for (int i = 0; i < OTA_TARGET_COUNT; i++) {
                ESP_LOGI(TAG, "  [%s] ver=%s size=%lu sha=%s",
                         names[i], m[i].version, (unsigned long)m[i].size, m[i].sha256);
                ESP_LOGI(TAG, "       url=%s", m[i].url);
            }
            break;
        }
        case OTA_CMD_UPGRADE_ESP32: {
            if (app_ota_fetch_manifest(m) != ESP_OK) {
                ESP_LOGE(TAG, "manifest fetch failed, abort upgrade");
                return;
            }
            if (m[OTA_TARGET_ESP32].url[0] == '\0') { ESP_LOGE(TAG, "esp32 url empty in manifest"); return; }
            esp_err_t e = app_ota_upgrade_esp32(&m[OTA_TARGET_ESP32], NULL);
            ESP_LOGI(TAG, "esp32 upgrade -> %s", e == ESP_OK ? "OK (boot partition switched, reboot to apply)" : esp_err_to_name(e));
            break;
        }
        case OTA_CMD_UPGRADE_IMAGE: {
            if (app_ota_fetch_manifest(m) != ESP_OK) {
                ESP_LOGE(TAG, "manifest fetch failed, abort upgrade");
                return;
            }
            if (m[OTA_TARGET_IMAGE].url[0] == '\0') { ESP_LOGE(TAG, "image url empty in manifest"); return; }
            esp_err_t e = app_ota_upgrade_image(&m[OTA_TARGET_IMAGE], NULL);
            ESP_LOGI(TAG, "image upgrade -> %s", e == ESP_OK ? "OK (active slot flipped, reboot to apply)" : esp_err_to_name(e));
            break;
        }
        case OTA_CMD_UPGRADE_STM32: {
            if (app_ota_fetch_manifest(m) != ESP_OK) {
                ESP_LOGE(TAG, "manifest fetch failed, abort upgrade");
                return;
            }
            if (m[OTA_TARGET_STM32].url[0] == '\0') { ESP_LOGE(TAG, "stm32 url empty in manifest"); return; }
            esp_err_t e = app_ota_upgrade_stm32(&m[OTA_TARGET_STM32], NULL);
            ESP_LOGI(TAG, "stm32 download -> %s", e == ESP_OK ? "OK (saved to /spiffs/fw/stm32_fw.bin)" : esp_err_to_name(e));
            break;
        }
        default:
            ESP_LOGW(TAG, "unknown ota cmd %d", (int)cmd);
            break;
    }
}

static void ota_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "ota task started (stack=%d)", OTA_TASK_STACK_BYTES);

    ota_cmd_t cmd;
    while (1) {
        /* 阻塞等待命令；ota_task_exec 返回前不会接收下一条（天然串行化） */
        if (xQueueReceive(s_ota_queue, &cmd, portMAX_DELAY) == pdTRUE) {
            ota_task_exec(cmd);
        }
    }
}

esp_err_t app_ota_task_start(void)
{
    if (s_ota_queue != NULL) return ESP_OK;  /* 已启动 */

    s_ota_queue = xQueueCreate(4, sizeof(ota_cmd_t));
    if (s_ota_queue == NULL) {
        ESP_LOGE(TAG, "create ota cmd queue failed");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t r = xTaskCreate(ota_task, "ota", OTA_TASK_STACK_BYTES, NULL, OTA_TASK_PRIO, NULL);
    if (r != pdPASS) {
        vQueueDelete(s_ota_queue);
        s_ota_queue = NULL;
        ESP_LOGE(TAG, "create ota task failed");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t app_ota_request(ota_cmd_t cmd)
{
    if (s_ota_queue == NULL) {
        ESP_LOGW(TAG, "ota task not started, call app_ota_task_start() first");
        return ESP_ERR_INVALID_STATE;
    }
    if (xQueueSend(s_ota_queue, &cmd, 0) != pdTRUE) {
        ESP_LOGW(TAG, "ota busy (queue full), reject cmd %d", (int)cmd);
        return ESP_ERR_INVALID_STATE;
    }
    return ESP_OK;
}