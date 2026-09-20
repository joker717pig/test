/**
  ******************************************************************************
  * @文件名称   app_therapy.c
  * @文件描述   治疗控制【模板】（阶段C）：从处方表下发真实治疗命令 + 会话状态机
  *
  *             模板核心：按 scheme->steps[] **顺序循环**执行——
  *               每个 STIM 步骤：刺激 on_s 秒 -> 休息 off_s 秒 -> 下一步，循环到总时长结束
  *               （例 处方3疗程1 rx3_c1：步骤0 4Hz/500us 28s/2s -> 步骤1 50Hz/300us 8s/2s -> 步骤0 ...）
  *
  *             下发：FC10 写当前步骤 [freq,pw,mA]（按步骤通道 CH1 0x0100 / CH2 0x0103 / 双通道 6 寄存器）
  *                   -> FC06 启停 0x0106（通道掩码）
 *             双强度：阶段1 mA = intens1，阶段2 mA = intens2（UI 直接选 0~90 mA，原值即毫安下发，不再换算）
 *             强度严格按设置：0 -> mA=0 不输出（无默认回退）
  *             异步：esp_timer 1s(真实) -> APP_EVENT_TREATY_POLL -> 事件任务推进会话时钟 + 状态机
  *             强度实时调：APP_EVENT_TREATY_INTENS -> 事件任务写 mA + 重发 0x0106 触发渐变
  *             时间缩放：therapy_set_scale(N) = 1 真实秒 = N 会话秒（默认 1=真实速度；联调验证用）
  *
  *             参考：main/voice/voice.c（异步下发样板）、EDA_EMG_PC/src/ui/main_window.py::_on_stim_params
  ******************************************************************************
  */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_therapy.h"
#include "app_events.h"
#include "app_modbus.h"
#include "app_modbus_reg.h"
#include "emg_force.h"
#include "menu_ui.h"     /* Treat_Timer_Ctx_t / timer_status_t */
#include "voice.h"
#include "lang.h"       /* g_app_lang */

static const char *TAG = "app_therapy";

/* 强度口径：UI 直接选 0~90，单位即 mA，原值下发给 STM32，不再做档位->mA 换算（2026-09 由旧 0~10 档 x9 迁移） */
#define THERAPY_MA_MAX  90

/* 会话总时长（运行时可改写，默认 30 分钟） */
static uint32_t s_total_ms = THERAPY_TOTAL_MS;

/* ---- 会话状态 ---- */
typedef enum {
    TH_IDLE = 0,     /* 未启动 */
    TH_RUNNING,      /* 运行中（会话时钟推进） */
    TH_PAUSED,       /* 暂停（停波，保留剩余/步骤） */
    TH_END,          /* 结束（总时长完成） */
} therapy_session_t;

/* ---- 当前步骤相位 ---- */
typedef enum {
    PH_ON = 0,       /* 当前步骤电刺激中 */
    PH_OFF,          /* 当前步骤休息中 */
} therapy_phase_t;

// typedef struct {
//     const rx_scheme_t  *scheme;    /* 当前方案（处方表） */
//     Treat_Timer_Ctx_t  *ui;        /* UI 上下文（可 NULL=console 驱动） */
//     therapy_session_t   session;
//     uint8_t             step_idx;             /* 当前步骤（scheme->steps[] 索引，循环） */
//     uint8_t             step_cnt;             /* 步骤数 = scheme->step_cnt */
//     therapy_phase_t     phase;
//     uint8_t             intens1, intens2;     /* 步骤0/1 强度档 0~10（模板 ≤2 步，更多需扩展） */
//     int32_t             remain_ms;            /* 会话剩余（会话时钟，ms） */
//     int32_t             phase_ms;             /* 当前步骤相位已走（会话时钟，ms） */
//     uint32_t            scale_s;              /* 1 真实秒 = N 会话秒 */
//     esp_timer_handle_t  timer;
// } therapy_ctx_t;


typedef struct {
    const rx_scheme_t  *scheme;    /* 当前方案（处方表） */
    Treat_Timer_Ctx_t  *ui;        /* UI 上下文（可 NULL=console 驱动） */
    Chart_Data_t       *chart;                /* 主动训练图表*/
    Chart_Data_t       *chart_instr;          /* 按指示主动训练图表*/
    therapy_session_t   session;
    uint8_t             step_idx;             /* 当前步骤（scheme->steps[] 索引，循环） */
    uint8_t             step_cnt;             /* 步骤数 = scheme->step_cnt */
    therapy_phase_t     phase;
    uint8_t             intens1, intens2;     /* 阶段1/2 强度 0~90（原值即 mA，直接下发） */
    int32_t             remain_ms;            /* 会话剩余（会话时钟，ms） */
    int32_t             phase_ms;             /* 当前步骤相位已走（会话时钟，ms） */
    uint32_t            scale_s;              /* 1 真实秒 = N 会话秒 */
    esp_timer_handle_t  timer;

    int32_t             stim_tol_ms;          /* 电刺激治疗总时间 */
    int32_t             stim_remain_ms;       /* 电刺激治疗剩余时间 */
    int32_t             train_remain_ms;      /* 主动训练剩余时间 */
    int32_t             train_instr_remain_ms;/* 按指示训练剩余时间 */
    int32_t             stage_remain_ms;      /* 阶段会话剩余（会话时钟，ms） */
    int32_t             stage_phase_ms;       /* 阶段当前步骤相位已走（会话时钟，ms） */
    // const rx_scheme_t *my_scheme;             /* 当前方案（处方表） */
    uint8_t             stage_idx;             /* 当前阶段（scheme->stages[] 索引，循环） */
    uint8_t             stage_cnt;             /* 阶段数 = scheme->stage_cnt */
    uint8_t             last_stage;             /* 上一个阶段索引 */
    uint8_t             stim_stage;             /* 电刺激阶段1/2 */

    uint16_t            emg_low_threshold;      /* 肌电阈值，默认 50 µV */
    uint8_t             emg_low_count;          /* 连续低于阈值计数 */

    /* ---- 条件刺激触发状态（3种触发类型共用） ---- */
    uint8_t             emg_fail_count;         /* 类型2: 连续未超过虚线次数 */
    int32_t             emg_peak;               /* 类型2: 当前收缩峰值 */
    uint8_t             emg_in_contraction;     /* 类型2: 是否处于一次收缩中 */
    int32_t             emg_above_blue_ms;      /* 类型3: 信号在蓝色区域(>75)的累计时间 ms */
    uint8_t             emg_was_above_blue;     /* 类型3: 上一采样是否在蓝色区域（用于连续性判断） */

} my_therapy_ctx_t;
static my_therapy_ctx_t s_th;

/* ================================================================
 *  辅助
 * ================================================================ */

