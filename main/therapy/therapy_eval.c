/**
  ******************************************************************************
  * @文件名称   therapy_eval.c
  * @文件描述   盆底评估协议（功能2）：4 阶段 + 总得分（结构框架）
  *             数据源：EDA_EMG/doc/治疗处方/利玮生物刺激反馈仪处方2026052501.md
  *
  *             阶段核对（与 md 全表一致）：
  *               前静息期  静息10s x2          → 平均值/变异性
  *               快速收缩期 用力10s+静息10s x5  → 最大值/上升时间/恢复时间
  *               持续收缩期 (静息2+用力1+保持6+放松1+静息2=12s+静息10s) x6 → 平均值/变异性
  *               后静息期  静息10s x2          → 平均值/变异性
  *               总得分（无动作）
  *             动作步骤用 rx_step_t 表达：WAIT=静息，TRAIN=用力/保持（voice=0 无语音）
  *             参考值/测试结果/得分留空（框架，后续填）
  ******************************************************************************
  */
#include "therapy_eval.h"
#include "emg_force.h"
#include "emg_dsp.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "voice.h"      /* 评估语音提示（audio_play_request） */
#include <string.h>
#include <math.h>

/* 前静息期：静息10s */
static const rx_step_t eval_rest_front_steps[] = {
    {RX_STEP_WAIT, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_RELAX_CN, 0},  /* 放松 10s */
};
static const rx_eval_metric_t eval_rest_front_metrics[] = { RX_EVAL_AVG, RX_EVAL_VAR };

/* 快速收缩期：用力10s + 静息10s */
static const rx_step_t eval_fast_steps[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_CONTRACT_CN, RX_TRAIN_CONTRACT},  /* 用力 10s */
    {RX_STEP_WAIT,  0, 0, 0, 0, 0, 0, 10, VOICE_ID_RELAX_CN, 0},                     /* 放松 10s */
};
static const rx_eval_metric_t eval_fast_metrics[] = { RX_EVAL_MAX, RX_EVAL_RISE, RX_EVAL_RECOVER };

/* 持续收缩期：静息2 + 用力保持8(用力1+保持6+放松1) + 静息12(静息2+静息10)，重复 6 次 */
static const rx_step_t eval_sustain_steps[] = {
    {RX_STEP_WAIT,  0, 0, 0, 0, 0, 0, 2,  VOICE_ID_RELAX_CN, 0},                  /* 放松 2s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8,  VOICE_ID_HOLD_CONTRACT_CN, RX_TRAIN_HOLD},  /* 保持用力 8s */
    {RX_STEP_WAIT,  0, 0, 0, 0, 0, 0, 12, VOICE_ID_RELAX_CN, 0},                  /* 放松 12s */
};
static const rx_eval_metric_t eval_sustain_metrics[] = { RX_EVAL_AVG, RX_EVAL_VAR };

/* 后静息期：静息10s */
static const rx_step_t eval_rest_back_steps[] = {
    {RX_STEP_WAIT, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_RELAX_CN, 0},  /* 放松 10s */
};
static const rx_eval_metric_t eval_rest_back_metrics[] = { RX_EVAL_AVG, RX_EVAL_VAR };

static const rx_eval_phase_t eval_phases[] = {
    { "前静息期",  1, eval_rest_front_steps, 2, 2, eval_rest_front_metrics },
    { "快速收缩期", 2, eval_fast_steps,       5, 3, eval_fast_metrics },
    { "持续收缩期", 3, eval_sustain_steps,    6, 2, eval_sustain_metrics },
    { "后静息期",  1, eval_rest_back_steps,  2, 2, eval_rest_back_metrics },
};

const rx_eval_t g_eval = { "盆底评估", 4, eval_phases };

/* ===================== 评估执行引擎 ===================== */

static const char *TAG = "eval";

#define EVAL_TICK_MS      50            /* tick 周期 */
#define EVAL_TICKS_PER_S  (1000 / EVAL_TICK_MS)  /* 20 采样/秒 */
#define EVAL_MIN_THR_UV   10.0f         /* 上升/恢复检测阈值下限（µV） */
#define EVAL_SETTLE_S     0             /* [我们] 前静息开头跳过的 settling 秒数（0=取消；与 emg_force 的 EMG_FORCE_SETTLE_MS 一致） */

