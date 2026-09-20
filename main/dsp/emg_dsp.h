/**
 ******************************************************************************
 * @文件名称   emg_dsp.h
 * @文件描述   肌电信号处理模块（ESP32 版，与 PC 端 src/emg_dsp.py 一一对应）
 *
 * 处理链（逐样本递推，无 FFT、无批量依赖，便于与 PC 验证结果对齐）:
 *   raw16 ─► 高通(20Hz) ─► 陷波(50Hz Q30) ─► 低通(150Hz) ─► ├ 整流+EMA → 包络(力度)
 *                                                            └ 滑动RMS → 评估值
 *
 * 单位: raw16 是 24bit ADC 右移 8 位的带符号码, 1 LSB ≈ 6.15µV (VREF=2.42V, 增益12)。
 *       包络/RMS 输出同单位 (raw16 LSB), 显示时按需乘 6.15 换算 µV。
 ******************************************************************************
 */

#ifndef EMG_DSP_H
#define EMG_DSP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 滤波器参数 (与 PC 端 emg_dsp.py 默认值一致) ---- */
#define EMG_DSP_HP_FC     25.0f    /* 高通截止频率 Hz (去直流/基线漂移 + 运动伪影) */
#define EMG_DSP_NOTCH_F0  50.0f    /* 陷波中心频率 Hz (工频, 可关)            */
#define EMG_DSP_NOTCH_Q   30.0f    /* 陷波 Q 值                                */
#define EMG_DSP_LP_FC     150.0f   /* 低通截止频率 Hz (滤 200Hz 开关电源干扰)  */
#define EMG_DSP_LP_Q      0.707f   /* 低通 Q (Butterworth)                    */
#define EMG_DSP_EMA_FC    4.0f     /* 力度包络等效截止频率 Hz                 */
#define EMG_DSP_RMS_WIN   0.2f     /* 评估 RMS 滑动窗口 (秒)                  */

/* RMS 滑动窗最大长度 (fs=1000Hz × 0.2s = 200 样本, 留余量) */
#define EMG_DSP_RMS_MAX_WIN 256

/* ---- 二阶 IIR (Transposed Direct Form II), 与 PC Biquad 一致 ---- */
typedef struct {
    float b0, b1, b2;   /* 分子系数 */
    float a1, a2;       /* 分母系数 (a0 已归一为 1) */
    float x1, x2;       /* 输入历史 */
    float y1, y2;       /* 输出历史 */
} emg_dsp_biquad_t;

/* ---- 完整处理链状态 (对应 PC EmgProcessor) ---- */
typedef struct {
    float fs;                       /* 采样率 */

    /* 高通 (二阶 biquad, 更狠滤运动伪影) */
    emg_dsp_biquad_t hp;

    /* 陷波 + 低通 */
    emg_dsp_biquad_t notch;
    emg_dsp_biquad_t lp;
    bool notch_enable;

    /* EMA 包络 */
    float ema_alpha, ema_y;

    /* 滑动 RMS */
    float rms_buf[EMG_DSP_RMS_MAX_WIN];
    uint16_t rms_win, rms_idx;
    float rms_sum_sq;
} emg_dsp_t;

/**
 * @brief  初始化处理链 (按采样率计算全部滤波系数)
 * @param  d   处理链句柄
 * @param  fs  采样率 (125/250/500/1000)
 */
void emg_dsp_init(emg_dsp_t *d, float fs);

/** @brief 清空滤波器状态 (重新采集前调用) */
void emg_dsp_reset(emg_dsp_t *d);

/**
 * @brief  喂入一个 raw16 样本, 输出力度包络与 RMS
 * @param  d    处理链句柄
 * @param  raw  raw16 带符号样本 (int16)
 * @param  env  输出: 整流+EMA 包络 (力度值, raw16 LSB)
 * @param  rms  输出: 滑动窗口 RMS (raw16 LSB)
 */
void emg_dsp_process(emg_dsp_t *d, int16_t raw, float *env, float *rms);

/** @brief raw16 LSB → µV (VREF=2.42V, 增益12) */
#define EMG_DSP_RAW16_LSB_UV 6.15f

#ifdef __cplusplus
}
#endif

#endif /* EMG_DSP_H */