/* 当前步骤（模板：按 steps[] 顺序循环） */
rx_step_t *cur_step(void)
{
    const rx_scheme_t *s = s_th.scheme;
    if (!s || s_th.stage_idx >= s->stage_cnt){
        ESP_LOGI(TAG, "获取当前阶段错误[stage%d]\r\n",s_th.stage_idx);
        return NULL;
    } 
    if (!s || s_th.step_idx >= s->steps_per_stage[s_th.stage_idx])  {
        ESP_LOGI(TAG, "获取当前步骤错误[step%d]\r\n",s_th.step_idx);
        return NULL;
    }
    return &s->stages[s_th.stage_idx][s_th.step_idx];
}
/* 当前阶段（模板：按 stage[] 顺序循环） */
rx_step_t *cur_stage(void)
{
    const rx_scheme_t *s = s_th.scheme;
    if (!s || s_th.stage_idx >= s->stage_cnt){
        ESP_LOGI(TAG, "获取当前阶段错误[stage%d]\r\n",s_th.stage_idx);
        return NULL;
    } 
    return s->stages[s_th.stage_idx];
}
static bool phase_is_on(void)
{
    return s_th.phase == PH_ON;
}

/* 当前步骤的强度阶段（1/2；无有效步骤时 0）
 * 直接取处方表里的 rx_step_t.stim_stage，不使用 s_th.stim_stage 缓存：
 * 缓存只在每个会话秒开头更新，而步骤推进（my_enter_next_on / enter_next_data_acq 等）
 * 会在同一秒内把 step_idx 推到下一步并立刻 wave_apply_start()，
 * 因此缓存会滞后一步，导致 mA 与 UI 显示的"当前阶段"不一致。 */
static uint8_t cur_stim_stage(void)
{
    const rx_scheme_t *s = s_th.scheme;
    if (!s || s_th.stage_idx >= s->stage_cnt) return 0;
    if (s_th.step_idx >= s->steps_per_stage[s_th.stage_idx]) return 0;
    return s->stages[s_th.stage_idx][s_th.step_idx].stim_stage;
}

/* 当前阶段强度（stim_stage 1 -> intens1；2 -> intens2；单位 mA，0~90） */
static uint8_t step_intens(void)
{
    switch (cur_stim_stage()) {
    case 1:  return s_th.intens1;
    case 2:  return s_th.intens2;
    default: return 0;
    }
}

/* 当前步骤 mA（强度原值即毫安；严格按设置：0 -> mA 0 不输出） */
static uint16_t step_ma(void)
{
    return (uint16_t)step_intens();
}

/* 步骤刺激通道掩码 -> 0x0106 启停值（rx_step_t.ch 与 WAVE_CTRL 位定义一致：CH1=1 CH2=2） */
static uint16_t step_wave_mask(const rx_step_t *st)
{
    return (uint16_t)(st->ch & (THERAPY_CH1 | THERAPY_CH2));
}

/* 写当前步骤参数（按通道：CH1 0x0100 / CH2 0x0103 / 双通道 6 寄存器；对齐 PC main_window.py）
 * @retval app_modbus 返回码 */
static int write_stim_params(const rx_step_t *st, uint16_t ma)
{
    uint16_t regs[6] = {0};
    uint16_t n = 0;
    if (st->ch & THERAPY_CH1) { regs[n++] = st->freq; regs[n++] = st->pw; regs[n++] = ma; }
    if (st->ch & THERAPY_CH2) { regs[n++] = st->freq; regs[n++] = st->pw; regs[n++] = ma; }
    uint16_t base = (st->ch & THERAPY_CH1) ? MB_REG_CH1_FREQ : MB_REG_CH2_FREQ;
    return app_modbus_write_registers(base, regs, n);
}

/* 实时写当前步骤 mA（按通道；@retval app_modbus 返回码） */
static int write_ma(const rx_step_t *st, uint16_t ma)
{
    int e = MB_OK;
    if (st->ch & THERAPY_CH1) e = app_modbus_write_register(MB_REG_CH1_MA, ma);
    if (st->ch & THERAPY_CH2) e = app_modbus_write_register(MB_REG_CH2_MA, ma);
    return e;
}

/* 同步 UI 字段（remain_sec/status/Intens_Value；跨任务裸写 int32 可接受） */
static void ui_sync(void)
{
    if (s_th.ui) {
        s_th.ui->remain_sec = s_th.stim_remain_ms;
        s_th.ui->treat_data.Intens_Value = (int32_t)step_intens();
        switch (s_th.session) {
        case TH_RUNNING: s_th.ui->status = TIMER_RUNNING; break;
        case TH_PAUSED:  s_th.ui->status = TIMER_PAUSED;  break;
        case TH_END:     s_th.ui->status = TIMER_END;     break;
        default:         s_th.ui->status = TIMER_IDLE;    break;
        }
    }
    if(s_th.chart) {
        s_th.chart->Value_TimeLeft = s_th.train_remain_ms;
    }
    if(s_th.chart_instr){
         s_th.chart_instr->Value_TimeLeft = s_th.train_instr_remain_ms;
    }
}

/* ================================================================
 *  下发（事件任务上下文执行，不卡 LVGL）
 * ================================================================ */

/* 可靠停波：0x0106=0 是安全关键命令，必须尽量送达底层（STM32 无通信超时/看门狗自保，
 * 停波令一旦丢失会无限期持续输出）。
 *  - push(采集推流)期间普通事务被 mb_transact 以 MB_ERR_BUSY 直接拒绝，
 *    改用 nowait（不等响应、可在推流帧间隙插队，见 mb_send_only 不查 push）连发；
 *  - 非 push 期用带 150ms×4 重试的确认写；若仍失败再用 nowait 补发兜底。
 * 不在此处停 push（push 生命周期由采集 / therapy_emergency_stop 管理）。 */