typedef struct {
    eval_state_t state;
    uint8_t      phase;                 /* 当前阶段 idx（0~3） */
    uint8_t      step;                  /* 当前步骤 idx */
    uint8_t      rep;                   /* 当前 repeat idx */
    uint16_t     tick_in_step;          /* 步骤内 tick 计数 */
    uint16_t     step_ticks;            /* 当前步骤总 tick */
    uint16_t     phase_remain_ticks;    /* 阶段剩余 tick（倒计时） */
    uint16_t     phase_total_ticks;     /* 阶段总 tick（进度分母） */
    uint16_t     total_remain_ticks;    /* [我们] 全程总剩余 tick（UI 总倒计时） */
    uint16_t     settle_ticks;          /* [我们] 前静息开头要跳过的采样 tick（硬件/DSP settling） */

    /* 静息统计（前/后静息期共用，阶段切换时清零） */
    double       sum, sumsq;
    uint32_t     n;

    /* 快速收缩期：当前用力段流式检测 */
    float        fast_seg_max;          /* 当前段峰值 */
    uint16_t     fast_seg_peak_t;       /* 峰值时刻（段内 tick） */
    uint16_t     fast_seg_rise_t;       /* 首超阈值时刻 */
    uint16_t     fast_seg_rec_t;        /* 回落阈值时刻 */
    uint8_t      fast_seg_state;        /* 0=未触发 1=上升/已触发 2=已回落 */
    float        fast_best_max;         /* 5 段峰值最大者 */
    uint32_t     fast_rise_acc;         /* 上升时间累加（tick） */
    uint32_t     fast_rec_acc;          /* 恢复时间累加（tick） */
    uint8_t      fast_seg_cnt;          /* 已完成用力段数 */

    /* 持续收缩期：保持段统计 */
    double       sust_sum, sust_sumsq;
    uint32_t     sust_n;

    esp_timer_handle_t timer;
    eval_result_t      res;
} eval_ctx_t;

static eval_ctx_t s_eval;

/* ---- 内部：变异系数 CV% = std/mean x100 ---- */
static float eval_calc_var(double sum, double sumsq, uint32_t n)
{
    if (n < 2) return 0.0f;
    double mean = sum / n;
    if (mean < 0.5) return 0.0f;              /* 均值接近 0 无意义 */
    double var = sumsq / n - mean * mean;
    if (var < 0.0) var = 0.0;
    return (float)(sqrt(var) / mean * 100.0);
}

