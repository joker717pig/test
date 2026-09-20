/**
  ******************************************************************************
  * @文件名称   app_log.h
  * @文件描述   统一 LOG 封装（阶段 0.7）
  *            应用层统一用 APP_LOGx（映射到 esp_log），便于以后统一重定向/过滤。
  ******************************************************************************
  */

#ifndef APP_LOG_H
#define APP_LOG_H

#include "esp_log.h"

#define APP_LOGE(tag, ...) ESP_LOGE(tag, __VA_ARGS__)
#define APP_LOGW(tag, ...) ESP_LOGW(tag, __VA_ARGS__)
#define APP_LOGI(tag, ...) ESP_LOGI(tag, __VA_ARGS__)
#define APP_LOGD(tag, ...) ESP_LOGD(tag, __VA_ARGS__)
#define APP_LOGV(tag, ...) ESP_LOGV(tag, __VA_ARGS__)

#endif /* APP_LOG_H */
