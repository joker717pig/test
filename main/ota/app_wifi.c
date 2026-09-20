/**
  ******************************************************************************
  * @文件名称   app_wifi.c
  * @文件描述   WiFi STA 连接模块（阶段 D / OTA 联网）
  *            固定热点联网：进入 OTA 页后连接指定手机热点，供 HTTP 下载固件。
  *            状态机基于 esp_event 的 WIFI_EVENT / IP_EVENT。
  ******************************************************************************
  */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "app_wifi.h"

static const char *TAG = "app_wifi";

static app_wifi_state_t s_state = APP_WIFI_IDLE;
static esp_netif_t *s_sta_netif = NULL;
static bool s_inited = false;

/* WiFi 连接结果事件位（用于同步等待/状态跟踪） */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_state = APP_WIFI_FAILED;
        ESP_LOGW(TAG, "STA disconnected");
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        s_state = APP_WIFI_CONNECTED;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
    }
}

esp_err_t app_wifi_init(void)
{
    if (s_inited) return ESP_OK;

    /* 默认事件循环已由 app_event_loop_init 创建，直接注册 */
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) { ESP_LOGE(TAG, "esp_netif_init failed: %s", esp_err_to_name(ret)); return ret; }

    /* 创建默认 STA netif（v5 必需，否则 esp_wifi_connect 无接口可绑定） */
    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (s_sta_netif == NULL) {
        ESP_LOGE(TAG, "create default wifi sta failed");
        return ESP_FAIL;
    }

    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "register WIFI_EVENT failed: %s", esp_err_to_name(ret)); return ret; }
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "register IP_EVENT failed: %s", esp_err_to_name(ret)); return ret; }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "esp_wifi_init failed: %s", esp_err_to_name(ret)); return ret; }

    /* STA 模式 + 省电关闭（下载期间要稳定吞吐） */
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "set mode STA failed: %s", esp_err_to_name(ret)); return ret; }
    esp_wifi_set_ps(WIFI_PS_NONE);

    ret = esp_wifi_start();
    if (ret != ESP_OK) { ESP_LOGE(TAG, "esp_wifi_start failed: %s", esp_err_to_name(ret)); return ret; }

    s_inited = true;
    ESP_LOGI(TAG, "WiFi STA init OK");
    return ESP_OK;
}

esp_err_t app_wifi_connect(void)
{
    if (!s_inited) {
        esp_err_t r = app_wifi_init();
        if (r != ESP_OK) return r;
    }

    wifi_config_t cfg = { 0 };
    memcpy(cfg.sta.ssid, APP_WIFI_SSID, sizeof(APP_WIFI_SSID));
    memcpy(cfg.sta.password, APP_WIFI_PASS, sizeof(APP_WIFI_PASS));
    cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &cfg);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "set STA config failed: %s", esp_err_to_name(ret)); return ret; }

    s_state = APP_WIFI_CONNECTING;
    ESP_LOGI(TAG, "connecting to SSID '%s' ...", APP_WIFI_SSID);

    ret = esp_wifi_connect();
    if (ret != ESP_OK) { ESP_LOGE(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(ret)); return ret; }
    return ESP_OK;
}

esp_err_t app_wifi_disconnect(void)
{
    if (!s_inited) return ESP_OK;
    esp_wifi_disconnect();
    s_state = APP_WIFI_IDLE;
    return ESP_OK;
}

app_wifi_state_t app_wifi_get_state(void)
{
    return s_state;
}

void app_wifi_get_ip(char *out, size_t len)
{
    if (out == NULL || len == 0) return;
    if (s_state != APP_WIFI_CONNECTED) {
        strlcpy(out, "0.0.0.0", len);
        return;
    }
    esp_netif_ip_info_t ip;
    if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip) == ESP_OK) {
        snprintf(out, len, IPSTR, IP2STR(&ip.ip));
    } else {
        strlcpy(out, "0.0.0.0", len);
    }
}

esp_err_t app_wifi_wait_connected(uint32_t timeout_ms)
{
    uint32_t t0 = (uint32_t)(esp_log_timestamp());
    while (1) {
        app_wifi_state_t st = s_state;
        if (st == APP_WIFI_CONNECTED) return ESP_OK;
        /* 断连后自动重连（STA_START 也会触发一次 connect，这里兜底重试） */
        if (st == APP_WIFI_FAILED || st == APP_WIFI_IDLE) {
            app_wifi_connect();
        }
        if ((uint32_t)(esp_log_timestamp()) - t0 > timeout_ms) {
            ESP_LOGW(TAG, "wait connected timeout (%lu ms)", (unsigned long)timeout_ms);
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}