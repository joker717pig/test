/**
  ******************************************************************************
  * @文件名称   app_debug.c
  * @文件描述   调试设施实现（阶段 0.7）：
  *            - vApplicationStackOverflowHook：栈溢出 hook（CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY 触发）
  *            - stack_monitor 任务：定期用 uxTaskGetSystemState 打印所有任务栈高水位
  ******************************************************************************
  */

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "app_debug.h"

static const char *TAG = "app_debug";

/**
  * @brief  栈溢出 hook（FreeRTOS 调用；需开启 CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY）
  */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    ESP_LOGE(TAG, "STACK OVERFLOW detected! Task: %s", pcTaskName ? pcTaskName : "(null)");
    abort();
}

/**
  * @brief  打印所有任务的栈高水位（字节，由串口命令 stack 触发，不自动定时）
  * @note   ESP-IDF 中任务栈以字节计，TaskStatus_t.usStackHighWaterMark 单位即字节
  */
void app_debug_print_stack_watermarks(void)
{
    UBaseType_t task_count = uxTaskGetNumberOfTasks();
    TaskStatus_t *status = calloc(task_count, sizeof(TaskStatus_t));
    if (status == NULL) {
        ESP_LOGE(TAG, "calloc(%u tasks) failed", (unsigned)task_count);
        return;
    }

    UBaseType_t n = uxTaskGetSystemState(status, task_count, NULL);
    printf("-- task stack high-water marks (%u tasks) --\r\n", (unsigned)n);
    for (UBaseType_t i = 0; i < n; i++) {
        printf("  %-16s %u bytes\r\n",
               status[i].pcTaskName, (unsigned)status[i].usStackHighWaterMark);
    }
    free(status);
}
