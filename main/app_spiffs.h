/**
  ******************************************************************************
  * @文件名称   app_spiffs.h
  * @文件描述   SPIFFS 文件系统挂载与工具 API（阶段 S0）
  *            挂载 storage 分区到 /spiffs，提供查询/遍历接口。
  *            验证命令见 app_console.c 的 spiffs 命令组。
  ******************************************************************************
  */

#ifndef APP_SPIFFS_H
#define APP_SPIFFS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 图片双槽（阶段 D / 决策 D1：图片参与 OTA） */
#define APP_STORAGE_NSPACE      "ota"       /* NVS 命名空间（独立于 app_params 的 "app"） */
#define APP_STORAGE_KEY_SLOT    "imgslot"   /* 键：当前激活图片槽 */
#define APP_STORAGE_SLOT_A      0           /* storage_a */
#define APP_STORAGE_SLOT_B      1           /* storage_b */
#define APP_STORAGE_LABEL_A     "storage_a"
#define APP_STORAGE_LABEL_B     "storage_b"

/**
  * @brief  挂载 SPIFFS 分区（active slot → /spiffs）
  *         按 NVS 的 active_storage 标记选择 storage_a / storage_b。
  * @retval ESP_OK 成功
  */
esp_err_t app_spiffs_init(void);

/**
  * @brief  读取当前激活的图片槽（0=storage_a / 1=storage_b）
  */
uint8_t app_spiffs_get_active_slot(void);

/**
  * @brief  切换激活图片槽（写 NVS 标记，重启后生效；OTA 整刷非激活槽后调用）
  * @param  slot APP_STORAGE_SLOT_A / APP_STORAGE_SLOT_B
  * @retval ESP_OK 成功
  */
esp_err_t app_spiffs_set_active_slot(uint8_t slot);

/**
  * @brief  打印 SPIFFS 分区信息（总空间 / 已用空间）
  */
void app_spiffs_print_info(void);

/**
  * @brief  递归列出 /spiffs 下所有文件（含子目录）
  * @param  path 起始路径（NULL 或 "" 表示 /spiffs 根目录）
  */
void app_spiffs_list(const char *path);

/**
  * @brief  读取并 hex-dump 文件前 N 字节
  * @param  path    文件路径（如 "/spiffs/font/lv_font_Btn.bin"）
  * @param  offset  起始偏移（字节）
  * @param  len     读取长度（字节），0 表示最多 256
  */
void app_spiffs_cat(const char *path, size_t offset, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* APP_SPIFFS_H */