static void wave_stop(void)
{
    if (app_modbus_push_active()) {
        for (int i = 0; i < 2; i++) {
            int e = app_modbus_write_register_nowait(MB_REG_WAVE_CTRL, WAVE_CTRL_STOP);
            ESP_LOGW(TAG, "wave stop(push) #%d (0x0106=0 nowait) -> %s",
                     i + 1, app_modbus_err_str(e));
            vTaskDelay(pdMS_TO_TICKS(30));
        }
        return;
    }

    int e = app_modbus_write_register(MB_REG_WAVE_CTRL, WAVE_CTRL_STOP);  /* 内部 150ms×4 重试 */
    if (e == MB_OK) {
        ESP_LOGI(TAG, "wave stop (0x0106=0) -> OK");
        return;
    }

    /* 确认写失败（超时/CRC/总线占用）：nowait 连发兜底，尽量保证停波送达 */
    ESP_LOGE(TAG, "wave stop confirmed FAIL: %s -> nowait 补发", app_modbus_err_str(e));
    for (int i = 0; i < 3; i++) {
        int e2 = app_modbus_write_register_nowait(MB_REG_WAVE_CTRL, WAVE_CTRL_STOP);
        ESP_LOGW(TAG, "wave stop fallback #%d -> %s", i + 1, app_modbus_err_str(e2));
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

/* 应用当前步骤参数并启动（mA=0 时不启动，严格按设置） */
static void wave_apply_start(void)
{
    const rx_step_t *st = cur_step();
    if (!st) return;
    uint16_t ma = step_ma();
    int e = write_stim_params(st, ma);
    ESP_LOGI(TAG, "step %d: FC10 [f=%d pw=%d mA=%d ch=0x%02X] -> %s",
             (int)s_th.step_idx, (int)st->freq, (int)st->pw, (int)ma,
             (unsigned)st->ch, app_modbus_err_str(e));
    if (ma > 0) {
        int e = app_modbus_write_register(MB_REG_WAVE_CTRL, step_wave_mask(st));
        ESP_LOGI(TAG, "  start wave (0x0106=0x%02X) -> %s", (unsigned)step_wave_mask(st), app_modbus_err_str(e));
    } else {
        ESP_LOGW(TAG, "  intens=0 -> mA=0 不启动（严格按设置）");
        app_modbus_write_register(MB_REG_WAVE_CTRL, WAVE_CTRL_STOP);
    }
}
/* ================================================================
 *  状态机（模板核心：按 steps[] 顺序循环）
 * ================================================================ */

/* 进入休息：停波 */
static void enter_off(void)
{
    s_th.phase = PH_OFF;
    s_th.phase_ms = 0;
    wave_stop();
}



/* 进入下一步刺激：步骤索引 +1 取模 -> 应用参数并启动 + 同步 UI（强度标签跟随） */
static void enter_next_on(void)
{
    if (s_th.step_cnt == 0) return;
    s_th.step_idx = (uint8_t)((s_th.step_idx + 1) % s_th.step_cnt);
    s_th.phase = PH_ON;
    s_th.phase_ms = 0;
    wave_apply_start();
    ui_sync();
}
/* 会话开始：从步骤0 刺激进入 */
static void enter_start_on(void)
{
    s_th.step_idx = 0;
    s_th.phase = PH_ON;
    s_th.phase_ms = 0;
    wave_apply_start();
}

static void session_end(void)
{
    wave_stop();
    s_th.session = TH_END;
    s_th.remain_ms = 0;
    s_th.phase_ms = 0;
    if (s_th.ui) { s_th.ui->remain_sec = 0; s_th.ui->status = TIMER_END; }
    if (s_th.chart) {s_th.chart->status = TIMER_END; }
    if (s_th.chart_instr) {s_th.chart_instr->status = TIMER_END; }
    ESP_LOGI(TAG, "session END (总时长完成)");
}
/* 进入下一步刺激：步骤索引 +1 取模 -> 应用参数并启动 + 同步 UI（强度标签跟随） */
static void my_enter_next_on(void)
{
    uint8_t voice_id = 0;
    if (s_th.stage_cnt == 0 || s_th.step_cnt == 0) return;
    if(s_th.last_stage != s_th.stage_idx) return;
    s_th.step_idx = (uint8_t)((s_th.step_idx + 1) % s_th.step_cnt);
    s_th.phase = PH_ON;
    s_th.phase_ms = 0;
    wave_apply_start();
    ui_sync();
     
}
/* 数据采集 进入下一步：步骤索引 +1 取模 -> 应用参数并启动 + 同步 UI（强度标签跟随） */
static void enter_next_data_acq(void)
{
    uint8_t voice_id = 0;
    if (s_th.stage_cnt == 0 || s_th.step_cnt == 0) return;
    if(s_th.last_stage != s_th.stage_idx) { 
        s_th.last_stage = s_th.stage_idx;
        s_th.step_idx = 0;
        s_th.stim_stage = 1;
        emg_force_stop_acq();
        enter_start_on();
        audio_play_request(VOICE_ID_COLLECT_DONE_CN,1500);              //语音播放:采集完成
       
    }else {
        s_th.step_idx = (uint8_t)((s_th.step_idx + 1) % s_th.step_cnt);
    }
    s_th.phase_ms = 0;
    voice_id = s_th.scheme->stages[s_th.stage_idx][s_th.step_idx].voice;
    if(voice_id) audio_play_request(voice_id,1500);              //语音播放;
    ui_sync();
}
/* 主动训练 进入下一步：步骤索引 +1 取模  */
static void enter_next_train(void)
{
    uint8_t voice_id = 0;
    if (s_th.stage_cnt == 0 || s_th.step_cnt == 0) return;
    if(s_th.last_stage != s_th.stage_idx) { 
        s_th.last_stage = s_th.stage_idx;
        s_th.step_idx = 0;
    }else {
        s_th.step_idx = (uint8_t)((s_th.step_idx + 1) % (s_th.step_cnt-1));     
    }
    voice_id = s_th.scheme->stages[s_th.stage_idx][s_th.step_idx].voice;
    if(voice_id) audio_play_request(voice_id,1500);              //语音播放;
    ui_sync();
    s_th.phase_ms = 0;
}
/* 条件刺激 进入下一步 */
static void enter_next_cond(void)
{
    s_th.step_idx = 2;
    s_th.phase_ms = 0;
    s_th.stim_stage = 1;
    s_th.phase = PH_ON;
    wave_apply_start();
    ui_sync();
}

static void play_next_voice()
{
    uint8_t voice_id = 0;
    if(s_th.last_stage != s_th.stage_idx) { 
        s_th.last_stage = s_th.stage_idx;
        s_th.step_idx = 0;
        voice_id = s_th.scheme->stages[s_th.stage_idx][s_th.step_idx].voice;
        if(voice_id)  audio_play_request(voice_id,1500);              //语音播放;
    }

    
}
static void enter_phase_treat(const rx_step_t *st) 
{
    switch(st->type) {
        /* 数据采集 */ 
        case RX_STEP_ACQ:      
            ESP_LOGI(TAG, "[%d]采集数据中...\r\n",st->type);
            if (s_th.phase_ms >= (int32_t)st->dur_s * 1000) enter_next_data_acq();
              
        break;
        /* 电刺激治疗 */ 
        case RX_STEP_STIM:
             if (s_th.phase == PH_ON) {
                ESP_LOGI(TAG, "电刺激中... step_idx:%d stage_idx:%d\r\n",s_th.step_idx,s_th.stage_idx);
                /* 电刺激时长到 -> 休息 */
                if (s_th.phase_ms >= (int32_t)st->on_s * 1000) enter_off();
                
            } else {
                ESP_LOGI(TAG, "休息中...step_idx:%d,stage_idx:%d\r\n",s_th.step_idx,s_th.stage_idx);
                /* 休息时长到 -> 下一步刺激 */
                if (s_th.phase_ms >= (int32_t)st->off_s * 1000) my_enter_next_on();
                
            }
            
        break;
        /* 主动训练+条件刺激触发 */ 
        case RX_STEP_TRAIN:      
            ESP_LOGI(TAG, "[%d]主动训练中...训练类型[%d]\r\n",st->type,st->train);
            if (s_th.phase_ms >= (int32_t)st->dur_s * 1000) enter_next_train();
            /* 训练类型: 收缩并保持 */ 
            if(st->train == RX_TRAIN_HOLD)  {
                /* 采集肌电信号 若不超过参考值 马上进入下一步骤：电刺激 */
                // enter_next_cond();
            } 
            /* 训练类型: 大力收缩N次 */ 
            else if(st->train == RX_TRAIN_STRONG)  {
                /* 采集肌电信号 若三次不超过参考值 马上进入下一步骤：电刺激 */
                // enter_next_cond();
            }
            
        break;
         /* 按指示主动训练 */ 
        case RX_STEP_TRAIN_INSTR:      
            ESP_LOGI(TAG, "[%d]按指示主动训练...训练类型[%d]\r\n",st->type,st->train);
            if (s_th.phase_ms >= (int32_t)st->dur_s * 1000) enter_next_data_acq();
            
        break;
         /* 条件刺激  */ 
        case RX_STEP_COND:      
            ESP_LOGI(TAG, "[%d]条件刺激中...\r\n",st->type);
            
            if (s_th.phase_ms >= (int32_t)st->dur_s * 1000) {
                s_th.step_idx = 1;
                enter_off();
                enter_next_train();
                emg_force_start_acq(500, 1);                   // 启动采集 (500SPS, CH1)
            }
        break;
        default: break;
    }
    if(s_th.ui) {
        if( s_th.ui->status == TIMER_END) {
            s_th.ui = 0;
            s_th.step_idx = 0;
            enter_off();
            ESP_LOGI(TAG, "电刺激治疗结束,stage_idx:%d,step_idx:%d\r\n",s_th.stage_idx,s_th.step_idx);
            return;
        }
    }
    if(s_th.chart) {
        if(s_th.chart->status == TIMER_END) {
            s_th.chart = 0;
            s_th.step_idx = 0;
            enter_off();
            ESP_LOGI(TAG, "主动训练结束,stage_idx:%d,step_idx:%d\r\n",s_th.stage_idx,s_th.step_idx);
            return;
        }
    }
    if(s_th.chart_instr) {
        if(s_th.chart_instr->status == TIMER_END) {
            s_th.chart_instr = 0;
            ESP_LOGI(TAG, "按指示训练结束,stage_idx:%d,step_idx:%d\r\n",s_th.stage_idx,s_th.step_idx);
            return;
        } 
    }
    play_next_voice();
    ui_sync();
}
/* 推进 1 个会话秒 */
static void advance_one_session_sec(void)
{
    
    const rx_step_t *st = cur_step();
    if (!st) return;
    s_th.remain_ms -= 1000;             // 疗程总剩余时间
    s_th.stage_remain_ms -= 1000;       // 阶段剩余时间
    if(s_th.stim_remain_ms)  s_th.stim_remain_ms -= 1000;        // 电刺激总剩余时间
    if(s_th.train_remain_ms)  s_th.train_remain_ms -= 1000;      // 主动训练总剩余时间
    if(s_th.train_instr_remain_ms)  s_th.train_instr_remain_ms -= 1000;      // 按指示主动训练总剩余时间
    s_th.phase_ms += 1000;

    if (s_th.remain_ms <= 0) { s_th.remain_ms = 0; session_end(); return; }
    
    if (s_th.stage_remain_ms <= 0) {    /* 本次阶段循环时间结束 */
        s_th.stage_idx++;
        if(s_th.stage_idx < s_th.stage_cnt) {
            s_th.step_cnt = s_th.scheme->steps_per_stage[s_th.stage_idx];
            s_th.stage_remain_ms = s_th.scheme->stage_time[s_th.stage_idx];
            rx_step_t *step = s_th.scheme->stages[s_th.stage_idx];
            if(step->type == RX_STEP_STIM) {
                s_th.stim_remain_ms = s_th.stage_remain_ms; 
                ESP_LOGI(TAG, "电刺激时间%02d:%02d:%02d",
                                (int)( s_th.stage_remain_ms / 60000),
                                (int)(( s_th.stage_remain_ms % 60000) / 1000),
                                (int)(( s_th.stage_remain_ms % 1000) / 10));
            }
            if(step->type == RX_STEP_TRAIN) {
                s_th.train_remain_ms = s_th.stage_remain_ms; 
                ESP_LOGI(TAG, "主动训练时间%02d:%02d:%02d",
                                (int)( s_th.stage_remain_ms / 60000),
                                (int)(( s_th.stage_remain_ms % 60000) / 1000),
                                (int)(( s_th.stage_remain_ms % 1000) / 10));
            }
            if(step->type == RX_STEP_TRAIN_INSTR) {
                s_th.train_instr_remain_ms = s_th.stage_remain_ms; 
                ESP_LOGI(TAG, "按指示训练时间%02d:%02d:%02d",
                                (int)( s_th.stage_remain_ms / 60000),
                                (int)(( s_th.stage_remain_ms % 60000) / 1000),
                                (int)(( s_th.stage_remain_ms % 1000) / 10));
            }
        } else {
            s_th.stage_idx = s_th.stage_cnt - 1;
        }
    }
    if(s_th.ui) { 
        if(s_th.stim_remain_ms <= 0) {
            s_th.ui->remain_sec = 0; 
            s_th.ui->status = TIMER_END; 
            ESP_LOGI(TAG, "电刺激治疗时间到\r\n");
        }
    }
    if(s_th.chart) { 
        if(s_th.train_remain_ms == 0) {
            s_th.chart->status = TIMER_END; 
            ESP_LOGI(TAG, "主动训练时间到\r\n");
        }
    }
    if (s_th.chart_instr) { 
        if(s_th.train_instr_remain_ms == 0) {
            s_th.chart_instr->status = TIMER_END;
            ESP_LOGI(TAG, "按指示训练时间到\r\n");
        } 
    }

    /* 本秒的步骤推进在 enter_phase_treat() 内部完成（可能已切到下一步/新阶段），
     * 因此缓存须在推进后再同步；强度取值本身走 step_intens()->cur_stim_stage() 直接查表，
     * 不依赖该缓存（旧写法在推进前赋值，导致 mA 比 UI 慢一步）。 */
    enter_phase_treat(st);
    s_th.stim_stage = cur_stim_stage();
}

/* ================================================================
 *  事件 / 定时器
 * ================================================================ */

static void tick_cb(void *arg)
{
    (void)arg;
    /* 仅运行中才发心跳事件（空闲/暂停/结束不唤醒事件任务，避免 monitor 噪音） */
    if (s_th.session == TH_RUNNING) {
        esp_event_post(APP_EVENT, APP_EVENT_TREATY_POLL, NULL, 0, 0);
    }
}

static void poll_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base; (void)data;

    if (id == APP_EVENT_TREATY_POLL) {
        if (s_th.session != TH_RUNNING) return;
        for (uint32_t i = 0; i < s_th.scale_s && s_th.session == TH_RUNNING; i++) {
            advance_one_session_sec();
        }
    } else if (id == APP_EVENT_TREATY_INTENS) {
        if (s_th.session == TH_RUNNING && phase_is_on()) {
            /* 实时调当前步骤 mA + 重发 0x0106 触发渐变（0T 方案A，只置位） */
            const rx_step_t *st = cur_step();
            if (st) {
                uint16_t ma = step_ma();
                int e = write_ma(st, ma);
                ESP_LOGI(TAG, "intens 实时: mA=%d ch=0x%02X -> %s",
                         (int)ma, (unsigned)step_wave_mask(st), app_modbus_err_str(e));
                e = app_modbus_write_register(MB_REG_WAVE_CTRL, step_wave_mask(st));
                ESP_LOGI(TAG, "  re-trigger (0x0106=0x%02X) -> %s", (unsigned)step_wave_mask(st), app_modbus_err_str(e));
            }
        }
        /* 强度值已在 therapy_set_intensity 更新 + ui 已同步 */
    }
}

/* ================================================================
 *  公开 API
 * ================================================================ */

/* 参数设置页调节强度 */
void therapy_send_intensity(uint8_t intens,uint16_t freq,uint16_t pw)
{
    if(!s_th.scheme) return;
    uint16_t regs[6] = {0};
    uint16_t n = 0;
    uint16_t ma = intens;   /* UI 值即 mA（0~90），原值下发，不换算 */
    if (s_th.scheme->ch & THERAPY_CH1) { regs[n++] = freq; regs[n++] = pw; regs[n++] = ma; }
    if (s_th.scheme->ch & THERAPY_CH2) { regs[n++] = freq; regs[n++] = pw; regs[n++] = ma; }
    uint16_t base = (s_th.scheme->ch & THERAPY_CH1) ? MB_REG_CH1_FREQ : MB_REG_CH2_FREQ;
    int e = app_modbus_write_registers(base, regs, n);
    ESP_LOGI(TAG, "FC10 [f=%d pw=%d mA=%d ch=0x%02X] -> %s",
                (int)freq, (int)pw, (int)ma,(unsigned)s_th.scheme->ch, app_modbus_err_str(e));

    uint8_t ch = (s_th.scheme->ch & (THERAPY_CH1 | THERAPY_CH2));
    if (ma > 0) {
        int e = app_modbus_write_register(MB_REG_WAVE_CTRL, ch);
        ESP_LOGI(TAG, "  start wave (0x0106=0x%02X) -> %s", (unsigned)ch, app_modbus_err_str(e));
    } else {
        ESP_LOGW(TAG, "  intens=0 -> mA=0 不启动（严格按设置）");
        app_modbus_write_register(MB_REG_WAVE_CTRL, WAVE_CTRL_STOP);
    }
}

esp_err_t app_therapy_init(void)
{
    memset(&s_th, 0, sizeof(s_th));
    s_th.scale_s = 1;                 /* 默认真实速度 */
    s_th.session = TH_IDLE;

    esp_err_t ret = esp_event_handler_register(APP_EVENT, APP_EVENT_TREATY_POLL, poll_handler, NULL);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "register TREATY_POLL failed: %s", esp_err_to_name(ret)); return ret; }
    ret = esp_event_handler_register(APP_EVENT, APP_EVENT_TREATY_INTENS, poll_handler, NULL);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "register TREATY_INTENS failed: %s", esp_err_to_name(ret)); return ret; }

    esp_timer_create_args_t args = {
        .callback = tick_cb,
        .name = "therapy_tick",
    };
    ret = esp_timer_create(&args, &s_th.timer);
    if (ret != ESP_OK) { ESP_LOGE(TAG, "create timer failed: %s", esp_err_to_name(ret)); return ret; }
    ret = esp_timer_start_periodic(s_th.timer, 1000000);   /* 1s 真实 */
    if (ret != ESP_OK) { ESP_LOGE(TAG, "start timer failed: %s", esp_err_to_name(ret)); return ret; }

    ESP_LOGI(TAG, "therapy template init OK (scale=%d=真实速度)", (int)s_th.scale_s);
    return ESP_OK;
}
int therapy_init_scheme(const rx_scheme_t *scheme)
{
    if (!scheme || scheme->stage_cnt == 0) return MB_ERR_PARAM;
    s_th.scheme  = scheme;
    s_th.stage_cnt = scheme->stage_cnt;
    s_th.stage_idx = 0;
    s_th.last_stage = 0;
    s_th.step_cnt = scheme->steps_per_stage[0];
    s_th.step_idx = 0; 
    return MB_OK;   
}
/* 运行时改写阶段时长（串口指令用）：scheme 为 NULL 时用当前会话方案 */
int therapy_set_stage_time(const rx_scheme_t *scheme, uint8_t stage_idx, uint32_t ms)
{
    const rx_scheme_t *s = scheme ? scheme : s_th.scheme;
    if (!s || !s->stage_time || stage_idx >= s->stage_cnt) return MB_ERR_PARAM;
    s->stage_time[stage_idx] = ms;
    ESP_LOGI(TAG, "set stage_time[%d] = %u ms (%d min %d s)",
             (int)stage_idx, (unsigned)ms, (int)(ms / 60000), (int)((ms % 60000) / 1000));
    return MB_OK;
}
uint32_t therapy_get_stage_time(const rx_scheme_t *scheme, uint8_t stage_idx)
{
    const rx_scheme_t *s = scheme ? scheme : s_th.scheme;
    if (!s || !s->stage_time || stage_idx >= s->stage_cnt) return 0;
    return s->stage_time[stage_idx];
}
/* 运行时改写会话总时长（串口指令用） */
void therapy_set_total_time(uint32_t ms)
{
    s_total_ms = ms;
    ESP_LOGI(TAG, "set total_time = %u ms (%d min %d s)",
             (unsigned)ms, (int)(ms / 60000), (int)((ms % 60000) / 1000));
}
uint32_t therapy_get_total_time(void)
{
    return s_total_ms;
}
int therapy_start(const rx_scheme_t *scheme, int intens1, int intens2, void *ui_ctx)
{
    if (!scheme || scheme->stage_cnt == 0) return MB_ERR_PARAM;

    /* 上一会话若在跑/暂停，先停波 */
    if (s_th.session == TH_RUNNING || s_th.session == TH_PAUSED) {
        wave_stop();
    }

    s_th.scheme  = scheme;
    s_th.ui      = (Treat_Timer_Ctx_t*)ui_ctx;
    s_th.stage_cnt = scheme->stage_cnt;
    s_th.stage_idx = 0;
    s_th.last_stage = 0;
    s_th.step_cnt = scheme->steps_per_stage[0];
    s_th.step_idx = 0;
    s_th.intens1 = (uint8_t)((intens1 < 0) ? 0 : (intens1 > THERAPY_MA_MAX ? THERAPY_MA_MAX : intens1));
    s_th.intens2 = (uint8_t)((intens2 < 0) ? 0 : (intens2 > THERAPY_MA_MAX ? THERAPY_MA_MAX : intens2));
    s_th.remain_ms = s_total_ms;                            /* 会话总时长（可运行时改写） */
    s_th.phase_ms  = 0;
    s_th.stim_tol_ms = STIM_TOTAL_MS;
    s_th.stage_remain_ms = scheme->stage_time[0];
    s_th.session   = TH_RUNNING;
    s_th.step_idx = 0;
    s_th.phase = PH_ON;
    s_th.stim_stage = 1;
    // if (s_th.ui) s_th.ui->total_sec = THERAPY_TOTAL_MS;  /* 没用到 */
    s_th.emg_low_threshold = RX_TRIGGER_REF_LINE;            /* 阈值：参考线 25 */
    s_th.emg_low_count = 0;
    s_th.emg_fail_count = 0;
    s_th.emg_peak = 0;
    s_th.emg_in_contraction = 0;
    s_th.emg_above_blue_ms = 0;
    s_th.emg_was_above_blue = 0;
    s_th.chart = NULL;
    s_th.chart_instr = NULL;
    uint8_t stage_cnt = s_th.stage_cnt;
    s_th.stim_remain_ms = 0;
    for(uint8_t i = 0; i < stage_cnt;i++) {
        if(scheme->stages[i]->type == RX_STEP_STIM ||
            scheme->stages[i]->type == RX_STEP_ACQ)  {
            s_th.stim_remain_ms += scheme->stage_time[i];  //电刺激治疗剩余时间 =  数据采集阶段+电刺激治疗阶段 
            if(i == 0 && scheme->stages[i]->type == RX_STEP_STIM)  enter_start_on();    /* 电刺激才需要 从步骤0 开始 */
            else if(i == 0 && scheme->stages[i]->type == RX_STEP_ACQ) {
                emg_force_reset();                              /* 清零滤波器/包络状态 */
                emg_force_start_acq(500, 1);                    // 启动采集 (500SPS, CH1)
            }  
                
        }
    }
    audio_play_request(VOICE_ID_START_CN,1500);      //语音播放：开始 
    if(scheme->stages[0][0].voice){
        ESP_LOGI(TAG, "疗程有语音段，播放语音id:%d",scheme->stages[0][0].voice);
        audio_play_request(scheme->stages[0][0].voice,1500);              //语音播放
    } 
    ui_sync();

    ESP_LOGI(TAG, "start [%s]: %d steps, %d stage intens1=%dmA intens2=%dmA %dmin scale=%d",
             scheme->note ? scheme->note : "?", (int)s_th.step_cnt,(int)s_th.stage_cnt,
             (int)s_th.intens1, (int)s_th.intens2,
             (int)(s_total_ms / 60000), (int)s_th.scale_s);
    return MB_OK;
}

