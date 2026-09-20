/**
  ******************************************************************************
  * @文件名称   therapy_rx.h
  * @文件描述   处方存储（阶段 C）：真实处方资料转只读 C 表（步骤序列结构）
  *             数据源：EDA_EMG/doc/治疗处方/利玮生物刺激反馈仪处方2026052501.md
  *
  *             结构（处方量大，拆 步骤→方案→处方 三级）：
  *               rx_step_t     一个步骤（采集/电刺激/等待/训练/条件触发）
  *               rx_scheme_t   一个治疗方案（有序步骤数组）
  *               rx_prescription_t  一个处方（多方案）
  *               g_rx_all[]    4 处方汇总（按处方1/2/3/4，即 RX_1~RX_4）
  *
  *             步骤类型（rx_step_type_t）：
  *               RX_STEP_ACQ    采集 EMG 基础值：dur_s + voice
  *               RX_STEP_STIM   电刺激段：freq/pw/on_s/off_s/repeat/ch
  *               RX_STEP_WAIT   等待（分钟转秒）：dur_s
  *               RX_STEP_TRAIN  训练动作：kind + dur_s + voice
  *               RX_STEP_COND   条件触发->电刺激（EMG 掉线/超限等，执行留阶段D）：
  *                               freq/pw/dur_s，触发条件在方案注释
  *
  *             训练动作（rx_train_kind_t）：
  *               RX_TRAIN_RELAX / CONTRACT / HOLD / GRAPH / STRONG
  *             语音字段 voice 用 voice.h 的 voice_id_t（0=无语音）
  ******************************************************************************
  */
#ifndef THERAPY_RX_H
#define THERAPY_RX_H

#include <stdint.h>

/* ---- 通道掩码（位掩码；STIM/COND 步骤与疗程级标记共用） ---- */
#define THERAPY_CH1   0x01   /* 通道1 */
#define THERAPY_CH2   0x02   /* 通道2 */
#define THERAPY_BOTH  (THERAPY_CH1 | THERAPY_CH2)  /* 通道1+2 */

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 步骤类型 ---- */
typedef enum {
    RX_STEP_ACQ = 0,        /* 采集 EMG 基础值（dur_s + voice） */
    RX_STEP_STIM,           /* 电刺激段（freq/pw/on/off/repeat/ch） */
    RX_STEP_WAIT,           /* 等待（dur_s，分钟已转秒） */
    RX_STEP_TRAIN,          /* 训练动作（kind + dur_s + voice） */
    RX_STEP_TRAIN_INSTR,    /* 按指示训练动作（kind + dur_s + voice） */
    RX_STEP_COND,           /* 条件触发->电刺激（freq/pw/dur_s；执行留阶段 D） */
} rx_step_type_t;

/* ---- 训练动作类型 ---- */
typedef enum {
    RX_TRAIN_RELAX = 0,   /* 放松 */
    RX_TRAIN_CONTRACT,    /* 用力/收缩 */
    RX_TRAIN_HOLD,        /* 收缩并保持（收缩+保持+放松组合） */
    RX_TRAIN_GRAPH,       /* 按图形发力 */
    RX_TRAIN_STRONG,      /* 大力收缩 N 次 */ 
} rx_train_kind_t;

/* ================= 条件刺激触发类型 ================= */
/* 由处方表中 TRAIN 步骤的 train 字段决定，therapy_emg_feedback() 据此选择触发逻辑 */
typedef enum {
    RX_TRIGGER_BELOW_REF   = 1,  /* 类型1: 信号低于参考线(25) → 触发10s刺激 */
    RX_TRIGGER_3FAIL       = 2,  /* 类型2: 连续3次未能超过虚线(25) → 触发 */
    RX_TRIGGER_ABOVE_BLUE  = 3,  /* 类型3: 信号超过参考线进入蓝色区域(>75)持续≥2s → 触发 */
} rx_trigger_kind_t;

/* 触发阈值常量（图表 Y 轴 0~100 刻度） */
#define RX_TRIGGER_REF_LINE      25   /* 参考线/虚线位置 */
#define RX_TRIGGER_BLUE_ZONE     75   /* 蓝色区域起始位置 */
#define RX_TRIGGER_3FAIL_COUNT   3    /* 类型2: 连续失败次数阈值 */
#define RX_TRIGGER_BLUE_HOLD_MS  2000 /* 类型3: 蓝色区域持续时间阈值 (ms) */

/* ---- 一个步骤 ---- */
typedef struct {
    rx_step_type_t type;   /* 步骤类型 */
    uint8_t  ch;           /* 通道位（STIM/COND） */
    uint16_t freq;         /* 频率 Hz（STIM/COND） */
    uint16_t pw;           /* 脉宽 us（STIM/COND） */
    uint16_t on_s;         /* 刺激/发力 秒（STIM/TRAIN） */
    uint16_t off_s;        /* 休息/放松 秒（STIM/TRAIN） */
    uint16_t repeat;       /* 重复次数（STIM 段循环；TRAIN 大力次数） */
    uint16_t dur_s;        /* 总时长秒（ACQ/WAIT/TRAIN/COND） */
    uint8_t  voice;        /* 语音段 voice_id_t（0=无） */
    uint8_t  train;        /* 训练动作类型（TRAIN） */
    uint8_t  stim_stage;    /* 电刺激阶段*/
    uint8_t  trigger;       /* 条件触发类型（COND） */
} rx_step_t;

/* ---- 一个治疗方案（有序步骤数组 + 疗程元数据） ---- */
typedef struct {
    uint8_t                    stage_stim;      /*电刺激阶段 单/双阶段*/
    uint8_t                    stage_cnt;       /* 阶段总数 */
    const uint8_t              *steps_per_stage; /* 每个阶段的步骤数（长度=stage_cnt） */
    const rx_step_t * const    *stages;          /* 指针数组：stages[i] → 第i阶段的步骤数组 */
    uint32_t                   *stage_time;      /* 阶段治疗时长（可运行时改写，见 therapy_set_stage_time） */
    uint8_t                    ch;              /* 通道位 */
    const char                 *time;            /* 治疗时长 */
    const char                 *channel;         /* 通道/电极描述 */
    const char                 *current;         /* 电流说明 */
    const char                 *note;            /* 备注/模式 */
}rx_scheme_t;


/* ---- 一个处方（多方案） ---- */
typedef struct {
    const char        *name;        /* 处方名 */
    const char        *mode;        /* 模式：电刺激/电刺激+条件性/.../生物反馈 */
    uint8_t            scheme_cnt;  /* 方案数 */
    const rx_scheme_t *schemes;     /* 方案数组 */
} rx_prescription_t;

/* ---- 处方索引（按处方1/2/3/4） ---- */
typedef enum {
    RX_1 = 0,   /* 高张力盆底功能障碍（UI 障碍格 Dys） */
    RX_2,       /* 盆底肌保养（UI 保养格 Mai） */
    RX_3,       /* 盆底肌治疗（UI 治疗格 The） */
    RX_4,       /* 凯格尔训练（UI 凯格尔格 Kge，纯生物反馈） */
    RX_MAX
} rx_id_t;

/* 4 处方汇总表（定义在 therapy_rx.c，按处方号索引） */
extern const rx_prescription_t *const g_rx_all[RX_MAX];

#ifdef __cplusplus
}
#endif

#endif /* THERAPY_RX_H */
