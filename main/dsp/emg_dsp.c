/**
 ******************************************************************************
 * @文件名称   emg_dsp.c
 * @文件描述   肌电信号处理模块实现（与 PC 端 src/emg_dsp.py 逐行对应）
 ******************************************************************************
 */

#include "emg_dsp.h"
#include <math.h>
#include <string.h>

#define PI_F 3.14159265358979323846f

/* ---- 内部: 二阶 IIR 递推 (TDF2), 与 PC Biquad.step 一致 ---- */
static inline float biquad_step(emg_dsp_biquad_t *f, float x)
{
    float y = f->b0 * x + f->x1;
    f->x1 = f->b1 * x - f->a1 * y + f->x2;
    f->x2 = f->b2 * x - f->a2 * y;
    return y;
}

/* ---- 内部: 设计陷波系数 (与 PC design_notch 一致) ---- */
static void biquad_design_notch(emg_dsp_biquad_t *f, float fs, float f0, float q)
{
    float w0 = 2.0f * PI_F * f0 / fs;
    float cos_w0 = cosf(w0);
    float alpha = sinf(w0) / (2.0f * q);
    float a0 = 1.0f + alpha;
    f->b0 = 1.0f / a0;
    f->b1 = -2.0f * cos_w0 / a0;
    f->b2 = 1.0f / a0;
    f->a1 = -2.0f * cos_w0 / a0;
    f->a2 = (1.0f - alpha) / a0;
}

/* ---- 内部: 设计低通系数 (与 PC design_lowpass 一致) ---- */
static void biquad_design_lowpass(emg_dsp_biquad_t *f, float fs, float fc, float q)
{
    float w0 = 2.0f * PI_F * fc / fs;
    float cos_w0 = cosf(w0);
    float alpha = sinf(w0) / (2.0f * q);
    float a0 = 1.0f + alpha;
    f->b0 = (1.0f - cos_w0) / 2.0f / a0;
    f->b1 = (1.0f - cos_w0) / a0;
    f->b2 = (1.0f - cos_w0) / 2.0f / a0;
    f->a1 = -2.0f * cos_w0 / a0;
    f->a2 = (1.0f - alpha) / a0;
}

/* ---- 内部: 设计高通系数 (二阶 Butterworth, 滤运动伪影) ---- */
static void biquad_design_highpass(emg_dsp_biquad_t *f, float fs, float fc, float q)
{
    float w0 = 2.0f * PI_F * fc / fs;
    float cos_w0 = cosf(w0);
    float alpha = sinf(w0) / (2.0f * q);
    float a0 = 1.0f + alpha;
    f->b0 = (1.0f + cos_w0) / 2.0f / a0;
    f->b1 = -(1.0f + cos_w0) / a0;
    f->b2 = (1.0f + cos_w0) / 2.0f / a0;
    f->a1 = -2.0f * cos_w0 / a0;
    f->a2 = (1.0f - alpha) / a0;
}

void emg_dsp_init(emg_dsp_t *d, float fs)
{
    if (d == NULL || fs <= 0.0f) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->fs = fs;
    d->notch_enable = true;

    /* 高通 (二阶 Butterworth, 滤运动伪影) */
    biquad_design_highpass(&d->hp, fs, EMG_DSP_HP_FC, 0.707f);

    /* 陷波 50Hz */
    biquad_design_notch(&d->notch, fs, EMG_DSP_NOTCH_F0, EMG_DSP_NOTCH_Q);

    /* 低通 150Hz */
    biquad_design_lowpass(&d->lp, fs, EMG_DSP_LP_FC, EMG_DSP_LP_Q);

    /* EMA: alpha = 2π fc / fs */
    d->ema_alpha = 2.0f * PI_F * EMG_DSP_EMA_FC / fs;

    /* RMS 窗口 (样本数) */
    d->rms_win = (uint16_t)(EMG_DSP_RMS_WIN * fs + 0.5f);
    if (d->rms_win < 1) {
        d->rms_win = 1;
    }
    if (d->rms_win > EMG_DSP_RMS_MAX_WIN) {
        d->rms_win = EMG_DSP_RMS_MAX_WIN;
    }
}

void emg_dsp_reset(emg_dsp_t *d)
{
    if (d == NULL) {
        return;
    }
    d->hp.x1 = d->hp.x2 = d->hp.y1 = d->hp.y2 = 0.0f;
    d->notch.x1 = d->notch.x2 = d->notch.y1 = d->notch.y2 = 0.0f;
    d->lp.x1 = d->lp.x2 = d->lp.y1 = d->lp.y2 = 0.0f;
    d->ema_y = 0.0f;
    memset(d->rms_buf, 0, sizeof(d->rms_buf));
    d->rms_idx = 0;
    d->rms_sum_sq = 0.0f;
}

void emg_dsp_process(emg_dsp_t *d, int16_t raw, float *env, float *rms)
{
    float x = (float)raw;
    float y;

    if (d == NULL) {
        if (env) *env = 0.0f;
        if (rms) *rms = 0.0f;
        return;
    }

    /* 1. 二阶高通 (去直流/基线漂移 + 运动伪影) */
    y = biquad_step(&d->hp, x);

    /* 2. 陷波 50Hz */
    if (d->notch_enable) {
        y = biquad_step(&d->notch, y);
    }

    /* 3. 低通 150Hz (滤 200Hz 干扰) */
    y = biquad_step(&d->lp, y);

    /* 4. 整流 + EMA → 包络 (力度) */
    if (env) {
        d->ema_y = d->ema_alpha * fabsf(y) + (1.0f - d->ema_alpha) * d->ema_y;
        *env = d->ema_y;
    }

    /* 5. 滑动 RMS (用带符号滤波后信号) */
    if (rms) {
        float old = d->rms_buf[d->rms_idx];
        d->rms_sum_sq += y * y - old * old;
        d->rms_buf[d->rms_idx] = y;
        d->rms_idx = (d->rms_idx + 1) % d->rms_win;
        *rms = sqrtf(d->rms_sum_sq / (float)d->rms_win);
    }
}
