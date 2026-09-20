/**
  ******************************************************************************
  * @文件名称   app_keypad.c
  * @文件描述   LVGL keypad 输入设备实现（阶段 B1.3）
  *            架构：INPUT_EVENT handler → FreeRTOS queue → keypad_read_cb → LV_KEY_*
  *            使用 LVGL group 实现 focus 导航。
  ******************************************************************************
  */

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "app_keypad.h"
#include "app_events.h"

static const char *TAG = "app_keypad";

#define KEYPAD_QUEUE_LEN    8

/* ---- 按键映射表（index=bit号 → LV_KEY_*） ---- */
static const lv_key_t s_key_map[7] = {
    [0] = LV_KEY_ENTER,     /* END → 确认 */
    [1] = LV_KEY_UP,        /* UP */
    [2] = LV_KEY_RIGHT,     /* RIGHT */
    [3] = LV_KEY_LEFT,      /* LEFT */
    [4] = LV_KEY_DOWN,      /* DOWN */
    [5] = LV_KEY_ESC,       /* ESC → 返回 */
    [6] = LV_KEY_HOME,      /* PWR → HOME（暂不使用） */
};

/* ---- 内部状态 ---- */
static QueueHandle_t s_queue = NULL;
static lv_indev_t   *s_indev = NULL;
static lv_group_t   *s_group = NULL;

/* ================================================================
 *  事件订阅：INPUT_EVENT → push 到 FreeRTOS queue
 * ================================================================ */

static void keypad_event_handler(void *arg, esp_event_base_t base,
                                 int32_t id, void *data)
{
    (void)arg; (void)base;
    if (id != INPUT_EVENT_BUTTON_PRESSED || data == NULL) return;

    input_event_data_t *ev = (input_event_data_t *)data;
    uint8_t bit = ev->key_id;

    /* 只处理 bit0~6 的有效按键，且需要 queue 已初始化 */
    if (bit >= 7 || s_queue == NULL) return;

    /* 查表转 LV_KEY_*，push 到队列
     * 注意：本 handler 跑在 esp_event 任务上下文（非 ISR）→ 用 xQueueSend */
    lv_key_t key = s_key_map[bit];
    ESP_LOGI(TAG, "INPUT bit=%u -> LV_KEY=%d", bit, (int)key);
    xQueueSend(s_queue, &key, 0);
}

/* ================================================================
 *  LVGL read 回调：从队列取按键
 * ================================================================ */

static void keypad_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    lv_key_t key = 0;

    if (xQueueReceive(s_queue, &key, 0) == pdTRUE) {
        data->key   = key;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* ================================================================
 *  初始化
 * ================================================================ */

esp_err_t app_keypad_init(void)
{
    if (s_indev != NULL) return ESP_OK;

    /* 1) 创建 FreeRTOS 按键队列 */
    s_queue = xQueueCreate(KEYPAD_QUEUE_LEN, sizeof(lv_key_t));
    if (s_queue == NULL) {
        ESP_LOGE(TAG, "xQueueCreate failed");
        return ESP_ERR_NO_MEM;
    }

    /* 2) 注册 LVGL keypad 输入设备 */
    s_indev = lv_indev_create();
    if (s_indev == NULL) {
        ESP_LOGE(TAG, "lv_indev_create failed");
        vQueueDelete(s_queue); s_queue = NULL;
        return ESP_FAIL;
    }
    lv_indev_set_type(s_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(s_indev, keypad_read_cb);

    /* 3) 创建默认 group */
    s_group = lv_group_create();
    lv_indev_set_group(s_indev, s_group);

    /* 4) 订阅 INPUT_EVENT */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        INPUT_EVENT, ESP_EVENT_ANY_ID, keypad_event_handler, NULL, NULL));

    ESP_LOGI(TAG, "LVGL keypad indev OK (group=%p, queue=%u slots, "
             "key_map: END/UP/RIGHT/LEFT/DOWN/ESC)", s_group, KEYPAD_QUEUE_LEN);
    return ESP_OK;
}

lv_group_t *app_keypad_get_group(void)
{
    return s_group;
}