/* ---- 内部：clamp 到 [0,1] ---- */
static float eval_clamp01(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

/* ---- 内部：线性映射 score（full=满分阈值，zero=零分阈值） ---- */
static float eval_lin_score(float v, float full, float zero)
{
    if (zero == full) return 100.0f;
    return eval_clamp01((zero - v) / (zero - full)) * 100.0f;
}

/* ---- 内部：快速收缩用力段单次采样 ---- */
static void eval_fast_sample(float env)
{
    float thr = s_eval.res.baseline * 2.0f;
    if (thr < EVAL_MIN_THR_UV) thr = EVAL_MIN_THR_UV;

    /* 峰值追踪 */
    if (env > s_eval.fast_seg_max) {
        s_eval.fast_seg_max = env;
        s_eval.fast_seg_peak_t = s_eval.tick_in_step;
    }

    /* 上升/恢复状态机 */
    if (s_eval.fast_seg_state == 0) {
        if (env > thr) {
            s_eval.fast_seg_state = 1;
            s_eval.fast_seg_rise_t = s_eval.tick_in_step;
        }
    } else if (s_eval.fast_seg_state == 1) {
        if (env < thr) {
            s_eval.fast_seg_state = 2;
            s_eval.fast_seg_rec_t = s_eval.tick_in_step;
        }
    }
}

/* ---- 内部：单次采样分发（按阶段+步骤类型统计） ---- */
static void eval_step_sample(const rx_step_t *st, float env)
{
    switch (s_eval.phase) {
    case 0:  /* 前静息：WAIT 统计 AVG/VAR */
    case 3:  /* 后静息：WAIT 统计 AVG/VAR */
        if (st->type == RX_STEP_WAIT) {
            s_eval.sum += env;
            s_eval.sumsq += env * env;
            s_eval.n++;
        }
        break;
    case 1:  /* 快速收缩：TRAIN 段峰值/上升/恢复检测 */
        if (st->type == RX_STEP_TRAIN) {
            eval_fast_sample(env);
        }
        break;
    case 2:  /* 持续收缩：TRAIN 保持段统计 AVG/VAR */
        if (st->type == RX_STEP_TRAIN) {
            s_eval.sust_sum += env;
            s_eval.sust_sumsq += env * env;
            s_eval.sust_n++;
        }
        break;
    default:
        break;
    }
}

/* ---- 内部：步骤结束结算（快速收缩用力段出账） ---- */
static void eval_step_end(const rx_step_t *st)
{
    if (s_eval.phase != 1 || st->type != RX_STEP_TRAIN) return;

    uint16_t rise = 0, rec = 0;
    if (s_eval.fast_seg_state >= 1) {
        rise = s_eval.fast_seg_peak_t - s_eval.fast_seg_rise_t;
        if (s_eval.fast_seg_state >= 2) {
            rec = s_eval.fast_seg_rec_t - s_eval.fast_seg_peak_t;
        }
    }
    s_eval.fast_rise_acc += rise;
    s_eval.fast_rec_acc += rec;
    if (s_eval.fast_seg_max > s_eval.fast_best_max) {
        s_eval.fast_best_max = s_eval.fast_seg_max;
    }
    s_eval.fast_seg_cnt++;

    /* 重置段检测状态 */
    s_eval.fast_seg_max = 0;
    s_eval.fast_seg_peak_t = 0;
    s_eval.fast_seg_rise_t = 0;
    s_eval.fast_seg_rec_t = 0;
    s_eval.fast_seg_state = 0;
}

/* ---- 内部：初始化一个阶段的计时与统计变量 ---- */
static void eval_init_phase(uint8_t phase)
{
    const rx_eval_phase_t *ph = &g_eval.phases[phase];
    uint32_t total = 0;
    for (uint8_t i = 0; i < ph->step_cnt; i++) {
        total += (uint32_t)ph->steps[i].dur_s * EVAL_TICKS_PER_S;
    }
    total *= ph->repeat;

    s_eval.phase = phase;
    s_eval.step = 0;
    s_eval.rep = 0;
    s_eval.tick_in_step = 0;
    s_eval.step_ticks = (uint16_t)((uint32_t)ph->steps[0].dur_s * EVAL_TICKS_PER_S);
    s_eval.phase_total_ticks = (uint16_t)total;
    s_eval.phase_remain_ticks = (uint16_t)total;

    /* [我们] 关键修复：同步运行状态 —— UI 依赖 eval_get_state() 检测阶段切换来清空波形 */
    switch (phase) {
    case 0:  s_eval.state = EVAL_REST1; break;
    case 1:  s_eval.state = EVAL_FAST;  break;
    case 2:  s_eval.state = EVAL_SUST;  break;
    case 3:  s_eval.state = EVAL_REST2; break;
    default: break;
    }

    /* 按阶段初始化统计变量 */
    switch (phase) {
    case 0:  /* 前静息 */
    case 3:  /* 后静息 */
        s_eval.sum = s_eval.sumsq = 0;
        s_eval.n = 0;
        break;
    case 1:  /* 快速收缩 */
        s_eval.fast_best_max = 0;
        s_eval.fast_rise_acc = s_eval.fast_rec_acc = 0;
        s_eval.fast_seg_cnt = 0;
        s_eval.fast_seg_state = 0;
        break;
    case 2:  /* 持续收缩 */
        s_eval.sust_sum = s_eval.sust_sumsq = 0;
        s_eval.sust_n = 0;
        break;
    }
}

/* ---- 内部：阶段结束算指标 ---- */
static void eval_finish_phase(void)
{
    float base = s_eval.res.baseline;
    switch (s_eval.phase) {
    case 0: {
        float avg = s_eval.n ? (float)(s_eval.sum / s_eval.n) : 0.0f;
        s_eval.res.rest1_avg = avg;
        s_eval.res.rest1_var = eval_calc_var(s_eval.sum, s_eval.sumsq, s_eval.n);
        s_eval.res.baseline = avg;    /* 相对标定基线 */
        ESP_LOGI(TAG, "前静息: avg=%.1fµV var=%.1f%% (基线)", avg, s_eval.res.rest1_var);
        break;
    }
    case 1: {
        s_eval.res.fast_max = s_eval.fast_best_max;
        s_eval.res.fast_max_ratio = (base > 1.0f) ? s_eval.fast_best_max / base : 0.0f;
        uint8_t seg = s_eval.fast_seg_cnt ? s_eval.fast_seg_cnt : 1;
        s_eval.res.fast_rise = (float)s_eval.fast_rise_acc / seg / EVAL_TICKS_PER_S;
        s_eval.res.fast_recover = (float)s_eval.fast_rec_acc / seg / EVAL_TICKS_PER_S;
        ESP_LOGI(TAG, "快速收缩: max=%.1fµV(%.1fx) rise=%.2fs rec=%.2fs",
                 s_eval.res.fast_max, s_eval.res.fast_max_ratio,
                 s_eval.res.fast_rise, s_eval.res.fast_recover);
        break;
    }
    case 2: {
        float avg = s_eval.sust_n ? (float)(s_eval.sust_sum / s_eval.sust_n) : 0.0f;
        s_eval.res.sust_avg = avg;
        s_eval.res.sust_ratio = (base > 1.0f) ? avg / base : 0.0f;
        s_eval.res.sust_var = eval_calc_var(s_eval.sust_sum, s_eval.sust_sumsq, s_eval.sust_n);
        ESP_LOGI(TAG, "持续收缩: avg=%.1fµV(%.1fx) var=%.1f%%",
                 avg, s_eval.res.sust_ratio, s_eval.res.sust_var);
        break;
    }
    case 3: {
        float avg = s_eval.n ? (float)(s_eval.sum / s_eval.n) : 0.0f;
        s_eval.res.rest2_avg = avg;
        s_eval.res.rest2_var = eval_calc_var(s_eval.sum, s_eval.sumsq, s_eval.n);
        ESP_LOGI(TAG, "后静息: avg=%.1fµV var=%.1f%%", avg, s_eval.res.rest2_var);
        break;
    }
    }
}

/* ---- 内部：总得分（相对标定加权评分，规则可后续按临床标准调整） ---- */
static uint8_t eval_compute_score(void)
{
    eval_result_t *r = &s_eval.res;

    /* 是否有真实用力/保持：未收缩（峰值 < 2 倍基线）或未保持（均值 < 1.5 倍基线）时对应项记 0，
     * 避免"全程静息"却因 0s 上升/恢复时间、低 CV 而拿到速度/稳定高分 */
    bool has_force = r->fast_max_ratio >= 2.0f;
    bool has_sust  = r->sust_ratio >= 1.5f;

    /* 力量（25%）：峰值/基线倍数，10 倍满分；未收缩记 0 */
    float p_force = has_force ? eval_clamp01(r->fast_max_ratio / 10.0f) * 100.0f : 0.0f;

    /* 耐力（25%）：保持段均值/基线倍数，5 倍满分；未保持记 0 */
    float p_endu = has_sust ? eval_clamp01(r->sust_ratio / 5.0f) * 100.0f : 0.0f;

    /* 速度（20%）：上升 ≤0.5s 满分 ≥2s 零分；恢复 ≤1s 满分 ≥3s 零分；未收缩记 0 */
    float p_rise = has_force ? eval_lin_score(r->fast_rise, 0.5f, 2.0f) : 0.0f;
    float p_rec  = has_force ? eval_lin_score(r->fast_recover, 1.0f, 3.0f) : 0.0f;

    /* 静息稳定（15%）：前/后静息 CV% 平均，≤10% 满分 ≥30% 零分（静息本身就该稳，不需收缩门槛） */
    float rest_cv = (r->rest1_var + r->rest2_var) * 0.5f;
    float p_rest = eval_lin_score(rest_cv, 10.0f, 30.0f);

    /* 保持稳定（15%）：持续收缩 CV%，≤10% 满分 ≥30% 零分；未保持记 0 */
    float p_sust = has_sust ? eval_lin_score(r->sust_var, 10.0f, 30.0f) : 0.0f;

    float total = p_force * 0.25f + p_endu * 0.25f
                + p_rise  * 0.10f + p_rec  * 0.10f
                + p_rest  * 0.15f + p_sust * 0.15f;

    /* 存分项得分（报告页显示） */
    r->score_force   = (uint8_t)p_force;
    r->score_endu    = (uint8_t)p_endu;
    r->score_rise    = (uint8_t)p_rise;
    r->score_recover = (uint8_t)p_rec;
    r->score_rest    = (uint8_t)p_rest;
    r->score_sust    = (uint8_t)p_sust;

    ESP_LOGI(TAG, "得分: force=%.0f endu=%.0f rise=%.0f rec=%.0f rest=%.0f sust=%.0f => %d",
             p_force, p_endu, p_rise, p_rec, p_rest, p_sust, (int)total);
    return (uint8_t)total;
}

/* ---- 内部：步骤动作显示名（与语音提示同一语义） ---- */
static const char *eval_step_action_name(const rx_step_t *st)
{
    if (st->type == RX_STEP_TRAIN) {
        return (st->train == RX_TRAIN_HOLD) ? "保持" : "收缩";
    }
    return "放松";   /* WAIT = 静息 */
}

/* ---- 内部：播放当前步骤的语音提示 ---- */
static void eval_play_step_voice(void)
{
    if (s_eval.phase >= g_eval.phase_cnt) return;
    const rx_eval_phase_t *ph = &g_eval.phases[s_eval.phase];
    const rx_step_t *st = &ph->steps[s_eval.step];
    if (st->voice != 0) {
        audio_play_request(st->voice, 1500);
    }
}

/* ---- 内部：阶段推进 ---- */
static void eval_advance_step(void)
{
    const rx_eval_phase_t *ph = &g_eval.phases[s_eval.phase];

    s_eval.step++;
    if (s_eval.step >= ph->step_cnt) {
        s_eval.step = 0;
        s_eval.rep++;
        if (s_eval.rep >= ph->repeat) {
            /* 本阶段结束 */
            eval_finish_phase();
            s_eval.phase++;
            if (s_eval.phase >= g_eval.phase_cnt) {
                /* 全部结束 */
                s_eval.res.total = eval_compute_score();
                s_eval.state = EVAL_DONE;
                if (s_eval.timer) esp_timer_stop(s_eval.timer);
                audio_play_request(VOICE_ID_DONE_CN, 1500);   /* 评估完成 */
                ESP_LOGI(TAG, "评估完成: 总得分=%d", s_eval.res.total);
                return;
            }
            eval_init_phase(s_eval.phase);
            eval_play_step_voice();   /* 新阶段第一个动作 */
            return;
        }
    }
    s_eval.tick_in_step = 0;
    s_eval.step_ticks = (uint16_t)((uint32_t)ph->steps[s_eval.step].dur_s * EVAL_TICKS_PER_S);
    eval_play_step_voice();   /* 新动作 */
}

/* ---- 内部：50ms tick ---- */
static void eval_tick_cb(void *arg)
{
    if (s_eval.state != EVAL_REST1 &&
        s_eval.state != EVAL_FAST &&
        s_eval.state != EVAL_SUST &&
        s_eval.state != EVAL_REST2) {
        return;
    }

    const rx_eval_phase_t *ph = &g_eval.phases[s_eval.phase];
    const rx_step_t *st = &ph->steps[s_eval.step];

    /* raw16 LSB -> µV */
    float env = emg_force_get_env() * EMG_DSP_RAW16_LSB_UV;

    if (s_eval.settle_ticks > 0) {
        s_eval.settle_ticks--;   /* settling：跳过前静息开头瞬态，不参与指标统计 */
    } else {
        eval_step_sample(st, env);
    }

    s_eval.tick_in_step++;
    if (s_eval.phase_remain_ticks > 0) s_eval.phase_remain_ticks--;
    if (s_eval.total_remain_ticks > 0) s_eval.total_remain_ticks--;

    if (s_eval.tick_in_step >= s_eval.step_ticks) {
        eval_step_end(st);
        eval_advance_step();
    }
}

/* ====== =============== 对外接口 ===================== */

esp_err_t eval_start(void)
{
    esp_timer_handle_t t = s_eval.timer;
    memset(&s_eval, 0, sizeof(s_eval));
    s_eval.timer = t;

    if (s_eval.timer == NULL) {
        esp_timer_create_args_t args = {
            .callback = eval_tick_cb,
            .arg = NULL,
            .name = "eval_tick",
        };
        esp_err_t err = esp_timer_create(&args, &s_eval.timer);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "timer create fail: %d", err);
            return err;
        }
    }

    s_eval.state = EVAL_REST1;
    eval_init_phase(0);
    s_eval.settle_ticks = (uint16_t)(EVAL_SETTLE_S * EVAL_TICKS_PER_S);

    audio_play_request(VOICE_ID_ASSESS_START_CN, 1500);   /* 开始评估 */
    eval_play_step_voice();                               /* 前静息第一个动作：放松 */

    /* [我们] 计算全程总 tick（4 阶段合计，UI 总倒计时从约 4:32 起递减） */
    uint32_t total_ticks = 0;
    for (uint8_t i = 0; i < g_eval.phase_cnt; i++) {
        const rx_eval_phase_t *p = &g_eval.phases[i];
        uint32_t t = 0;
        for (uint8_t j = 0; j < p->step_cnt; j++) {
            t += (uint32_t)p->steps[j].dur_s * EVAL_TICKS_PER_S;
        }
        t *= p->repeat;
        total_ticks += t;
    }
    s_eval.total_remain_ticks = (uint16_t)total_ticks;

    ESP_LOGI(TAG, "评估开始（共 %d 阶段，总 %lu s）", g_eval.phase_cnt,
             (unsigned long)(total_ticks / EVAL_TICKS_PER_S));
    return esp_timer_start_periodic(s_eval.timer, EVAL_TICK_MS * 1000);
}

