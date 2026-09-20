/**
  ******************************************************************************
  * @文件名称   app_events.h
  * @文件描述   EDA_EMG_ESP32 事件框架（阶段 0.6）
  *            事件基：INPUT_EVENT / DEVICE_EVENT / APP_EVENT / UI_EVENT
  *            分层约定：drivers -> app 状态机 -> comms(后续)/ui(后续 LVGL)
  *            模块之间只收发事件，不直接调用（解耦）。
  * @说明       handler 必须短小（只 post 事件 / 加锁写 UI / 置标志）。
  ******************************************************************************
  */

#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#include "esp_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 事件基声明（定义见 app_events.c） */
ESP_EVENT_DECLARE_BASE(INPUT_EVENT);    /* 输入：按键/编码器 */
ESP_EVENT_DECLARE_BASE(DEVICE_EVENT);   /* 驱动/设备：LCD/PSRAM/外设状态 */
ESP_EVENT_DECLARE_BASE(APP_EVENT);      /* 应用状态机 */
ESP_EVENT_DECLARE_BASE(UI_EVENT);       /* UI 刷新/界面切换 */

/* ---------------- INPUT_EVENT 事件 ID ---------------- */
typedef enum {
    INPUT_EVENT_BUTTON_PRESSED = 0,   /* 按键按下 */
    INPUT_EVENT_BUTTON_RELEASED,      /* 按键释放 */
    INPUT_EVENT_BUTTON_HELD,          /* 按键长按 */
    INPUT_EVENT_ENCODER_CHANGED,      /* 编码器旋转 */
    INPUT_EVENT_MAX
} input_event_id_t;

/* 输入事件负载 */
typedef struct {
    uint8_t  key_id;      /* 按键编号 0~5 */
    uint32_t arg;         /* 编码器增量 / 持续时长等 */
} input_event_data_t;

/* ---------------- DEVICE_EVENT 事件 ID ---------------- */
typedef enum {
    DEVICE_EVENT_LCD_INIT_DONE = 0,   /* LCD 初始化完成 */
    DEVICE_EVENT_LCD_POWER,           /* LCD 背光/电源 */
    DEVICE_EVENT_STACK_OVERFLOW,      /* 栈溢出告警 */
    DEVICE_EVENT_MAX
} device_event_id_t;

/* ---------------- APP_EVENT 事件 ID ---------------- */
typedef enum {
    APP_EVENT_STARTED = 0,            /* 应用启动完成 */
    APP_EVENT_MODE_CHANGE,            /* 治疗/评估模式切换 */
    APP_EVENT_HEARTBEAT_POLL,         /* 心跳周期读取（esp_timer 1s，RTC+电池，handler 里跑 modbus） */
    APP_EVENT_VOICE_PREVIEW,          /* 音量预览（esp_timer debounce 到点，handler 里两步写语音寄存器） */
    APP_EVENT_TREATY_POLL,            /* 治疗 1s 心跳（esp_timer 到点，handler 推进会话状态机 + 下发） */
    APP_EVENT_TREATY_INTENS,          /* 治疗强度实时调节（UI 改档后发，handler 写 mA + 重触发渐变） */
    APP_EVENT_MAX
} app_event_id_t;

/* ---------------- UI_EVENT 事件 ID ---------------- */
typedef enum {
    UI_EVENT_SCREEN_CHANGE = 0,       /* 界面切换 */
    UI_EVENT_REFRESH,                 /* 界面刷新 */
    UI_EVENT_MAX
} ui_event_id_t;

/**
  * @brief  初始化事件框架：创建默认事件循环 + 注册测试 handler
  * @retval ESP_OK 成功；其他见 esp_err_t
  */
esp_err_t app_event_loop_init(void);

/**
  * @brief  发送一组测试事件（验证事件通路，monitor 观察打印）
  * @retval ESP_OK 成功
  */
esp_err_t app_event_post_test_events(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_EVENTS_H */