/**
 * @brief  凯格尔训练专用启动：纯生物反馈，无电刺激
 * @note   与 therapy_start 的区别：
 *         - 不写 FC10 刺激参数、不写 0x0106 波形控制（无电刺激）
 *         - 不调用 enter_start_on() / wave_apply_start()
 *         - 直接以 PH_ON 进入第一个 TRAIN_INSTR 步骤，由 UI 跳转到 Chart1 页
 * @param  scheme   凯格尔处方方案（&g_rx_all[RX_4]->schemes[idx]）
 * @param  intens1  阶段1 强度（凯格尔不用，传 0 即可）
 * @param  intens2  阶段2 强度（凯格尔不用，传 0 即可）
 * @param  ui_ctx   UI 上下文（Treat_Timer_Ctx_t*，可 NULL）
 * @retval MB_OK 成功；MB_ERR_PARAM 参数错误
 */
int therapy_start_kegel(const rx_scheme_t *scheme)
{
    if (!scheme || scheme->stage_cnt == 0) return MB_ERR_PARAM;

    /* 上一会话若在跑/暂停，先停波（兜底，凯格尔本身无波） */
    if (s_th.session == TH_RUNNING || s_th.session == TH_PAUSED) {
        wave_stop();
    }

    s_th.scheme  = scheme;
    s_th.stage_cnt = scheme->stage_cnt;
    s_th.stage_idx = 0;
    s_th.last_stage = 0;
    s_th.step_cnt = scheme->steps_per_stage[0];
    s_th.step_idx = 0;
    s_th.remain_ms = s_total_ms;
    s_th.phase_ms  = 0;
    s_th.stage_remain_ms = scheme->stage_time[0];
    s_th.session   = TH_RUNNING;
    s_th.phase = PH_ON;               /* 直接进入训练步骤（无 ON/OFF 交替） */
    s_th.stim_stage = 0;              /* 无电刺激阶段 */
    s_th.emg_low_threshold = RX_TRIGGER_REF_LINE;
    s_th.emg_low_count = 0;
    s_th.emg_fail_count = 0;
    s_th.emg_peak = 0;
    s_th.emg_in_contraction = 0;
    s_th.emg_above_blue_ms = 0;
    s_th.emg_was_above_blue = 0;
    s_th.chart = NULL;
    s_th.chart_instr = NULL;

    /* 播放开始语音 */
    audio_play_request(VOICE_ID_START_CN, 1500);
    /* 第一个步骤有语音则播放（如"放松"） */
    if (scheme->stages[0][0].voice) {
        ESP_LOGI(TAG, "kegel: 首步语音 id=%d", scheme->stages[0][0].voice);
        audio_play_request(scheme->stages[0][0].voice, 1500);
    }

    ui_sync();

    ESP_LOGI(TAG, "kegel start [%s]: %d steps, %d stage,scale=%d",
             scheme->note ? scheme->note : "?", (int)s_th.step_cnt, (int)s_th.stage_cnt,(int)s_th.scale_s);
    return MB_OK;
}