void eval_stop(void)
{
    if (s_eval.timer) esp_timer_stop(s_eval.timer);
    s_eval.state = EVAL_IDLE;
    ESP_LOGI(TAG, "评估停止");
}

eval_state_t eval_get_state(void)
{
    return s_eval.state;
}

const char *eval_phase_name(void)
{
    if (s_eval.state == EVAL_IDLE) return "未开始";
    if (s_eval.state == EVAL_DONE) return "评估完成";
    if (s_eval.phase < g_eval.phase_cnt) return g_eval.phases[s_eval.phase].name;
    return "评估中";
}

uint16_t eval_remain_s(void)
{
    if (s_eval.state == EVAL_IDLE || s_eval.state == EVAL_DONE) return 0;
    return s_eval.phase_remain_ticks / EVAL_TICKS_PER_S;
}

uint16_t eval_total_remain_s(void)
{
    if (s_eval.state == EVAL_IDLE || s_eval.state == EVAL_DONE) return 0;
    return s_eval.total_remain_ticks / EVAL_TICKS_PER_S;
}

uint16_t eval_phase_elapsed_ticks(void)
{
    if (s_eval.state == EVAL_IDLE || s_eval.state == EVAL_DONE) return 0;
    return (uint16_t)(s_eval.phase_total_ticks - s_eval.phase_remain_ticks);
}

