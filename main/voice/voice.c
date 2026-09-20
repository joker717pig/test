/**
  ******************************************************************************
  * @文件名称   voice.c
  * @文件描述   语音控制模块（阶段 V2）：通过 Modbus 写 STM32 语音寄存器
  *             - play / stop / set_vol：同步事务（app_modbus_write_register）
  *             - preview（拖动音量实时出声）：esp_timer 300ms debounce →
  *               APP_EVENT_VOICE_PREVIEW 事件 handler（事件循环任务）里两步写：
  *               VOICE_VOL(静默调音量) + VOICE_PLAY(当前语言数字段)
  *             数字段预览：中文 VOICE_ID_NUM1_CN+(v-1)、英文 VOICE_ID_NUM1_EN+(v-1)，v=0 不播。
  ******************************************************************************
  */

#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "voice.h"
#include "app_events.h"
#include "app_modbus.h"
#include "app_modbus_reg.h"
#include "app_params.h"     /* 当前语言 app_params_get_lang() */
#include "lang.h"           /* LANG_EN / LANG_ZH */

static const char *TAG = "voice";

#define VOICE_PREVIEW_DEBOUNCE_MS  300   /* 拖动 debounce，避免每步都发 modbus */

static esp_timer_handle_t s_preview_timer;
static uint8_t s_preview_vol;            /* 待下发预览音量档 0~5 */

static QueueHandle_t s_audio_queue = NULL;  /* 语音播放队列句柄 */

/* 预览数字段：v(1~5) → 当前语言数字段；v=0 返回静音段（不播） */
static uint8_t preview_seg_for(uint8_t vol, bool en)
{
    if (vol == 0u || vol > VOICE_VOL_MAX) return VOICE_ID_SILENCE_20MS;
    if (en) return (uint8_t)(VOICE_ID_NUM1_EN + (vol - 1u));
    return   (uint8_t)(VOICE_ID_NUM1_CN + (vol - 1u));
}

/* ================================================================
 *  同步事务封装
 * ================================================================ */

int voice_play(uint8_t seg)
{
    if (seg < VOICE_SEG_MIN || seg > VOICE_SEG_MAX) return MB_ERR_PARAM;
    /* 采集 push 期间也要能下发语音（评估/治疗动作提示）；
     * 用 nowait：不等待响应、不重试，避免响应被推流淹没导致超时重试连播两次 */
    return app_modbus_write_register_nowait(MB_REG_VOICE_PLAY, seg);
}

int voice_stop(void)
{
    return app_modbus_write_register(MB_REG_VOICE_STOP, 1);
}

int voice_set_vol(uint8_t vol)
{
    if (vol > VOICE_VOL_MAX) return MB_ERR_PARAM;
    return app_modbus_write_register(MB_REG_VOICE_VOL, vol);
}

int voice_is_busy(bool *busy)
{
    uint16_t v;
    int e = app_modbus_read_registers(MB_REG_VOICE_BUSY, 1, &v);
    if (e != MB_OK) return e;
    if (busy) *busy = (v != 0u);
    return MB_OK;
}

uint8_t voice_vol_to_cmd(uint8_t vol)
{
    if (vol > VOICE_VOL_MAX) vol = VOICE_VOL_MAX;
    return (uint8_t)(0xE0u + 3u * vol);
}

/* ================================================================
 *  音量预览：esp_timer debounce → 事件 → 事件循环任务执行 modbus
 * ================================================================ */

static void preview_timer_cb(void *arg)
{
    (void)arg;
    esp_event_post(APP_EVENT, APP_EVENT_VOICE_PREVIEW, NULL, 0, 0);
}

static void preview_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)id; (void)data;
    uint8_t vol = s_preview_vol;
    if (vol > VOICE_VOL_MAX) return;

    /* 第一步：静默调音量（STM32 下发音量命令） */
    int e = app_modbus_write_register(MB_REG_VOICE_VOL, vol);
    if (e != MB_OK)
    {
        ESP_LOGW(TAG, "preview vol write failed: %s", app_modbus_err_str(e));
        return;
    }
    if (vol == 0u) return;   /* 静音：只设音量，不播预览 */

    /* 第二步：播当前语言数字段（STM32 驱动支持播放中打断，串行播出） */
    uint8_t seg = preview_seg_for(vol, app_params_get_lang() == LANG_EN);
    e = app_modbus_write_register(MB_REG_VOICE_PLAY, seg);
    if (e != MB_OK) ESP_LOGW(TAG, "preview play 0x%02X failed: %s", seg, app_modbus_err_str(e));
}

