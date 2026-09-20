/**
  ******************************************************************************
  * @文件名称   app_heartbeat.c
  * @文件描述   心跳模块实现（见 app_heartbeat.h 头注释）
  *            一次 FC03 读 0x0305 起 10 个寄存器：
  *              vals[0..4] = RTC(0x0305~09 年/月/日/时/分)
  *              vals[5]    = 0x030A (CTRL 只写，读回忽略)
  *              vals[6..9] = 电池(0x030B~0E SOC/电压/状态/电流)
  *             RTC → mktime + clock_settime 同步系统实时钟（顶栏免改）。
  *             esp_timer 回调只发 APP_EVENT_HEARTBEAT_POLL 事件（非阻塞），
  *             modbus 事务在事件循环任务 handler 里执行。
  ******************************************************************************
  */

#include <string.h>
#include <time.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "app_heartbeat.h"
#include "app_events.h"
#include "app_modbus.h"
#include "app_modbus_reg.h"

static const char *TAG = "heartbeat";

#define APP_HEARTBEAT_POLL_MS   1000    /* 心跳检查周期 (ms) */
#define APP_HEARTBEAT_IDLE_MS   3000    /* 总线静默超过该时长才下发一帧探活 (ms) */

static esp_timer_handle_t s_timer;

/* ---- RTC 状态 ---- */
static app_rtc_t s_time;        /* 最近成功读到的 STM32 RTC 时间 */
static bool      s_time_valid;
static app_rtc_t s_pending;     /* 对时草稿 */
static bool      s_pending_init;

/* ---- 电池状态 ---- */
static app_batt_t s_batt;       /* 最近成功读到的电池数据 */
static bool       s_batt_valid;

/* 用给定时间更新系统实时钟（Frame 顶栏每秒 time()+localtime_r 直接可见） */
static void sync_c_clock(const app_rtc_t *t)
{
    struct tm tm = {0};
    struct timespec ts;

    tm.tm_year = t->year - 1900;
    tm.tm_mon  = t->month - 1;
    tm.tm_mday = t->day;
    tm.tm_hour = t->hour;
    tm.tm_min  = t->minute;
    tm.tm_sec  = 0;

    ts.tv_sec  = mktime(&tm);
    ts.tv_nsec = 0;
    if (clock_settime(CLOCK_REALTIME, &ts) == 0)
    {
        ESP_LOGD(TAG, "clock synced: %04d-%02d-%02d %02d:%02d",
                 t->year, t->month, t->day, t->hour, t->minute);
    }
}

/* 一次心跳：FC03 读 0x0305 起 10 个寄存器，同时解析 RTC + 电池 */
static void poll_once(void)
{
    uint16_t vals[10];
    int ret;

    /* 采集推送期间不发事务：响应帧会被 push 解析器误判为采集帧 (invalid CNT) */
    if (app_modbus_push_active()) {
        return;
    }

    /* STM32 OTA 服务期间不发心跳：Boot 在主动拉块(REQ/DATA 流)，
       心跳 FC03 会和 OTA 帧在总线上撞帧、干扰 Boot 收块。 */
    if (app_modbus_ota_serve_active()) {
        return;
    }

    /* 超时兜底探活：只有总线静默超过 IDLE_MS(近期没收到 STM32 任何帧)才下发一帧。
       OTA 时 Boot 不断发 REQ、采集时 STM32 不断推数据 → 总线活跃, 无需周期性心跳;
       空闲时 STM32 不主动说话, 靠这帧确认链路在线并顺带刷新 RTC/电池。 */
    if (app_modbus_ms_since_last_rx() < APP_HEARTBEAT_IDLE_MS) {
        return;
    }

    ret = app_modbus_read_registers(MB_REG_RTC_YEAR, 10, vals);
    if (ret != MB_OK)
    {
        /* 采集推送期间事务被拒：跳过本轮（沿用上次值） */
        if (ret != MB_ERR_BUSY)
        {
            ESP_LOGW(TAG, "poll failed: %s", app_modbus_err_str(ret));
        }
        return;
    }

    /* ---- RTC: vals[0..4] ---- */
    {
        app_rtc_t t;
        t.year   = (int)vals[0];
        t.month  = (int)vals[1];
        t.day    = (int)vals[2];
        t.hour   = (int)vals[3];
        t.minute = (int)vals[4];

        if (t.year < 2000 || t.year > 2099 || t.month < 1 || t.month > 12 ||
            t.day < 1 || t.day > 31 || t.hour > 23 || t.minute > 59)
        {
            ESP_LOGW(TAG, "poll invalid time %04d-%02d-%02d %02d:%02d",
                     t.year, t.month, t.day, t.hour, t.minute);
        }
        else
        {
            s_time = t;
            s_time_valid = true;
            sync_c_clock(&t);
        }
    }

    /* ---- 电池: vals[6..9]（vals[5]=0x030A CTRL 只写，忽略） ---- */
    {
        app_batt_t b;
        b.soc       = (uint8_t)(vals[6] & 0xFF);
        b.volt_mv   = vals[7];
        b.status    = (uint8_t)(vals[8] & 0xFF);
        b.charge_ma = vals[9];

        if (b.soc > 100u || b.status > BAT_STATUS_FAULT ||
            b.volt_mv < 2000u || b.volt_mv > 5000u)
        {
            ESP_LOGW(TAG, "poll invalid batt: soc=%d volt=%d status=%d ma=%d",
                     (int)b.soc, (int)b.volt_mv, (int)b.status, (int)b.charge_ma);
        }
        else
        {
            s_batt = b;
            s_batt_valid = true;
            ESP_LOGD(TAG, "batt soc=%d%% volt=%dmV status=%d ma=%d",
                     (int)b.soc, (int)b.volt_mv, (int)b.status, (int)b.charge_ma);
        }
    }

    /* ---- 电极脱落状态: 非采集期顺带读 0x0300 起 2 个 (DEV_STATUS, ELEC_STATUS) ----
       能走到这里说明总线静默(非采集/非OTA), 正是菜单/设置等非采集页面;
       采集期则由 STM32 状态推帧(CNT=0xFFFE)写入同一快照, 二者互补。 */
    {
        uint16_t elec_vals[2];
        int eret = app_modbus_read_registers(MB_REG_DEV_STATUS, 2, elec_vals);
        if (eret == MB_OK)
        {
            app_modbus_set_elec_status(elec_vals[1]);  /* vals[1] = 0x0301 ELEC_STATUS */
        }
        else if (eret != MB_ERR_BUSY)
        {
            ESP_LOGD(TAG, "elec poll failed: %s", app_modbus_err_str(eret));
        }
    }
}