uint16_t eval_phase_total_s(void)
{
    if (s_eval.state == EVAL_IDLE || s_eval.state == EVAL_DONE) return 0;
    return s_eval.phase_total_ticks / EVAL_TICKS_PER_S;
}

const char *eval_action_name(void)
{
    if (s_eval.state == EVAL_IDLE) return "无";
    if (s_eval.state == EVAL_DONE) return "完成";
    if (s_eval.phase >= g_eval.phase_cnt) return "无";
    return eval_step_action_name(&g_eval.phases[s_eval.phase].steps[s_eval.step]);
}

const char *eval_next_action_name(void)
{
    if (s_eval.state == EVAL_IDLE) return "无";
    if (s_eval.state == EVAL_DONE) return "完成";

    uint8_t phase = s_eval.phase;
    uint8_t step  = s_eval.step;
    uint8_t rep   = s_eval.rep;
    const rx_eval_phase_t *ph = &g_eval.phases[phase];

    step++;
    if (step >= ph->step_cnt) {
        step = 0;
        rep++;
        if (rep >= ph->repeat) {
            phase++;
            rep = 0;
            if (phase >= g_eval.phase_cnt) return "完成";
            ph = &g_eval.phases[phase];
        }
    }
    return eval_step_action_name(&ph->steps[step]);
}