void therapy_init_chart(void *ui_chart) 
{
    s_th.chart = (Chart_Data_t*)ui_chart;
    s_th.chart->status = TIMER_RUNNING;
}
void therapy_init_chart_instr(void *ui_chart) 
{
    s_th.chart_instr = (Chart_Data_t*)ui_chart;
    s_th.chart_instr->status = TIMER_RUNNING;
}
void therapy_pause(void)
{
    if (s_th.session != TH_RUNNING) return;
    wave_stop();
    s_th.session = TH_PAUSED;
    ui_sync();
    ESP_LOGI(TAG, "pause (remain=%dms, step=%d, phase=%s)",
             (int)s_th.remain_ms, (int)s_th.step_idx, s_th.phase == PH_ON ? "ON" : "OFF");
}
void my_therapy_pause(void)
{
    if (s_th.session != TH_RUNNING) return;
    s_th.session = TH_PAUSED;
    ESP_LOGI(TAG, "pause (remain=%dms, step=%d, phase=%s)",
             (int)s_th.remain_ms, (int)s_th.step_idx, s_th.phase == PH_ON ? "ON" : "OFF");
}
void therapy_resume(void)
{
    if (s_th.session != TH_PAUSED) return;
    s_th.session = TH_RUNNING;

    if (phase_is_on() && s_th.ui) {
        wave_apply_start();    /* 重发当前步骤参数 + 启动（0T 方案A） */
    }
    ui_sync();
    ESP_LOGI(TAG, "resume");
}
/* 无条件强制停波 */
void therapy_forced_stop(void)
{
    wave_stop();
    ESP_LOGI(TAG, "forced stop");
}
void therapy_stop(void)
{
    if (s_th.session == TH_IDLE) return;
    wave_stop();
    s_th.session = TH_IDLE;
    if (s_th.ui) s_th.ui->status = TIMER_IDLE;
    ESP_LOGI(TAG, "stop");
}