int voice_preview(uint8_t vol)
{
    if (vol > VOICE_VOL_MAX) return MB_ERR_PARAM;
    s_preview_vol = vol;
    esp_timer_stop(s_preview_timer);
    esp_timer_start_once(s_preview_timer, VOICE_PREVIEW_DEBOUNCE_MS * 1000u);
    return MB_OK;
}

/*
 * 处方表目前统一保存中文语音 ID。
 * 如果当前系统语言是英文，则自动转换到对应英文语音段。
 *
 * 中文 0x01 ~ 0x28
 * 英文 0x2E ~ 0x55
 * 两组编号固定相差 0x2D。
 */
static uint8_t voice_id_by_lang(uint8_t id)
{
    if (app_params_get_lang() != LANG_EN) {
        return id;
    }
    /* 已经是英文 ID，不再转换 */
    if (id >= VOICE_ID_SELECT_LANG_EN && id <= VOICE_ID_LINE_LOSS_EN) {
        return id;
    }
    /* 中文语音 ID -> 对应英文语音 ID */
    if (id >= VOICE_ID_SELECT_LANG_CN && id <= VOICE_ID_LINE_LOSS_CN) {
        return (uint8_t)(id + (VOICE_ID_SELECT_LANG_EN - VOICE_ID_SELECT_LANG_CN));
    }
    return id;
}

esp_err_t audio_play_request(uint8_t id, uint16_t ms)
{
    if (!s_audio_queue)
        return ESP_ERR_INVALID_STATE;

    uint8_t play_id = voice_id_by_lang(id);
    audio_segment_t seg = {
        .segment_id = play_id,
        .duration_ms = ms,
    };
    if (xQueueSend(s_audio_queue, &seg, pdMS_TO_TICKS(50)) != pdPASS) {
        ESP_LOGE(TAG, "队列满，语音 [%d] 被丢弃", play_id);
        return ESP_FAIL;
    }
    return ESP_OK;
}


static void audio_playback_task(void *arg) 
{
    audio_segment_t seg;
    while (1) {
        // 阻塞等待
        if (xQueueReceive(s_audio_queue, &seg, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG, "播放: [%d] (%d ms)", 
                     seg.segment_id,seg.duration_ms);
            
            // 实际播放：写 STM32 语音寄存器（失败重试 2 次，间隔 100ms）
            mb_err_t reg = MB_ERR_BUSY;
            for (int attempt = 0; attempt < 3; attempt++) {
                reg = voice_play(seg.segment_id);
                if (reg == MB_OK) break;
                ESP_LOGW(TAG, "voice_play 0x%02X 失败(%s), 重试 %d/2",
                         seg.segment_id, app_modbus_err_str(reg), attempt + 1);
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            vTaskDelay(pdMS_TO_TICKS(seg.duration_ms));  // 等待播放时长
            
            ESP_LOGI(TAG, "语音[%d]播放完成:[%d] ", seg.segment_id, reg);
        }
    }
}

esp_err_t voice_init(void)
{
    s_audio_queue = xQueueCreate(10, sizeof(audio_segment_t));
    xTaskCreate(audio_playback_task, "audio_pb", 4096, NULL, 8, NULL);

    /* 注册预览事件 handler（事件循环任务里执行 modbus，不阻塞 LVGL） */
    esp_err_t ret = esp_event_handler_register(APP_EVENT, APP_EVENT_VOICE_PREVIEW,
                                               preview_handler, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "event handler register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_timer_create_args_t args = {
        .callback = preview_timer_cb,
        .arg      = NULL,
        .name     = "voice_preview",
    };
    ret = esp_timer_create(&args, &s_preview_timer);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_timer create failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 开机把 NVS 音量静默下发一次（芯片音量与设置一致） */
    uint8_t boot_vol = (uint8_t)app_params_get_volume();
    int e = voice_set_vol(boot_vol);
    if (e != MB_OK) ESP_LOGW(TAG, "boot vol sync failed: %s", app_modbus_err_str(e));
    ESP_LOGI(TAG, "voice init OK (vol=%u, preview debounce %d ms)", (unsigned)boot_vol, VOICE_PREVIEW_DEBOUNCE_MS);
    return ESP_OK;
}