rx_train_kind_t eval_action_kind(void)
{
    if (s_eval.state == EVAL_IDLE || s_eval.state == EVAL_DONE || s_eval.phase >= g_eval.phase_cnt) return RX_TRAIN_RELAX;
    const rx_step_t *st = &g_eval.phases[s_eval.phase].steps[s_eval.step];
    return (st->type == RX_STEP_TRAIN) ? (rx_train_kind_t)st->train : RX_TRAIN_RELAX;
}

rx_train_kind_t eval_next_action_kind(void)
{
    if (s_eval.state == EVAL_IDLE || s_eval.state == EVAL_DONE || s_eval.phase >= g_eval.phase_cnt) return RX_TRAIN_RELAX;
    uint8_t phase = s_eval.phase, step = s_eval.step, rep = s_eval.rep;
    const rx_eval_phase_t *ph = &g_eval.phases[phase];
    if (++step >= ph->step_cnt) {
        step = 0;
        if (++rep >= ph->repeat) { phase++; rep = 0; if (phase >= g_eval.phase_cnt) return RX_TRAIN_RELAX; ph = &g_eval.phases[phase]; }
    }
    const rx_step_t *st = &ph->steps[step];
    return (st->type == RX_STEP_TRAIN) ? (rx_train_kind_t)st->train : RX_TRAIN_RELAX;
}

const eval_result_t *eval_get_result(void)
{
    return &s_eval.res;
}
