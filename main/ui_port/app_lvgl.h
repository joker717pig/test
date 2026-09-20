/**
  ******************************************************************************
  * @文件名称   app_lvgl.h
  * @文件描述   LVGL 初始化（阶段 1.5，经 esp_lvgl_port 接入 ST7365）
  *            全帧双缓冲 PSRAM（300KB×2）+ panel 内部 SRAM DMA 中转
  ******************************************************************************
  */

#ifndef APP_LVGL_H
#define APP_LVGL_H

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  初始化 LVGL：lvgl_port_init（创建渲染任务+tick）+ lvgl_port_add_disp（接入 app_lcd）
  * @note   需在 app_lcd_init() 之后调用
  * @retval ESP_OK 成功；其他见 esp_err_t
  */
esp_err_t app_lvgl_init(void);

/**
  * @brief  获取 LVGL display 句柄
  * @retval lv_display_t*；未初始化返回 NULL
  */
lv_display_t *app_lvgl_get_disp(void);

/**
 * @brief  进入"内部 flash 擦写安全区"（OTA 图片/ESP32 升级擦写 flash 前调用）
 *
 *         擦写内部 flash 会反复 spi_flash_disable_interrupts_caches_and_other_cpu：
 *         关 cache、屏蔽非 IRAM 中断并停住另一核。而 LCD 的 SPI 完成回调链
 *         （esp_lcd post_cb / esp_lvgl_port flush_ready 回调）都不在 IRAM，
 *         且 SPI LCD 无 IRAM 安全配置；若此刻有在途 flush/DMA，其完成中断会被
 *         延迟并与 SPI 驱动"中断惰性使能"状态机竞争，导致 LCD SPI 队列永久停摆、
 *         LVGL 刷新冻死。
 *
 *         本函数：1) 取 LVGL 锁（等 LVGL 任务退出当前渲染，持锁期间它不再发起
 *         新 flush）；2) 排空 LCD SPI 总线上所有在途颜色事务（等 DMA 全部完成、
 *         flush_ready 已投递）。返回后关 cache 窗口里无任何在飞 LCD 传输。
 *         必须配对 app_lvgl_end_flash_op()。期间屏幕保持最后一帧静态。
 */
void app_lvgl_begin_flash_op(void);

/**
 * @brief  退出"内部 flash 擦写安全区"，释放 LVGL 锁恢复渲染
 */
void app_lvgl_end_flash_op(void);

/**
 * @brief  在"flash 擦写安全区"内临时放行 LVGL 刷新一帧（OTA 长擦写/下载时维持进度条动画）
 *
 *         必须在 app_lvgl_begin_flash_op() 之后、app_lvgl_end_flash_op() 之前调用。
 *         动作：解锁 → 让出 CPU 给 LVGL 任务跑一轮 lv_timer_handler（读 OTA 状态、
 *         重画进度条/百分比、把 flush 排入 SPI DMA）→ 重新取锁 → app_lcd_drain()
 *         排空在途 DMA。返回后仍处于安全区（锁持有、总线空闲），可继续擦/写 flash。
 *
 *         关键：末尾的 drain 保证返回时总线上无在飞 LCD 传输，因此调用方随后
 *         擦/写 flash（关 cache、屏蔽非 IRAM 中断）不会丢失 LCD 完成通知，不破坏
 *         冻死修复。可在长擦写循环里按 ~200ms 节流反复调用。
 */
void app_lvgl_pump_flash_ui(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_LVGL_H */

