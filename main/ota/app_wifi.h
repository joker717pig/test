/**
  ******************************************************************************
  * @文件名称   app_wifi.h
  * @文件描述   WiFi STA 连接模块（阶段 D / OTA 联网）
  *            固定热点联网：进入 OTA 页后连接指定手机热点，供 HTTP 下载固件。
  *            - STA 模式（无需 SoftAP，省 flash）
  *            - 状态机：IDLE → CONNECTING → CONNECTED / FAILED
  *            - UI 通过 app_wifi_get_state() 轮询状态，展示连接进度
  ******************************************************************************
  */

#ifndef APP_WIFI_H
#define APP_WIFI_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 固定热点（OTA 升级专用；后续可改 NVS/Kconfig 可配置） */
#define APP_WIFI_SSID           "EDA_EMG_OTA"
#define APP_WIFI_PASS           "12345678"

/* 连接状态 */
typedef enum {
    APP_WIFI_IDLE = 0,      /* 未启动 */
    APP_WIFI_CONNECTING,    /* 连接中 */
    APP_WIFI_CONNECTED,     /* 已连接 */
    APP_WIFI_FAILED,        /* 连接失败 */
} app_wifi_state_t;

/**
  * @brief  初始化 WiFi（esp_netif + esp_wifi + 事件 handler），不立即连接
  * @retval ESP_OK 成功
  */
esp_err_t app_wifi_init(void);

/**
  * @brief  启动连接固定热点（SSID/密码见上方宏）
  * @retval ESP_OK 已发起连接
  */
esp_err_t app_wifi_connect(void);

/**
  * @brief  断开连接（退出 OTA 页时调用，释放无线资源）
  * @retval ESP_OK 成功
  */
esp_err_t app_wifi_disconnect(void);

/**
  * @brief  查询当前连接状态
  */
app_wifi_state_t app_wifi_get_state(void);

/**
  * @brief  获取 IP 地址字符串（未连接返回 "0.0.0.0"）
  * @param  out 输出缓冲（≥16 字节）
  */
void app_wifi_get_ip(char *out, size_t len);

/**
 * @brief  阻塞等待联网成功（供 ota 任务调用）。
 *         内部轮询状态，断连(FAILED)时自动重连，直到 CONNECTED 或超时。
 * @param  timeout_ms 总超时（ms）
 * @retval ESP_OK 已联网；ESP_ERR_TIMEOUT 超时仍未连上
 */
esp_err_t app_wifi_wait_connected(uint32_t timeout_ms);
#endif /* APP_WIFI_H */