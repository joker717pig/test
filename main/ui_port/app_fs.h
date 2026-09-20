/**
  ******************************************************************************
  * @文件名称   app_fs.h
  * @文件描述   LVGL 'C' 盘 FS 驱动（ui_port 平台移植层）
  *            把同事代码的 Windows 绝对路径（C:/Users/zqt/...）透明翻译到
  *            SPIFFS，并在 open 时整文件读入 RAM → 渲染零文件 I/O。
  * @目标      让同事交付的 ui_page 代码（原装 Windows 路径）不改路径直接可用，
  *            同事更新代码时我们零适配。
  ******************************************************************************
  */

#ifndef APP_FS_H
#define APP_FS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  注册 LVGL 'C' 盘 FS 驱动（路径翻译 + 内存缓存）
  * @note   依赖：SPIFFS 已挂载（app_spiffs_init 先调用）；
  *         LVGL 已初始化（app_lvgl_init 先调用）
  * @retval ESP_OK 成功
  */
esp_err_t app_fs_init(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_FS_H */
