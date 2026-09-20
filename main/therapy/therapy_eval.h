/**
  ******************************************************************************
  * @文件名称   therapy_eval.h
  * @文件描述   盆底评估协议（功能2）：采集 EMG 计算指标/得分（非治疗）
  *             数据源：EDA_EMG/doc/治疗处方/利玮生物刺激反馈仪处方2026052501.md
  *
  *             动作步骤复用 therapy_rx.h 的 rx_step_t（WAIT 静息 / TRAIN 用力保持）；
  *             评估指标为新结构（rx_eval_metric_t / rx_eval_phase_t / rx_eval_t）
  *             参考值/测试结果/得分暂为空（搭框架，后续填）
  ******************************************************************************
  */
#ifndef THERAPY_EVAL_H
#define THERAPY_EVAL_H

#include "therapy_rx.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 评估指标类型 ---- */
typedef enum {
    RX_EVAL_AVG = 0,    /* 平均值 */
    RX_EVAL_VAR,        /* 变异性 */
    RX_EVAL_MAX,        /* 最大值 */
    RX_EVAL_RISE,       /* 上升时间 */
    RX_EVAL_RECOVER,    /* 恢复时间 */
    RX_EVAL_TOTAL,      /* 总得分 */
} rx_eval_metric_t;

/* ---- 一个评估阶段 ---- */
typedef struct {
    const char             *name;       /* 阶段名（前静息期/快速收缩期/持续收缩期/后静息期） */
    uint8_t                 step_cnt;   /* 动作步骤数 */
    const rx_step_t        *steps;      /* 动作序列（复用 rx_step_t：WAIT 静息 / TRAIN 用力保持） */
    uint8_t                 repeat;     /* 整段循环次数 */
    uint8_t                 metric_cnt; /* 评估指标数 */
    const rx_eval_metric_t *metrics;    /* 本阶段评估指标 */
} rx_eval_phase_t;

/* ---- 评估协议 ---- */
typedef struct {
    const char            *name;       /* 协议名 */
    uint8_t                phase_cnt;  /* 阶段数 */
    const rx_eval_phase_t *phases;     /* 阶段数组 */
} rx_eval_t;

/* 盆底评估协议（定义在 therapy_eval.c） */
extern const rx_eval_t g_eval;

/* ============================================================================
 * 评估执行引擎（Glazer 4 阶段状态机 + 指标计算 + 相对标定评分）
 * 数据源：emg_force_get_env()（力度包络 µV）
 * 计时：esp_timer 50ms 自驱动（每秒 20 次采样，保证峰值/上升/恢复分辨率）
 * ========================================================================== */

/* ---- 评估运行状态 ---- */
typedef enum {
    EVAL_IDLE = 0,   /* 未开始 */
    EVAL_REST1,      /* 前静息期 */
    EVAL_FAST,       /* 快速收缩期 */
    EVAL_SUST,       /* 持续收缩期 */
    EVAL_REST2,      /* 后静息期 */
    EVAL_DONE,       /* 评估完成 */
} eval_state_t;

/* ---- 评估结果（相对标定：以前静息均值为基线，不依赖固定参考值） ---- */
typedef struct {
    /* 基线 */
    float    baseline;         /* 前静息平均值 (µV)，相对标定基准 */
    /* 前静息期 */
    float    rest1_avg;        /* 平均值 (µV) */
    float    rest1_var;        /* 变异性 (%) = std/mean × 100 */
    /* 快速收缩期 */
    float    fast_max;         /* 5 次用力峰值中的最大者 (µV) */
    float    fast_max_ratio;   /* 峰值/基线 倍数 */
    float    fast_rise;        /* 上升时间 (s)：基线 -> 90% 峰值 */
    float    fast_recover;     /* 恢复时间 (s)：峰值 -> 回落阈值 */
    /* 持续收缩期 */
    float    sust_avg;         /* 保持段平均值 (µV) */
    float    sust_ratio;       /* 保持段均值/基线 倍数 */
    float    sust_var;         /* 保持段变异性 (%) */
    /* 后静息期 */
    float    rest2_avg;        /* 平均值 (µV) */
    float    rest2_var;        /* 变异性 (%) */
    /* 总得分 */
    uint8_t  total;            /* 0~100 */
    /* 分项得分（0~100，报告页按行显示） */
    uint8_t  score_force;      /* 力量（快速收缩最大值） */
    uint8_t  score_endu;       /* 耐力（持续收缩平均值） */
    uint8_t  score_rise;       /* 上升速度 */
    uint8_t  score_recover;    /* 恢复速度 */
    uint8_t  score_rest;       /* 静息稳定（前/后静息变异性） */
    uint8_t  score_sust;       /* 保持稳定（持续收缩变异性） */
} eval_result_t;

/**
 * @brief  开始评估：复位结果 + 从 g_eval 阶段0 步骤0 启动 50ms tick
 * @note   调用前需已 emg_force_start_acq() 启动采集；返回后由 esp_timer 自驱动
 * @retval ESP_OK 成功
 */
esp_err_t eval_start(void);

/** @brief 停止评估：停 timer + 复位 IDLE（退出评估页时调用） */
void eval_stop(void);

/** @brief 当前评估状态 */
eval_state_t eval_get_state(void);

/** @brief 当前阶段名（"前静息期"等；IDLE 时 "未开始"） */
const char *eval_phase_name(void);

/** @brief 当前阶段剩余秒数（UI 倒计时；IDLE/DONE 返回 0） */
uint16_t eval_remain_s(void);

/** @brief 全程总剩余秒数（4 阶段合计倒计时；IDLE/DONE 返回 0） */
uint16_t eval_total_remain_s(void);

/** @brief 当前阶段内已用 tick 数（0~阶段总tick；UI 按此定位画点位置，避免与引擎 tick 漂移） */
uint16_t eval_phase_elapsed_ticks(void);

/** @brief 当前阶段总秒数（UI 进度分母；IDLE/DONE 返回 0） */
uint16_t eval_phase_total_s(void);

/** @brief 当前动作名（"放松"/"用力"/"保持用力"；IDLE 返回 "无"、DONE 返回 "完成"） */
const char *eval_action_name(void);

/** @brief 下一步动作名（"下节预览"用；最后一步返回 "完成"） */
const char *eval_next_action_name(void);
rx_train_kind_t eval_action_kind(void);
rx_train_kind_t eval_next_action_kind(void);

/** @brief 评估结果（DONE 后有效；运行中为中间值） */
const eval_result_t *eval_get_result(void);

#ifdef __cplusplus
}
#endif

#endif /* THERAPY_EVAL_H */
