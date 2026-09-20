/**
  ******************************************************************************
  * @文件名称   app_keypad.h
  * @文件描述   LVGL keypad 输入设备（阶段 B1.3）
  *            将 Modbus INPUT_EVENT_BUTTON_PRESSED 映射为 LV_KEY_*，
  *            通过 LVGL group 实现物理按键导航。
  * @按键映射  bit0=END→LV_KEY_ENTER, bit1=UP, bit2=RIGHT, bit3=LEFT,
  *            bit4=DOWN, bit5=ESC→LV_KEY_ESC, bit6=PWR(暂不用)
  ******************************************************************************
  */

#ifndef APP_KEYPAD_H
#define APP_KEYPAD_H

#include "esp_err.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
  * @brief  初始化 LVGL keypad 输入设备 + 订阅 INPUT_EVENT
  * @note   需在 LVGL 初始化之后、创建 UI 之前调用
  * @retval ESP_OK 成功
  */
esp_err_t app_keypad_init(void);

/**
  * @brief  获取默认 LVGL group（供页面将控件加入导航组）
  * @retval lv_group_t*（未初始化返回 NULL）
  */
lv_group_t *app_keypad_get_group(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_KEYPAD_H */
