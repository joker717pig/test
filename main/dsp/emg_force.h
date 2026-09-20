/**
 ******************************************************************************
 * @文件名称   emg_force.h
 * @文件描述   肌电力度引擎（阶段 3.2 数据流接线）
 *            封装 emg_dsp 处理链 + 消费 Modbus 采集环形缓冲：
 *               s_acq_ring --(50ms 泵)--> emg_dsp_process --> 力度包络/RMS
 *            对外提供当前力度值，供 UI 力度条 / therapy 收缩放松判断使用。
 ******************************************************************************
 */

#ifndef EMG_FORCE_H
#define EMG_FORCE_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  初始化力度引擎（创建 DSP 链 + 启动 50ms 消费泵）
 * @note   需在 app_modbus_init() 之后调用（泵要读采集环形缓冲）
 */
esp_err_t emg_force_init(void);

/**
 * @brief  设置采样率（启动采集时调用，会同时重置滤波器状态）
 * @param  fs  采样率 (125/250/500/1000)
 */
void emg_force_set_fs(float fs);

/** @brief 重置滤波器状态（重新采集前调用） */
void emg_force_reset(void);

/** @brief 当前力度包络 (整流+EMA, 单位 raw16 LSB, ×6.15 = µV) */
float emg_force_get_env(void);

/** @brief 当前滑动 RMS (单位 raw16 LSB, ×6.15 = µV) */
float emg_force_get_rms(void);

/**
 * @brief  启动采集: 下发 SPS/通道/启动到 STM32 并开启 push 监听
 * @param  sps      采样率 (125/250/500/1000)
 * @param  channel  通道 (1=CH1, 2=CH2, 3=双通道)
 */
esp_err_t emg_force_start_acq(uint16_t sps, uint8_t channel);

/** @brief 停止采集: 关闭 push 监听并停止 STM32 */
esp_err_t emg_force_stop_acq(void);

#ifdef __cplusplus
}
#endif

#endif /* EMG_FORCE_H */
