/**
  ******************************************************************************
  * @文件名称   app_console.h
  * @文件描述   串口命令 console（USB-Serial-JTAG 交互，0.C 增强）
  *            支持命令：help / stack / mem / psram / part / info
  ******************************************************************************
  */

#ifndef APP_CONSOLE_H
#define APP_CONSOLE_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  启动串口命令任务（从 stdin 读取命令，monitor 中交互）
  * @retval ESP_OK 成功
  */
esp_err_t app_console_start(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_CONSOLE_H */
