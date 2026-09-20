/**
  ******************************************************************************
  * @文件名称   app_events.c
  * @文件描述   EDA_EMG_ESP32 事件框架实现（阶段 0.6）
  *            默认事件循环 + 事件基定义 + 测试 handler（串口打印验证）。
  ******************************************************************************
  */

#include <inttypes.h>
#include "esp_log.h"
#include "app_events.h"

/* 事件基定义 */
ESP_EVENT_DEFINE_BASE(INPUT_EVENT);
ESP_EVENT_DEFINE_BASE(DEVICE_EVENT);
ESP_EVENT_DEFINE_BASE(APP_EVENT);
ESP_EVENT_DEFINE_BASE(UI_EVENT);

static const char *TAG = "app_events";

/* 事件基 -> 可读名字（用于日志） */
static const char *event_base_name(esp_event_base_t base)
{
    if (base == INPUT_EVENT)  return "INPUT_EVENT";
    if (base == DEVICE_EVENT) return "DEVICE_EVENT";
    if (base == APP_EVENT)    return "APP_EVENT";
    if (base == UI_EVENT)     return "UI_EVENT";
    return "UNKNOWN";
}

/**
  * @brief  测试事件 handler：打印事件基/ID（验证事件通路）
  * @note   生产代码中 handler 应短小，只做事件转发/标志位/加锁写 UI。
  */
static void test_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t id, void *event_data)
{
    (void)handler_args;
    (void)event_data;
    ESP_LOGD(TAG, "EVENT  base=%s id=%" PRId32 " data=%p",
             event_base_name(base), id, event_data);
}

esp_err_t app_event_loop_init(void)
{
    /* 默认事件循环任务栈由 CONFIG_ESP_SYSTEM_EVENT_TASK_STACK_SIZE 决定
     * （默认仅 2304B，见 sdkconfig.defaults 已提到 8192 —— RTC 轮询等 handler
     *  会在事件循环任务里跑 modbus 事务，需要更大栈）。 */
    esp_err_t ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "default event loop already exists");
        } else {
            ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(ret));
            return ret;
        }
    }

    /* 注册测试 handler（对 4 个事件基的所有 ID） */
    ret = esp_event_handler_instance_register(INPUT_EVENT, ESP_EVENT_ANY_ID,
                                              test_event_handler, NULL, NULL);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "register INPUT_EVENT failed: %s", esp_err_to_name(ret)); return ret; }
    ret = esp_event_handler_instance_register(DEVICE_EVENT, ESP_EVENT_ANY_ID,
                                              test_event_handler, NULL, NULL);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "register DEVICE_EVENT failed: %s", esp_err_to_name(ret)); return ret; }
    ret = esp_event_handler_instance_register(APP_EVENT, ESP_EVENT_ANY_ID,
                                              test_event_handler, NULL, NULL);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "register APP_EVENT failed: %s", esp_err_to_name(ret)); return ret; }
    ret = esp_event_handler_instance_register(UI_EVENT, ESP_EVENT_ANY_ID,
                                              test_event_handler, NULL, NULL);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "register UI_EVENT failed: %s", esp_err_to_name(ret)); return ret; }

    ESP_LOGI(TAG, "event loop ready, test handler registered");
    return ESP_OK;
}

esp_err_t app_event_post_test_events(void)
{
    input_event_data_t in = { .key_id = 0, .arg = 1 };

    ESP_ERROR_CHECK(esp_event_post(INPUT_EVENT, INPUT_EVENT_BUTTON_PRESSED,
                                   &in, sizeof(in), portMAX_DELAY));
    ESP_ERROR_CHECK(esp_event_post(DEVICE_EVENT, DEVICE_EVENT_LCD_INIT_DONE,
                                   NULL, 0, portMAX_DELAY));
    ESP_ERROR_CHECK(esp_event_post(APP_EVENT, APP_EVENT_STARTED,
                                   NULL, 0, portMAX_DELAY));
    ESP_ERROR_CHECK(esp_event_post(UI_EVENT, UI_EVENT_REFRESH,
                                   NULL, 0, portMAX_DELAY));

    ESP_LOGI(TAG, "posted 4 test events");
    return ESP_OK;
}