/* 紧急/兜底停止：无条件停采集(push) + 强制停波 + 会话复位 IDLE。
 * 与 therapy_stop 的区别：不看 session 是否 IDLE —— 即使引擎以为已空闲，
 * 底层 STM32 仍可能因历史漏发/丢帧在输出（STM32 无自保）。
 * 用于"返回主菜单"全局收口，任何路径漏停都在此兜住。幂等，可重复调用。 */
void therapy_emergency_stop(void)
{
    bool need_wave = (s_th.session != TH_IDLE);   /* 引擎认为在跑/暂停/结束 */
    bool need_push = app_modbus_push_active();    /* 采集推流仍开着（可能伴随条件刺激波） */

    /* 无治疗且无采集：什么都不用发（开机首次进菜单走这里，零总线开销/零阻塞） */
    if (!need_wave && !need_push) {
        s_th.chart = NULL;
        s_th.chart_instr = NULL;
        return;
    }

    ESP_LOGW(TAG, "EMERGENCY stop: 强制停采集+停波（返回主菜单兜底, wave=%d push=%d）",
             (int)need_wave, (int)need_push);

    /* 先停采集：内部先 stop_push() 再写 ACQ_CTRL=0，保证后续停波走带重试的确认事务 */
    if (need_push) emg_force_stop_acq();

    /* 强制停波（wave_stop 内部按 push 状态选 nowait 插队 / 确认写+补发） */
    wave_stop();

    s_th.session = TH_IDLE;
    if (s_th.ui) s_th.ui->status = TIMER_IDLE;
    s_th.chart = NULL;
    s_th.chart_instr = NULL;
}

