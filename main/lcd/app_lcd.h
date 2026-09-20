/**
  ******************************************************************************
  * @文件名称   app_lcd.h
  * @文件描述   ST7365 显示驱动（阶段 0.D，LVGL-agnostic）
  *            基于 esp_lcd：自定义 panel 实现 esp_lcd_panel_t，
  *            与 LVGL 完全解耦，将来任意版本 LVGL 可直接接。
  * @说明       像素格式 RGB565；字节序 MSB first（大端）。
  *            逻辑分辨率横屏 480x320（MADCTL=0xE8 硬件横屏）。
  ******************************************************************************
  */

#ifndef APP_LCD_H
#define APP_LCD_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 颜色（RGB565） */
#define APP_LCD_COLOR_BLACK    0x0000
#define APP_LCD_COLOR_WHITE    0xFFFF
#define APP_LCD_COLOR_RED      0xF800
#define APP_LCD_COLOR_GREEN    0x07E0
#define APP_LCD_COLOR_BLUE     0x001F
#define APP_LCD_COLOR_YELLOW   0xFFE0
#define APP_LCD_COLOR_CYAN     0x07FF
#define APP_LCD_COLOR_MAGENTA  0xF81F
#define APP_LCD_COLOR_GRAY     0x8410
#define APP_LCD_COLOR_MED_BLUE 0x0024   /* 医疗蓝 TIME_BG_COLOR(0x000521) 的 RGB565 */

/**
  * @brief  初始化 LCD：SPI 总线 + panel IO + ST7365 面板 + 背光
  * @retval ESP_OK 成功；其他见 esp_err_t
  */
esp_err_t app_lcd_init(void);

/**
  * @brief  获取 LCD panel IO 句柄（供 LVGL port 接入，flush 完成回调用）
  * @retval esp_lcd_panel_io_handle_t；未初始化返回 NULL
  */
esp_lcd_panel_io_handle_t app_lcd_get_io(void);

/**
  * @brief  获取 LCD panel 句柄（供 LVGL port 接入，flush 回调调 draw_bitmap）
  * @retval esp_lcd_panel_handle_t；未初始化返回 NULL
  */
esp_lcd_panel_handle_t app_lcd_get_panel(void);

/**
  * @brief  背光开关（LED_PWM，0.D 先做 GPIO 高低电平，高=亮）
  * @param  on true=开 false=关
  */
void app_lcd_backlight(bool on);

/**
  * @brief  全屏填充纯色
  * @param  color RGB565 颜色（主机字节序，驱动内部转大端）
  * @retval ESP_OK 成功
  */
esp_err_t app_lcd_fill(uint16_t color);

/**
 * @brief  排空 LCD SPI 总线上所有在途颜色事务（阻塞等 DMA 全部完成）
 *         内部发一次 polling NOP：esp_lcd 的 tx_param 会先 get_trans_result
 *         回收所有排队事务（等每条 DMA 完成、且 ISR 已跑 post_cb →
 *         on_color_trans_done → lv_disp_flush_ready），再发命令。
 *         返回时总线空闲、无 DMA 在飞、本帧 flush 完成通知已投递。
 *         供"擦写内部 flash 前静默 LCD"使用：确保关 cache 窗口里没有
 *         在途 flush 完成回调会被丢失（LCD SPI 中断/完成回调非 IRAM，
 *         擦 flash 关 cache 时被屏蔽，丢通知会导致 LVGL 刷新永久冻死）。
 * @retval ESP_OK 成功
 */
esp_err_t app_lcd_drain(void);

/**
  * @brief  0.11 裸测刷屏：黑→白→红→彩条→渐变
  *         确认 RGB565 字节序（MSB first）与方向
  */
void app_lcd_bare_test(void);

/**
  * @brief  刷屏性能基准：N 次整屏纯色填充计时
  *         打印 ms/帧 与 帧率，用于调 SPI 时钟验证速度
  */
void app_lcd_bench(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_LCD_H */