/* ============ 周期读取：esp_timer + 事件框架（零新增任务） ============ */

static void hb_poll_timer_cb(void *arg)
{
    (void)arg;
    esp_event_post(APP_EVENT, APP_EVENT_HEARTBEAT_POLL, NULL, 0, 0);
}

static void hb_poll_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)id; (void)data;
    poll_once();
}

esp_err_t app_heartbeat_init(void)
{
    esp_err_t ret;

    /* 注册事件 handler：APP_EVENT_HEARTBEAT_POLL → 事件循环任务里执行读取 */
    ret = esp_event_handler_register(APP_EVENT, APP_EVENT_HEARTBEAT_POLL, hb_poll_handler, NULL);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "event handler register failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* esp_timer 周期触发（回调只发事件，非阻塞） */
    esp_timer_create_args_t args = {
        .callback = hb_poll_timer_cb,
        .arg      = NULL,
        .name     = "heartbeat",
    };
    ret = esp_timer_create(&args, &s_timer);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_timer create failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_timer_start_periodic(s_timer, APP_HEARTBEAT_POLL_MS * 1000u);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_timer start failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 立即触发一次首读（事件循环任务里执行），顶栏尽快显示时间/电量 */
    esp_event_post(APP_EVENT, APP_EVENT_HEARTBEAT_POLL, NULL, 0, 0);

    ESP_LOGI(TAG, "heartbeat started (period %d ms)", APP_HEARTBEAT_POLL_MS);
    return ESP_OK;
}

esp_err_t app_heartbeat_stop(void)
{
    if (s_timer != NULL)
    {
        esp_timer_stop(s_timer);
    }
    ESP_LOGI(TAG, "heartbeat stopped");
    return ESP_OK;
}

/* ================================================================
 *  RTC 对外接口
 * ================================================================ */

bool app_heartbeat_get_time(app_rtc_t *out)
{
    if (out == NULL || !s_time_valid) return false;
    *out = s_time;
    return true;
}

static void ensure_pending_init(void)
{
    if (s_pending_init) return;
    if (s_time_valid)
    {
        s_pending = s_time;
    }
    else
    {
        s_pending.year = 2026;
        s_pending.month = 1;
        s_pending.day = 1;
        s_pending.hour = 0;
        s_pending.minute = 0;
    }
    s_pending_init = true;
}

void app_heartbeat_get_pending(app_rtc_t *out)
{
    if (out == NULL) return;
    ensure_pending_init();
    *out = s_pending;
}

void app_heartbeat_set_pending(const app_rtc_t *in)
{
    if (in == NULL) return;
    ensure_pending_init();
    s_pending = *in;
}

int app_heartbeat_commit(void)
{
    uint16_t vals[5];
    int ret;

    ensure_pending_init();

    vals[0] = (uint16_t)s_pending.year;
    vals[1] = (uint16_t)s_pending.month;
    vals[2] = (uint16_t)s_pending.day;
    vals[3] = (uint16_t)s_pending.hour;
    vals[4] = (uint16_t)s_pending.minute;

    /* FC10 写 5 个时间字段 */
    ret = app_modbus_write_registers(MB_REG_RTC_YEAR, vals, 5);
    if (ret != MB_OK)
    {
        ESP_LOGE(TAG, "commit write failed: %s", app_modbus_err_str(ret));
        return ret;
    }

    /* FC06 写 CTRL 提交 */
    ret = app_modbus_write_register(MB_REG_RTC_CTRL, 1);
    if (ret != MB_OK)
    {
        ESP_LOGE(TAG, "commit ctrl failed: %s", app_modbus_err_str(ret));
        return ret;
    }
    return MB_OK;
}

/* ================================================================
 *  电池对外接口
 * ================================================================ */

bool app_heartbeat_get_batt(app_batt_t *out)
{
    if (out == NULL || !s_batt_valid) return false;
    *out = s_batt;
    return true;
}