int therapy_set_intensity(int intens_level)
{
    if (intens_level < 0) intens_level = 0;
    if (intens_level > THERAPY_MA_MAX) intens_level = THERAPY_MA_MAX;

    /* 更新当前阶段强度 mA（严格按设置，0 则不输出）
     * 按当前步骤实际所在的强度阶段写入，与 step_intens() 取值口径一致 */
    switch (cur_stim_stage()) {
    case 1:  s_th.intens1 = (uint8_t)intens_level; break;
    case 2:  s_th.intens2 = (uint8_t)intens_level; break;
    default: s_th.intens1 = (uint8_t)intens_level; break;
    }

    if (s_th.ui) s_th.ui->treat_data.Intens_Value = intens_level;

    /* 治疗中且当前步骤 ON：异步下发 mA + 重触发渐变（不卡 LVGL） */
    if (s_th.session == TH_RUNNING && phase_is_on()) {
        int v = intens_level;
        esp_event_post(APP_EVENT, APP_EVENT_TREATY_INTENS, &v, sizeof(v), 0);
    }
    ESP_LOGI(TAG, "set_intensity=%dmA (step%d %s)",
             intens_level, (int)s_th.step_idx, phase_is_on() ? "ON" : "OFF");
    return MB_OK;
}

void therapy_set_scale(uint32_t scale_s)
{
    s_th.scale_s = scale_s ? scale_s : 1;
    ESP_LOGI(TAG, "scale=%d (1 真实秒 = %d 会话秒)", (int)s_th.scale_s, (int)s_th.scale_s);
}

uint32_t therapy_get_scale(void)
{
    return s_th.scale_s;
}

bool therapy_active(void)
{
    return (s_th.session == TH_RUNNING) || (s_th.session == TH_PAUSED);
}

int32_t therapy_remain_ms(void)
{
    return s_th.remain_ms;
}

int therapy_status(void)
{
    switch (s_th.session) {
    case TH_RUNNING: return TIMER_RUNNING;
    case TH_PAUSED:  return TIMER_PAUSED;
    case TH_END:     return TIMER_END;
    default:         return TIMER_IDLE;
    }
}

uint8_t therapy_current_intens(void)
{
    return step_intens();
}

uint16_t therapy_current_freq(void)
{
    const rx_step_t *st = cur_step();
    return st ? st->freq : 0;
}

uint16_t therapy_current_pw(void)
{
    const rx_step_t *st = cur_step();
    return st ? st->pw : 0;
}
uint8_t therapy_current_stage_stim(void)
{
    const rx_step_t *st = cur_step();
    return st ? st->stim_stage : 0;
}
void therapy_print_status(void)
{
    static const char *sn[] = { "IDLE", "RUNNING", "PAUSED", "END" };
    printf("therapy: session=%s step=%d/%d phase=%s remain=%dms (%d:%02d) intens1=%d intens2=%d cur=%d scale=%d\r\n",
           sn[s_th.session], (int)s_th.step_idx, (int)s_th.step_cnt,
           s_th.phase == PH_ON ? "ON" : "OFF", (int)s_th.remain_ms,
           (int)(s_th.remain_ms / 60000), (int)((s_th.remain_ms % 60000) / 1000),
           (int)s_th.intens1, (int)s_th.intens2, (int)step_intens(), (int)s_th.scale_s);
}

