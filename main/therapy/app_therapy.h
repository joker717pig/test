/**
  ******************************************************************************
  * @文件名称   app_therapy.h
  * @文件描述   治疗控制【模板】（阶段C）：从处方表下发真实治疗命令 + 会话状态机
  *
  *             ================== 模板使用指南（同事接入新疗程） ==================
  *             1. 处方数据：已在 main/therapy/therapy_rx.c 的 g_rx_all[] 表格
  *                （处方1/2/3/4，每处方多疗程；每疗程 = 有序步骤数组 rx_step_t）
  *             2. 选择疗程：在 UI 开始治疗处（PelFlo_TheChild_event.c 的 Start）
  *                或 console（app_console.c `therapy start`）传入
  *                `&g_rx_all[RX_x]->schemes[疗程idx]` 即可
  *             3. 会话时长：THERAPY_TOTAL_MS（默认 30 分钟，模板常量，按需改）
  *             4. 步骤执行（模板核心）：按 scheme->steps[] **顺序循环**——
  *                每个 STIM 步骤：刺激 on_s 秒 -> 休息 off_s 秒 -> 下一步，循环到总时长结束
  *                （例 处方3疗程1：步骤0 4Hz/500us 28s/2s -> 步骤1 50Hz/300us 8s/2s -> 步骤0 ...）
 *             5. 强度映射：阶段1 -> intens1、阶段2 -> intens2（UI 直接选 0~90，原值即 mA
 *                下发，不再换算；严格按设置，0 不输出）。超过 2 阶段需扩展 step_intens()
  *             6. 模板只执行 STIM 步骤；ACQ/TRAIN/COND/WAIT 步骤（采集/训练/条件刺激）
  *                模板未实现，同事按需在状态机推进处扩展
  *             ===============================================================
  *
  *             异步下发：esp_timer 1s(真实) -> APP_EVENT_TREATY_POLL -> 事件任务推进
  *             时间缩放：therapy_set_scale(N) = 1 真实秒 = N 会话秒（联调验证用，默认 1）
  *             下发参考：EDA_EMG_PC/src/ui/main_window.py::_on_stim_params
  ******************************************************************************
  */
#ifndef APP_THERAPY_H
#define APP_THERAPY_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "therapy_rx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 模板默认会话总时长（单位 ms）：默认 30 分钟，新疗程按需修改 */
#define THERAPY_TOTAL_MS  (30 * 60 * 1000)
#define STIM_TOTAL_MS  (2 * 60 * 1000)

/**
  * @brief  初始化治疗控制：注册事件 handler + 创建 1s esp_timer（需在 modbus 就绪后调用）
  * @retval ESP_OK 成功；其他见 esp_err_t
  */
esp_err_t app_therapy_init(void);

/**
  * @brief  开始一个治疗会话（从 scheme->steps[0] 刺激开始，按步骤顺序循环）
  * @param  scheme   处方方案（如 &g_rx_all[RX_3]->schemes[0] = 处方3疗程1）
  * @param  intens1  阶段1 强度 0~90（原值即 mA，直接下发；0=不输出）
  * @param  intens2  阶段2 强度 0~90（原值即 mA，直接下发；0=不输出）
  * @param  ui_ctx   UI 上下文（Treat_Timer_Ctx_t*，可 NULL=纯 console 驱动）
  * @retval MB_OK 成功；MB_ERR_PARAM 参数错误
  */
int therapy_start(const rx_scheme_t *scheme, int intens1, int intens2, void *ui_ctx);

/**
 * @brief  凯格尔训练专用启动：纯生物反馈，无电刺激（跳过 ParSet/Treat 页）
 * @param  scheme   凯格尔处方方案（&g_rx_all[RX_4]->schemes[idx]）
 * @retval MB_OK 成功；MB_ERR_PARAM 参数错误
 */
int therapy_start_kegel(const rx_scheme_t *scheme);

/** @brief 暂停：停波 + 保留会话状态（剩余时间/相位），resume 续跑 */
void therapy_pause(void);

/** @brief 恢复：重发当前相位参数 + 启动（0T 方案A） */
void therapy_resume(void);

/** @brief 停止：停波 + 会话复位 IDLE（退出/返回时调用） */
void therapy_stop(void);

/**
 * @brief 紧急/兜底停止：无条件停采集(push) + 强制停波 + 复位 IDLE
 * @note  与 therapy_stop 不同，不看引擎是否 IDLE —— 专治"引擎以为已停、底层仍在输出"
 *        的状态不同步（STM32 无通信超时/看门狗自保）。用于"返回主菜单"全局收口，
 *        任何页面路径漏停都在此兜住。幂等，可重复调用。
 */
void therapy_emergency_stop(void);

void my_therapy_pause(void);
void my_therapy_resume(void);
/**
  * @brief  实时调当前相位强度档（0~10）
  * @note   治疗中当前相位 ON 时异步下发 mA + 重发 0x0106 触发渐变（不卡 LVGL）；
  *         非 ON 相位/暂停时只更新存储值，下次 ON 进入时生效
  * @retval MB_OK 成功
  */
int therapy_set_intensity(int intens_level);

/** @brief 设置会话时钟缩放：1 真实秒 = scale 会话秒（0/1=真实速度；联调验证用） */
void therapy_set_scale(uint32_t scale_s);
uint32_t therapy_get_scale(void);

/* ---- 状态查询（UI / console 用） ---- */
bool therapy_active(void);              /* 会话运行中或暂停中 */
int32_t therapy_remain_ms(void);        /* 会话剩余（会话时钟，ms） */
int therapy_status(void);               /* timer_status_t（menu_ui.h） */
uint8_t therapy_current_intens(void);   /* 当前相位强度档 */
uint16_t therapy_current_freq(void);    /* [我们] 当前步骤频率 Hz（UI 参数切换显示用；未运行时 0） */
uint16_t therapy_current_pw(void);      /* [我们] 当前步骤脉宽 us（UI 参数切换显示用；未运行时 0） */
uint8_t therapy_current_stage_stim(void);
void therapy_print_status(void);        /* console 打印会话状态 */
uint16_t therapy_cur_freq(const rx_scheme_t *scheme,uint8_t idx);   
uint16_t therapy_cur_pw(const rx_scheme_t *scheme,uint8_t idx);
void therapy_init_chart(void *ui_chart);
void therapy_init_chart_instr(void *ui_chart);
uint32_t therapy_current_stage_time(void);    
uint8_t therapy_current_step_cnt(void);
rx_step_t *cur_stage(void);
rx_step_t *cur_step(void);
uint32_t therapy_get_stim_time(const rx_scheme_t *scheme);
/* 运行时改写阶段时长（串口指令用）：scheme 为 NULL 时用当前会话方案 */
int  therapy_set_stage_time(const rx_scheme_t *scheme, uint8_t stage_idx, uint32_t ms);
uint32_t therapy_get_stage_time(const rx_scheme_t *scheme, uint8_t stage_idx);
/* 运行时改写会话总时长（串口指令用） */
void therapy_set_total_time(uint32_t ms);
uint32_t therapy_get_total_time(void);
void therapy_emg_feedback(int32_t emg_uv);
int therapy_init_scheme(const rx_scheme_t *scheme);
void therapy_send_intensity(uint8_t intens,uint16_t freq,uint16_t pw);
uint16_t therapy_cur_stage_stim(const rx_scheme_t *scheme);
#ifdef __cplusplus
}
#endif

#endif /* APP_THERAPY_H */
