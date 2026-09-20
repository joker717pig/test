/**
 ******************************************************************************
 * @文件名称   emg_force.c
 * @文件描述   肌电力度引擎实现（阶段 3.2）
 ******************************************************************************
 */

#include "emg_force.h"
#include "emg_dsp.h"
#include "app_modbus.h"
#include "app_modbus_reg.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdbool.h>

static const char *TAG = "emg_force";

#define EMG_FORCE_DEFAULT_FS  500.0f   /* 默认采样率 (盆底肌建议 500SPS) */
#define EMG_FORCE_POLL_MS     50       /* 消费泵周期 (ms) */
#define EMG_FORCE_BATCH       64       /* 每次最多消费样本数 (500SPS×128ms) */
#define EMG_FORCE_SETTLE_MS   0        /* [我们] 启动采集后 settling（设为 0 = 取消；如需恢复改回 5000 等） */

static emg_dsp_t s_dsp;
static volatile float s_env = 0.0f;    /* 当前力度包络 (raw16 LSB) */
static volatile float s_rms = 0.0f;    /* 当前 RMS (raw16 LSB) */
static esp_timer_handle_t s_timer = NULL;
static uint32_t s_settle_end_ms = 0;   /* settling 结束时刻 (esp_timer ms)，期间力度输出 0 */

/* 消费泵使用的静态缓冲（避免 esp_timer 任务栈压力） */
static int16_t s_ch1[EMG_FORCE_BATCH];
static int16_t s_ch2[EMG_FORCE_BATCH];

/**
 * @brief  50ms 消费泵：从采集环形缓冲读走 raw，喂入 DSP，更新力度值
 * @note   运行在 esp_timer 任务上下文；单生产者(RX任务)/单消费者(本泵)环形缓冲，
 *         最坏情况误差 1~2 个样本，对力度反馈无影响。
 */
static void emg_force_poll_cb(void *arg)
{
    (void)arg;
    bool settling = ((uint32_t)(esp_timer_get_time() / 1000) < s_settle_end_ms);

    uint16_t n = app_modbus_acq_read(s_ch1, s_ch2, EMG_FORCE_BATCH);
    if (n == 0) {
        return;
    }

    for (uint16_t i = 0; i < n; i++) {
        float env, rms;
        emg_dsp_process(&s_dsp, s_ch1[i], &env, &rms);
        if (!settling) {          /* settling 期间仍喂 DSP 让滤波器建立，但不对外输出力度 */
            s_env = env;
            s_rms = rms;
        }
    }

    /* 周期打印力度值 (monitor 观察用, 500ms 限流) */
    {
        static uint32_t last_ms = 0;
        uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if (now_ms - last_ms >= 500) {
            last_ms = now_ms;
            ESP_LOGI(TAG, "force env=%.1f uV  rms=%.1f uV",
                     (double)(s_env * EMG_DSP_RAW16_LSB_UV),
                     (double)(s_rms * EMG_DSP_RAW16_LSB_UV));
        }
    }
}

esp_err_t emg_force_init(void)
{
    emg_dsp_init(&s_dsp, EMG_FORCE_DEFAULT_FS);

    esp_timer_create_args_t args = {
        .callback = emg_force_poll_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "emg_force",
    };
    esp_err_t err = esp_timer_create(&args, &s_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "timer create failed: %s", esp_err_to_name(err));
        return err;
    }
    err = esp_timer_start_periodic(s_timer, EMG_FORCE_POLL_MS * 1000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "timer start failed: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "init OK (fs=%.0f, poll=%dms)", EMG_FORCE_DEFAULT_FS, EMG_FORCE_POLL_MS);
    return ESP_OK;
}

void emg_force_set_fs(float fs)
{
    emg_dsp_init(&s_dsp, fs);   /* 重算系数并清零状态 */
    s_env = 0.0f;
    s_rms = 0.0f;
    ESP_LOGI(TAG, "fs set to %.0f", fs);
}

void emg_force_reset(void)
{
    emg_dsp_reset(&s_dsp);
    s_env = 0.0f;
    s_rms = 0.0f;
}

float emg_force_get_env(void)
{
    return s_env;
}

float emg_force_get_rms(void)
{
    return s_rms;
}

esp_err_t emg_force_start_acq(uint16_t sps, uint8_t channel)
{
    int e;

    e = app_modbus_write_register(MB_REG_ACQ_SPS, sps);
    if (e != MB_OK) { ESP_LOGW(TAG, "write SPS failed: %s", app_modbus_err_str(e)); return ESP_FAIL; }
    e = app_modbus_write_register(MB_REG_ACQ_CHANNEL, channel);
    if (e != MB_OK) { ESP_LOGW(TAG, "write CH failed: %s", app_modbus_err_str(e)); return ESP_FAIL; }
    e = app_modbus_write_register(MB_REG_ACQ_CTRL, 1);
    if (e != MB_OK) { ESP_LOGW(TAG, "write CTRL failed: %s", app_modbus_err_str(e)); return ESP_FAIL; }

    if (app_modbus_start_push(channel) != ESP_OK)
        return ESP_FAIL;

    emg_force_set_fs((float)sps);   /* 重算 DSP 系数并清零 */
    s_settle_end_ms = (uint32_t)(esp_timer_get_time() / 1000) + EMG_FORCE_SETTLE_MS;
    ESP_LOGI(TAG, "acq started (sps=%u ch=%u, settle=%dms)", sps, channel, EMG_FORCE_SETTLE_MS);
    return ESP_OK;
}

esp_err_t emg_force_stop_acq(void)
{
    app_modbus_stop_push();
    app_modbus_write_register(MB_REG_ACQ_CTRL, 0);
    emg_force_reset();
    s_settle_end_ms = 0;
    ESP_LOGI(TAG, "acq stopped");
    return ESP_OK;
}