uint32_t therapy_get_stim_time(const rx_scheme_t *scheme)
{
    uint8_t stage_cnt = s_th.stage_cnt;
    uint32_t stim_remain_ms = 0;
    for(uint8_t i = 0; i < scheme->stage_cnt;i++) {         //按阶段查找
        if(scheme->stages[i]->type == RX_STEP_STIM ||
            scheme->stages[i]->type == RX_STEP_ACQ)  {
            stim_remain_ms += scheme->stage_time[i];  //电刺激治疗剩余时间 =  数据采集阶段+电刺激治疗阶段     
        }
    }
    return stim_remain_ms;
}
uint16_t therapy_cur_freq(const rx_scheme_t *scheme,uint8_t idx)
{
    for(uint8_t i = 0; i < scheme->stage_cnt;i++) {         //按阶段查找
        if(scheme->stages[i]->type == RX_STEP_STIM) {
            for(uint8_t j = 0; j < scheme->steps_per_stage[i];j++) {    //按阶段内步骤数查找
                if(scheme->stages[i][j].stim_stage == idx) {            //按强度阶段查找
                    return scheme->stages[i][j].freq;
                }  
            } 
        }
    }
    return 0;
}
uint16_t therapy_cur_pw(const rx_scheme_t *scheme,uint8_t idx)
{
    for(uint8_t i = 0; i < scheme->stage_cnt;i++) {
        if(scheme->stages[i]->type == RX_STEP_STIM) {
            for(uint8_t j = 0; j < scheme->steps_per_stage[i];j++) {    //按阶段内步骤数查找
                if(scheme->stages[i][j].stim_stage == idx) {            //按强度阶段查找
                    return scheme->stages[i][j].pw;
                }  
            }   
        }
    }
    return 0;

}
uint16_t therapy_cur_stage_stim(const rx_scheme_t *scheme)
{
    return scheme->stage_stim ;
}
uint32_t therapy_current_stage_time(void)
{
    return s_th.scheme->stage_time[s_th.stage_idx];
}
uint8_t therapy_current_step_cnt(void)
{
    return s_th.scheme->steps_per_stage[s_th.stage_idx];
}

/**
 * @brief 由外部肌电采集/图表刷新调用
 * @param emg_uv 当前实时肌电值，单位 µV
 *
 * 当处于主动训练步骤、且肌电持续低于阈值时，
 * 自动进入条件刺激。
 */
void therapy_emg_feedback(int32_t emg_uv)
{
    if (s_th.session != TH_RUNNING) return;

    const rx_step_t *st = cur_step();
    if (!st) return;

    /* 只在主动训练步骤中判断 */
    if (st->type != RX_STEP_TRAIN) return;
    if (st->train != RX_TRAIN_HOLD &&
        st->train != RX_TRAIN_CONTRACT &&
        st->train != RX_TRAIN_STRONG) {
        return;
    }

    /* 训练刚开始前 1 秒不做判断，避免电极接触/幅度还没稳定 */
    if (s_th.phase_ms < 1000) {
        return;
    }

    /* 根据训练类型选择触发逻辑 */
    switch (st->trigger) {
    /* ============ 类型1: 信号低于参考线(25) → 触发 ============ */
    case RX_TRIGGER_BELOW_REF: {
        if (emg_uv < RX_TRIGGER_REF_LINE) {
            s_th.emg_low_count++;
            /* 连续 10 次(500ms)低于参考线，进入条件刺激 */
            if (s_th.emg_low_count >= 10) {
                s_th.emg_low_count = 0;
                emg_force_stop_acq();
                ESP_LOGI(TAG, "[触发1] EMG=%d < 参考线%d 持续%d次 -> 条件刺激",
                         (int)emg_uv, RX_TRIGGER_REF_LINE, 10);
                enter_next_cond();
            }
        } else {
            s_th.emg_low_count = 0;
        }
        break;
    }

    /* ============ 类型2: 连续3次收缩峰值未超过虚线(25) → 触发 ============ */
    case RX_TRIGGER_3FAIL: {
        /*
         * "一次收缩"的定义：信号从低处上升再回落为一个完整收缩。
         * 用 RX_TRIGGER_CONTRACT_MIN(=5) 作为收缩起始/结束阈值：
         *   - 信号从 <5 升到 >=5 → 收缩开始，记录峰值
         *   - 信号从 >=5 降到 <5 → 收缩结束，比较峰值与参考线
         * 如果峰值 < 参考线(25)，计一次失败；连续 3 次失败 → 触发。
         */
        #define RX_TRIGGER_CONTRACT_MIN  5  /* 收缩检测最小幅度（区分噪声） */

        if (!s_th.emg_in_contraction) {
            /* 当前不在收缩中，检测是否开始新收缩 */
            if (emg_uv >= RX_TRIGGER_CONTRACT_MIN) {
                s_th.emg_in_contraction = 1;
                s_th.emg_peak = emg_uv;
            }
        } else {
            /* 当前在收缩中，更新峰值 */
            if (emg_uv > s_th.emg_peak) {
                s_th.emg_peak = emg_uv;
            }
            /* 检测收缩结束：信号回落到最小幅度以下 */
            if (emg_uv < RX_TRIGGER_CONTRACT_MIN) {
                s_th.emg_in_contraction = 0;
                int32_t peak = s_th.emg_peak;
                s_th.emg_peak = 0;

                if (peak >= RX_TRIGGER_REF_LINE) {
                    /* 峰值超过虚线，本次成功，重置失败计数 */
                    s_th.emg_fail_count = 0;
                    ESP_LOGI(TAG, "[触发2] 收缩峰值=%d >= 虚线%d，重置计数", (int)peak, RX_TRIGGER_REF_LINE);
                } else {
                    /* 峰值未超过虚线，计一次失败 */
                    s_th.emg_fail_count++;
                    ESP_LOGI(TAG, "[触发2] 第%d次收缩峰值=%d < 虚线%d", (int)s_th.emg_fail_count, (int)peak, RX_TRIGGER_REF_LINE);
                    if (s_th.emg_fail_count >= RX_TRIGGER_3FAIL_COUNT) {
                        s_th.emg_fail_count = 0;
                        emg_force_stop_acq();
                        ESP_LOGI(TAG, "[触发2] 连续%d次峰值未超过虚线%d -> 条件刺激",
                                 RX_TRIGGER_3FAIL_COUNT, RX_TRIGGER_REF_LINE);
                        enter_next_cond();
                    }
                }
            }
        }
        break;
    }

    /* ============ 类型3: 信号进入蓝色区域(>75)持续≥2s → 触发 ============ */
    case RX_TRIGGER_ABOVE_BLUE: {
        if (emg_uv > RX_TRIGGER_BLUE_ZONE) {
            if (!s_th.emg_was_above_blue) {
                /* 刚进入蓝色区域，开始计时 */
                s_th.emg_above_blue_ms = 0;
                s_th.emg_was_above_blue = 1;
            } else {
                /* 持续在蓝色区域，累加时间（50ms 周期） */
                s_th.emg_above_blue_ms += 50;
            }
            if (s_th.emg_above_blue_ms >= RX_TRIGGER_BLUE_HOLD_MS) {
                s_th.emg_above_blue_ms = 0;
                s_th.emg_was_above_blue = 0;
                emg_force_stop_acq();
                ESP_LOGI(TAG, "[触发3] EMG=%d > 蓝色区域%d 持续%dms -> 条件刺激",
                         (int)emg_uv, RX_TRIGGER_BLUE_ZONE, RX_TRIGGER_BLUE_HOLD_MS);
                enter_next_cond();
            }
        } else {
            /* 信号离开蓝色区域，重置计时 */
            s_th.emg_above_blue_ms = 0;
            s_th.emg_was_above_blue = 0;
        }
        break;
    }

    default:
        break;
    }
}



