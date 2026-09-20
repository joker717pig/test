/**
  ******************************************************************************
  * @文件名称   app_debug.h
  * @文件描述   调试设施（阶段 0.7）：栈溢出 hook + 栈水位监控任务
  ******************************************************************************
  */

#ifndef APP_DEBUG_H
#define APP_DEBUG_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  打印所有任务的栈高水位（由串口命令 stack 触发）
  */
void app_debug_print_stack_watermarks(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_DEBUG_H */
