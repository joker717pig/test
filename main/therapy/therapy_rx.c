/**
  ******************************************************************************
  * @文件名称   therapy_rx.c
  * @文件描述   处方存储表（阶段 C）：真实处方资料转只读 C 表（步骤序列结构）
  *             数据源：EDA_EMG/doc/治疗处方/利玮生物刺激反馈仪处方2026052501.md
  *             （自动生成：tools/gen_rx_steps.py --write，勿手改数据段）
  *
  *             多数组组合结构（处方量大，拆 步骤→方案→处方 三级）：
  *               <处方>_steps[]   全部方案步骤平铺（rx_step_t）
  *               <处方>_schemes[] 方案数组（引用 steps 偏移）
  *               rx<处方号>       处方（scheme_cnt + schemes）
  *               g_rx_all[]       4 处方汇总（按处方1/2/3/4）
  *
  *             步骤字段（rx_step_t）：{type, ch, freq, pw, on_s, off_s,
  *               repeat, dur_s, voice, train}；含义见 therapy_rx.h
  *             方案数核对：处方1=15 / 处方2=20 / 处方3=25 / 处方4=18（与 md 全表一致）
  ******************************************************************************
  */
#include "therapy_rx.h"
#include "voice.h"   /* VOICE_ID_* 语音段枚举 */


/* ================= 处方1 高张力盆底功能障碍 ================= */
/* 每个疗程一个数组；疗程号 = 文档治疗方案行序 */
/* ---- 疗程1 [阶段1/1] (电刺激) ---- */
static const rx_step_t rx1_c1[] = {
    {RX_STEP_STIM, THERAPY_CH1, 4, 300, 10, 5, 1, 0, 0, 0, 1},  /* 4Hz/300us 10s/5s */
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 13, 2, 1, 0, 0, 0, 1},  /* 5Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c1_stages[] = { rx1_c1 };
static const uint8_t            rx1_c1_step_cnts[] = { 2 };
static uint32_t rx1_c1_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程2 [阶段1/1] (电刺激) ---- */
static const rx_step_t rx1_c2[] = {
    {RX_STEP_STIM, THERAPY_CH1, 4, 300, 10, 5, 1, 0, 0, 0, 1},  /* 4Hz/300us 10s/5s */
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c2_stages[] = { rx1_c2};
static const uint8_t            rx1_c2_step_cnts[] = { 2 };
static uint32_t rx1_c2_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程3 [阶段1/1] (电刺激) ---- */
static const rx_step_t rx1_c3[] = {
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 13, 2, 1, 0, 0, 0, 1},  /* 5Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c3_stages[] = { rx1_c3 };
static const uint8_t            rx1_c3_step_cnts[] = { 2 };
static uint32_t rx1_c3_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程4 [阶段1/1] (电刺激) ---- */
static const rx_step_t rx1_c4[] = {
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 13, 2, 1, 0, 0, 0, 2},  /* 5Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c4_stages[] = { rx1_c4 };
static const uint8_t            rx1_c4_step_cnts[] = { 2 };
static uint32_t rx1_c4_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程5 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx1_c5[] = {
    {RX_STEP_STIM, THERAPY_CH1, 50, 300, 8, 2, 1, 0, 0, 0, 1},  /* 50Hz/300us 8s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 4, 300, 10, 5, 1, 0, 0, 0, 2},  /* 4Hz/300us 10s/5s */
};
static const rx_step_t * const  rx1_c5_stages[] = { rx1_c5 };
static const uint8_t            rx1_c5_step_cnts[] = { 2 };
static uint32_t rx1_c5_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程6 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx1_c6[] = {
    {RX_STEP_STIM, THERAPY_CH1, 50, 300, 8, 2, 1, 0, 0, 0, 1},  /* 50Hz/300us 8s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 13, 2, 1, 0, 0, 0, 2},  /* 5Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c6_stages[] = { rx1_c6 };
static const uint8_t            rx1_c6_step_cnts[] = { 2 };
static uint32_t rx1_c6_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程7 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx1_c7[] = {
    {RX_STEP_STIM, THERAPY_CH1, 50, 300, 8, 2, 1, 0, 0, 0, 1},  /* 50Hz/300us 8s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 13, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c7_stages[] = { rx1_c7 };
static const uint8_t            rx1_c7_step_cnts[] = { 2 };
static uint32_t rx1_c7_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程8 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx1_c8[] = {
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 2, 1, 1, 0, 0, 0, 1},  /* 80Hz/160us 2s/1s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 2, 1, 1, 0, 0, 0, 1},  /* 80Hz/160us 2s/1s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 2, 1, 1, 0, 0, 0, 1},  /* 80Hz/160us 2s/1s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 4, 300, 10, 5, 1, 0, 0, 0, 2},  /* 4Hz/300us 10s/5s */
};
static const rx_step_t * const  rx1_c8_stages[] = { rx1_c8 };
static const uint8_t            rx1_c8_step_cnts[] = { 4 };
static uint32_t rx1_c8_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程9 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx1_c9[] = {
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 1, 2, 1, 0, 0, 0, 1},  /* 80Hz/160us 1s/2s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 1, 2, 1, 0, 0, 0, 1},  /* 80Hz/160us 1s/2s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 1, 2, 1, 0, 0, 0, 1},  /* 80Hz/160us 1s/2s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 13, 2, 1, 0, 0, 0, 2},  /* 5Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c9_stages[] = { rx1_c9 };
static const uint8_t            rx1_c9_step_cnts[] = { 4 };
static uint32_t rx1_c9_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程10 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx1_c10[] = {
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 1, 2, 1, 0, 0, 0, 1},  /* 80Hz/160us 1s/2s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 1, 2, 1, 0, 0, 0, 1},  /* 80Hz/160us 1s/2s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 1, 2, 1, 0, 0, 0, 1},  /* 80Hz/160us 1s/2s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 13, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c10_stages[] = { rx1_c10 };
static const uint8_t            rx1_c10_step_cnts[] = { 4 };
static uint32_t rx1_c10_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程11 [阶段1/1] (电刺激) ---- */
static const rx_step_t rx1_c11[] = {
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 13, 2, 1, 0, 0, 0, 1},  /* 5Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 4, 300, 9, 6, 1, 0, 0, 0, 1},  /* 4Hz/300us 9s/6s */
};
static const rx_step_t * const  rx1_c11_stages[] = { rx1_c11 };
static const uint8_t            rx1_c11_step_cnts[] = { 2 };
static uint32_t rx1_c11_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程12 [阶段1/1] (电刺激) ---- */
static const rx_step_t rx1_c12[] = {
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 13, 2, 1, 0, 0, 0, 1},  /* 5Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c12_stages[] = { rx1_c12 };
static const uint8_t            rx1_c12_step_cnts[] = { 2 };
static uint32_t rx1_c12_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程13 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx1_c13[] = {
    {RX_STEP_STIM, THERAPY_CH1, 50, 300, 8, 2, 1, 0, 0, 0, 1},  /* 50Hz/300us 8s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 4, 300, 9, 6, 1, 0, 0, 0, 2},  /* 4Hz/300us 9s/6s */
};
static const rx_step_t * const  rx1_c13_stages[] = { rx1_c13 };
static const uint8_t            rx1_c13_step_cnts[] = { 2 };
static uint32_t rx1_c13_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程14 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx1_c14[] = {
    {RX_STEP_STIM, THERAPY_CH1, 50, 300, 8, 2, 1, 0, 0, 0, 1},  /* 50Hz/300us 8s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 13, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c14_stages[] = { rx1_c14 };
static const uint8_t            rx1_c14_step_cnts[] = { 2 };
static uint32_t rx1_c14_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程15 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx1_c15[] = {
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 1, 2, 1, 0, 0, 0, 1},  /* 80Hz/160us 1s/2s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 1, 2, 1, 0, 0, 0, 1},  /* 80Hz/160us 1s/2s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 80, 160, 1, 2, 1, 0, 0, 0, 1},  /* 80Hz/160us 1s/2s x3 */
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 13, 2, 1, 0, 0, 0, 2},  /* 5Hz/300us 13s/2s */
};
static const rx_step_t * const  rx1_c15_stages[] = { rx1_c15 };
static const uint8_t            rx1_c15_step_cnts[] = { 4 };
static uint32_t rx1_c15_time_cnts[] = {(30*60*1000)}; 
static const rx_scheme_t rx1_schemes[] = {
    {1, 1, rx1_c1_step_cnts,rx1_c1_stages,rx1_c1_time_cnts, THERAPY_CH1, "30 min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程1 [阶段1/1] */
    {1, 1, rx1_c2_step_cnts,rx1_c2_stages,rx1_c2_time_cnts, THERAPY_CH1, "30 min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程2 [阶段1/1] */
    {1, 1, rx1_c3_step_cnts,rx1_c3_stages,rx1_c3_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程3 [阶段1/1] */
    {1, 1, rx1_c4_step_cnts,rx1_c4_stages,rx1_c4_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程4 [阶段1/1] */
    {2, 1, rx1_c5_step_cnts,rx1_c5_stages,rx1_c5_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程5 [阶段1/2+阶段2/2] */
    {2, 1, rx1_c6_step_cnts,rx1_c6_stages,rx1_c6_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程6 [阶段1/2+阶段2/2] */
    {2, 1, rx1_c7_step_cnts,rx1_c7_stages,rx1_c7_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程7 [阶段1/2+阶段2/2] */
    {2, 1, rx1_c8_step_cnts,rx1_c8_stages,rx1_c8_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程8 [阶段1/2+阶段2/2] */
    {2, 1, rx1_c9_step_cnts,rx1_c9_stages,rx1_c9_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程9 [阶段1/2+阶段2/2] */
    {2, 1, rx1_c10_step_cnts,rx1_c10_stages,rx1_c10_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程10 [阶段1/2+阶段2/2] */
    {1, 1, rx1_c11_step_cnts,rx1_c11_stages,rx1_c11_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程11 [阶段1/1] */
    {1, 1, rx1_c12_step_cnts,rx1_c12_stages,rx1_c12_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程12 [阶段1/1] */
    {2, 1, rx1_c13_step_cnts,rx1_c13_stages,rx1_c13_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程13 [阶段1/2+阶段2/2] */
    {2, 1, rx1_c14_step_cnts,rx1_c14_stages,rx1_c14_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程14 [阶段1/2+阶段2/2] */
    {2, 1, rx1_c15_step_cnts,rx1_c15_stages,rx1_c15_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程15 [阶段1/2+阶段2/2] */
};
const rx_prescription_t rx1 = { "处方1: 高张力盆底功能障碍", "电刺激", 15, rx1_schemes };

/* ================= 处方2 盆底肌保养 ================= */
/* 每个疗程一个数组；疗程号 = 文档治疗方案行序 */
/* ---- 疗程1 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx2_c1[] = {
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 28, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 28s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 50, 300, 8, 2, 1, 0, 0, 0, 2},  /* 50Hz/300us 8s/2s */
};
static const rx_step_t * const  rx2_c1_stages[] = { rx2_c1 };
static const uint8_t            rx2_c1_step_cnts[] = { 2 };
static uint32_t rx2_c1_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程2 [阶段1/1] (电刺激) ---- */
static const rx_step_t rx2_c2[] = {
    {RX_STEP_STIM, THERAPY_CH1, 20, 500, 8, 2, 1, 0, 0, 0, 1},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t * const  rx2_c2_stages[] = { rx2_c2 };
static const uint8_t            rx2_c2_step_cnts[] = { 1 };
static uint32_t rx2_c2_time_cnts[] = {(30*60*1000)}; 

/* ---- 疗程3 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx2_c3[] = {
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 8, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 8s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 30, 300, 8, 2, 1, 0, 0, 0, 2},  /* 30Hz/300us 8s/2s */
};
static const rx_step_t * const  rx2_c3_stages[] = { rx2_c3 };
static const uint8_t            rx2_c3_step_cnts[] = { 2 };
static uint32_t rx2_c3_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程4 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx2_c4[] = {
    {RX_STEP_STIM, THERAPY_CH1, 20, 300, 2, 1, 1, 0, 0, 0, 1},  /* 20Hz/300us 2s/1s */
    {RX_STEP_STIM, THERAPY_CH1, 50, 300, 2, 1, 1, 0, 0, 0, 2},  /* 50Hz/300us 2s/1s */
};
static const rx_step_t * const  rx2_c4_stages[] = { rx2_c4 };
static const uint8_t            rx2_c4_step_cnts[] = { 2 };
static uint32_t rx2_c4_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程5 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx2_c5[] = {
    {RX_STEP_STIM, THERAPY_CH1, 4, 500, 13, 2, 1, 0, 0, 0, 1},  /* 4Hz/500us 13s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 33, 300, 13, 2, 1, 0, 0, 0, 2},  /* 33Hz/300us 13s/2s */
};
static const rx_step_t * const  rx2_c5_stages[] = { rx2_c5 };
static const uint8_t            rx2_c5_step_cnts[] = { 2 };
static uint32_t rx2_c5_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程6 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx2_c6[] = {
    {RX_STEP_STIM, THERAPY_CH1, 40, 100, 1, 2, 1, 0, 0, 0, 1},  /* 40Hz/100us 1s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 80, 300, 1, 2, 1, 0, 0, 0, 2},  /* 80Hz/300us 1s/2s */
};
static const rx_step_t * const  rx2_c6_stages[] = { rx2_c6 };
static const uint8_t            rx2_c6_step_cnts[] = { 2 };
static uint32_t rx2_c6_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程7 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx2_c7[] = {
    {RX_STEP_STIM, THERAPY_CH1, 33, 300, 13, 2, 1, 0, 0, 0, 1},  /* 33Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 80, 300, 1, 2, 1, 0, 0, 0, 2},  /* 80Hz/300us 1s/2s  */
    {RX_STEP_STIM, THERAPY_CH1, 80, 300, 1, 2, 1, 0, 0, 0, 2},  /* 80Hz/300us 1s/2s  */
    {RX_STEP_STIM, THERAPY_CH1, 80, 300, 1, 2, 1, 0, 0, 0, 2},  /* 80Hz/300us 1s/2s  */
    {RX_STEP_STIM, THERAPY_CH1, 80, 300, 1, 2, 1, 0, 0, 0, 2},  /* 80Hz/300us 1s/2s  */
    {RX_STEP_STIM, THERAPY_CH1, 80, 300, 1, 2, 1, 0, 0, 0, 2},  /* 80Hz/300us 1s/2s  */
};
static const rx_step_t * const  rx2_c7_stages[] = { rx2_c7 };
static const uint8_t            rx2_c7_step_cnts[] = { 6 };
static uint32_t rx2_c7_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程8 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx2_c8_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c8_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 20, 500, 8, 2, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
/* ---- 疗程10 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx2_c8_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD,0,RX_TRIGGER_BELOW_REF},  /* 收缩并保持 14s */
    {RX_STEP_COND, THERAPY_BOTH, 20, 500, 0, 0, 0, 10, 0, 0, 1},  /* 条件->电刺激 500us/20Hz 10s */
};
static const rx_step_t * const rx2_c8_stages[] = { rx2_c8_s0,rx2_c8_s1,rx2_c8_s2};
static const uint8_t            rx2_c8_step_cnts[] = { 2,2,3};
static uint32_t rx2_c8_time_cnts[] = { 30*1000,(((14*60)+30)*1000),(15*60*1000)};  

/* ---- 疗程9 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx2_c9_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c9_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 20, 500, 8, 2, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c9_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 15, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG,0,RX_TRIGGER_3FAIL},  /* 大力收缩 14s */
    {RX_STEP_COND, THERAPY_BOTH, 20, 500, 0, 0, 0, 10, 0, 0, 1},  /* 条件->电刺激 500us/20Hz 10s */
};
static const rx_step_t * const rx2_c9_stages[] = { rx2_c9_s0,rx2_c9_s1,rx2_c9_s2};
static const uint8_t            rx2_c9_step_cnts[] = { 2,2,3};
static uint32_t rx2_c9_time_cnts[] = { 30*1000,(((14*60)+30)*1000),(15*60*1000)}; 

/* ---- 疗程10 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx2_c10_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c10_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 20, 500, 8, 2, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c10_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD,0,RX_TRIGGER_BELOW_REF},  /* 收缩并保持 14s */
    {RX_STEP_COND, THERAPY_BOTH, 20, 500, 0, 0, 0, 10, 0, 0, 1},  /* 条件->电刺激 500us/20Hz 10s */
};
static const rx_step_t * const rx2_c10_stages[] = { rx2_c10_s0,rx2_c10_s1,rx2_c10_s2};
static const uint8_t            rx2_c10_step_cnts[] = { 2,2,3};
static uint32_t rx2_c10_time_cnts[] = { 30*1000,(((14*60)+30)*1000),(15*60*1000)}; 

/* ---- 疗程11 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx2_c11_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c11_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 20, 500, 8, 2, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c11_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 15, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG,0,RX_TRIGGER_3FAIL},  /* 大力收缩 14s */
    {RX_STEP_COND, THERAPY_BOTH, 20, 500, 0, 0, 0, 10, 0, 0, 1},  /* 条件->电刺激 500us/20Hz 10s */
};
static const rx_step_t * const rx2_c11_stages[] = { rx2_c11_s0,rx2_c11_s1,rx2_c11_s2};
static const uint8_t            rx2_c11_step_cnts[] = { 2,2,3};
static uint32_t rx2_c11_time_cnts[] = { 30*1000,(((14*60)+30)*1000),(15*60*1000)}; 

/* ---- 疗程12 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx2_c12_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c12_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 20, 500, 8, 2, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c12_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD,0,RX_TRIGGER_BELOW_REF},  /* 收缩保持 14s */
    {RX_STEP_COND, THERAPY_BOTH, 20, 500, 0, 0, 0, 10, 0, 0, 1},  /* 条件->电刺激 500us/20Hz 10s */
};
static const rx_step_t * const rx2_c12_stages[] = { rx2_c12_s0,rx2_c12_s1,rx2_c12_s2};
static const uint8_t            rx2_c12_step_cnts[] = { 2,2,3};
static uint32_t rx2_c12_time_cnts[] = { 30*1000,(((14*60)+30)*1000),(15*60*1000)}; 

/* ---- 疗程13 [阶段1/2+阶段2/2] (电刺激+条件性电刺激+生物反馈) ---- */
static const rx_step_t rx2_c13_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c13_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 20, 500, 8, 2, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c13_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 15, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG,0,RX_TRIGGER_3FAIL},  /* 大力收缩 14s */
    {RX_STEP_COND, THERAPY_BOTH, 20, 500, 0, 0, 0, 10, 0, 0, 1},  /* 条件->电刺激 500us/20Hz 10s */
};
static const rx_step_t rx2_c13_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD},  /* 大力收缩 14s */
};
static const rx_step_t * const rx2_c13_stages[] = { rx2_c13_s0,rx2_c13_s1,rx2_c13_s2,rx2_c13_s3};
static const uint8_t            rx2_c13_step_cnts[] = { 2,2,3,2};
static uint32_t rx2_c13_time_cnts[] = { 30*1000,(((9*60)+30)*1000),((10*60)*1000),(10*60*1000)}; 

/* ---- 疗程14 [阶段1/2+阶段2/2] (电刺激+条件性电刺激+生物反馈) ---- */
static const rx_step_t rx2_c14_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c14_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 20, 500, 8, 2, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c14_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD,0,RX_TRIGGER_ABOVE_BLUE},  /* 收缩保持 14s */
    {RX_STEP_COND, THERAPY_BOTH, 20, 500, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 500us/20Hz 10s */
};
static const rx_step_t rx2_c14_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG },  /* 大力收缩 14s */
};
static const rx_step_t * const rx2_c14_stages[] = { rx2_c14_s0,rx2_c14_s1,rx2_c14_s2,rx2_c14_s3};
static const uint8_t            rx2_c14_step_cnts[] = { 2,2,3,2};
static uint32_t rx2_c14_time_cnts[] = { 30*1000,(((9*60)+30)*1000),((10*60)*1000),(10*60*1000)}; 

/* ---- 疗程15 [阶段1/2+阶段2/2] (电刺激+条件性电刺激+生物反馈) ---- */
static const rx_step_t rx2_c15_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c15_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 20, 500, 8, 2, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c15_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 15, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG,0,RX_TRIGGER_3FAIL},  /* 大力收缩 14s */
    {RX_STEP_COND, THERAPY_BOTH, 20, 500, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 500us/20Hz 10s */
};
static const rx_step_t rx2_c15_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, RX_TRAIN_GRAPH},  /* 按图形发力 14s */
};
static const rx_step_t * const rx2_c15_stages[] = { rx2_c15_s0,rx2_c15_s1,rx2_c15_s2,rx2_c15_s3};
static const uint8_t            rx2_c15_step_cnts[] = { 2,2,3,2};
static uint32_t rx2_c15_time_cnts[] = { 30*1000,(((9*60)+30)*1000),(10*60*1000),(10*60*1000)}; 

/* ---- 疗程16 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx2_c16_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c16_s1[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 33, 300, 18, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t rx2_c16_s2[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 80, 300, 2, 1, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c16_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, RX_TRAIN_GRAPH},  /* 按图形发力 14s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG},  /* 大力收缩 14s */
};
static const rx_step_t * const rx2_c16_stages[] = { rx2_c16_s0,rx2_c16_s1,rx2_c16_s2,rx2_c16_s3};
static const uint8_t            rx2_c16_step_cnts[] = { 2,1,1,4};
static uint32_t rx2_c16_time_cnts[] = { 30*1000,(((4*60)+30)*1000),(5*60*1000),(20*60*1000)};
/* ---- 疗程17 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx2_c17_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c17_s1[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 33, 300, 18, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t rx2_c17_s2[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 80, 300, 2, 1, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c17_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_GRAPH_CN ,RX_TRAIN_STRONG },  /* 收缩按图形发力  14s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, RX_TRAIN_GRAPH },  /* 按图形发力 14s */
};
static const rx_step_t * const rx2_c17_stages[] = { rx2_c17_s0,rx2_c17_s1,rx2_c17_s2,rx2_c17_s3};
static const uint8_t            rx2_c17_step_cnts[] = { 2,1,1,4};
static uint32_t rx2_c17_time_cnts[] = { 30*1000,(((4*60)+30)*1000),(5*60*1000),(20*60*1000)};

/* ---- 疗程18 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx2_c18_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c18_s1[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 33, 300, 18, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t rx2_c18_s2[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 80, 300, 2, 1, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c18_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 13, VOICE_ID_CONTRACT_GRAPH_CN ,RX_TRAIN_STRONG },  /* 收缩按图形发力  14s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, RX_TRAIN_GRAPH },  /* 按图形发力 14s */
};
static const rx_step_t * const rx2_c18_stages[] = { rx2_c18_s0,rx2_c18_s1,rx2_c18_s2,rx2_c18_s3};
static const uint8_t            rx2_c18_step_cnts[] = { 2,1,1,4};
static uint32_t rx2_c18_time_cnts[] = { 30*1000,(((4*60)+30)*1000),(5*60*1000),(20*60*1000)};

/* ---- 疗程19 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx2_c19_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c19_s1[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 33, 300, 18, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t rx2_c19_s2[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 80, 300, 2, 1, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c19_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN ,RX_TRAIN_GRAPH },  /* 按图形发力  14s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 13, VOICE_ID_BY_GRAPH_CN, RX_TRAIN_GRAPH },  /* 按图形发力 14s */
};
static const rx_step_t * const rx2_c19_stages[] = { rx2_c19_s0,rx2_c19_s1,rx2_c19_s2,rx2_c19_s3};
static const uint8_t            rx2_c19_step_cnts[] = { 2,1,1,4};
static uint32_t rx2_c19_time_cnts[] = { 30*1000,(((4*60)+30)*1000),(5*60*1000),(20*60*1000)};

/* ---- 疗程20 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx2_c20_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx2_c20_s1[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 33, 300, 18, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
};
static const rx_step_t rx2_c20_s2[] = { /* 等待 300s */
    {RX_STEP_STIM, THERAPY_BOTH, 80, 300, 2, 1, 1, 0, 0, 0, 2},  /* 20Hz/500us 8s/2s */
};
static const rx_step_t rx2_c20_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN ,RX_TRAIN_HOLD},  /* 按图形发力  14s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG},  /* 按图形发力 14s */
};
static const rx_step_t * const rx2_c20_stages[] = { rx2_c20_s0,rx2_c20_s1,rx2_c20_s2,rx2_c20_s3};
static const uint8_t            rx2_c20_step_cnts[] = { 2,1,1,4};
static uint32_t rx2_c20_time_cnts[] = { 30*1000,(((4*60)+30)*1000),(5*60*1000),(20*60*1000)};

/* --- rx2 方案数组（疗程 -> 步骤数组 + 时间/通道/电流/备注） --- */
static const rx_scheme_t rx2_schemes[] = {
    {2, 1, rx2_c1_step_cnts,rx2_c1_stages,rx2_c1_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程1 [阶段1/2+阶段2/2] */
    {1, 1, rx2_c2_step_cnts,rx2_c2_stages,rx2_c2_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程2 [阶段1/1] */
    {2, 1, rx2_c3_step_cnts,rx2_c3_stages,rx2_c3_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程3 [阶段1/2+阶段2/2] */
    {2, 1, rx2_c4_step_cnts,rx2_c4_stages,rx2_c4_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程4 [阶段1/2+阶段2/2] */
    {2, 1, rx2_c5_step_cnts,rx2_c5_stages,rx2_c5_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程5 [阶段1/2+阶段2/2] */
    {2, 1, rx2_c6_step_cnts,rx2_c6_stages,rx2_c6_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程6 [阶段1/2+阶段2/2] */
    {2, 1, rx2_c7_step_cnts,rx2_c7_stages,rx2_c7_time_cnts, THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程7 [阶段1/2+阶段2/2] */
    {2, 3, rx2_c8_step_cnts,rx2_c8_stages,rx2_c8_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程8 [阶段1/2+阶段2/2] */
    {2, 3, rx2_c9_step_cnts,rx2_c9_stages,rx2_c9_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程9 [阶段1/2+阶段2/2] */
    {2, 3, rx2_c10_step_cnts,rx2_c10_stages,rx2_c10_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程10 [阶段1/2+阶段2/2] */
    {2, 3, rx2_c11_step_cnts,rx2_c11_stages,rx2_c11_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程11 [阶段1/2+阶段2/2] */
    {2, 3, rx2_c12_step_cnts,rx2_c12_stages,rx2_c12_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程12 [阶段1/2+阶段2/2] */
    {2, 4, rx2_c13_step_cnts,rx2_c13_stages,rx2_c13_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激+生物反馈" },  /* 疗程13 [阶段1/2+阶段2/2] */
    {2, 4, rx2_c14_step_cnts,rx2_c14_stages,rx2_c14_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激+生物反馈" },  /* 疗程14 [阶段1/2+阶段2/2] */
    {2, 4, rx2_c15_step_cnts,rx2_c15_stages,rx2_c15_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激+生物反馈" },  /* 疗程15 [阶段1/2+阶段2/2] */
    {2, 4, rx2_c16_step_cnts,rx2_c16_stages,rx2_c16_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程16 [阶段1/2+阶段2/2] */
    {2, 4, rx2_c17_step_cnts,rx2_c17_stages,rx2_c17_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程17 [阶段1/2+阶段2/2] */
    {2, 4, rx2_c18_step_cnts,rx2_c18_stages,rx2_c18_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程18 [阶段1/2+阶段2/2] */
    {2, 4, rx2_c19_step_cnts,rx2_c19_stages,rx2_c19_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程19 [阶段1/2+阶段2/2] */
    {2, 4, rx2_c20_step_cnts,rx2_c20_stages,rx2_c20_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程20 [阶段1/2+阶段2/2] */
};
const rx_prescription_t rx2 = { "处方2: 盆底肌保养", "电刺激/条件性/生物反馈", 20,rx2_schemes};

/* ================= 处方3 盆底肌治疗 ================= */
/* 每个疗程一个数组；疗程号 = 文档治疗方案行序 */
/* ---- 疗程1 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx3_c1[] = {
    {RX_STEP_STIM, THERAPY_CH1, 4, 500, 28, 2, 1, 0, 0, 0, 1},  /* 4Hz/500us 28s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 50, 300, 8, 2, 1, 0, 0, 0, 2},  /* 50Hz/300us 8s/2s */
};
static const rx_step_t * const rx3_c1_stages[] = { rx3_c1 };
static const uint8_t            rx3_c1_step_cnts[] = { 2 };
static uint32_t rx3_c1_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程2 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx3_c2[] = {
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 6, 2, 1, 0, 0, 0, 1},  /* 5Hz/300us 6s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 10, 300, 5, 2, 1, 0, 0, 0, 2},  /* 10Hz/300us 5s/2s */
};
static const rx_step_t * const rx3_c2_stages[] = { rx3_c2 };
static const uint8_t            rx3_c2_step_cnts[] = { 2 };
static uint32_t rx3_c2_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程3 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx3_c3[] = {
    {RX_STEP_STIM, THERAPY_CH1, 5, 300, 28, 2, 1, 0, 0, 0, 1},  /* 5Hz/300us 28s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 20, 300, 28, 2, 1, 0, 0, 0, 2},  /* 20Hz/300us 28s/2s */
};
static const rx_step_t * const rx3_c3_stages[] = { rx3_c3 };
static const uint8_t            rx3_c3_step_cnts[] = { 2 };
static uint32_t rx3_c3_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程4 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx3_c4[] = {
    {RX_STEP_STIM, THERAPY_CH1, 8, 300, 13, 2, 1, 0, 0, 0, 1},  /* 8Hz/300us 13s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 30, 300, 13, 2, 1, 0, 0, 0, 2},  /* 30Hz/300us 13s/2s */
};
static const rx_step_t * const rx3_c4_stages[] = { rx3_c4};
static const uint8_t            rx3_c4_step_cnts[] = { 2 };
static uint32_t rx3_c4_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程5 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx3_c5[] = {
    {RX_STEP_STIM, THERAPY_CH1, 75, 100, 1, 2, 1, 0, 0, 0, 1},  /* 75Hz/100us 1s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 40, 300, 1, 2, 1, 0, 0, 0, 2},  /* 40Hz/300us 1s/2s */
};
static const rx_step_t * const rx3_c5_stages[] = { rx3_c5 };
static const uint8_t            rx3_c5_step_cnts[] = { 2 };
static uint32_t rx3_c5_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程6 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx3_c6[] = {
    {RX_STEP_STIM, THERAPY_CH1, 4, 500, 18, 4, 1, 0, 0, 0, 1},  /* 4Hz/500us 18s/4s */
    {RX_STEP_STIM, THERAPY_CH1, 30, 300, 18, 4, 1, 0, 0, 0, 2},  /* 30Hz/300us 18s/4s */
};
static const rx_step_t * const rx3_c6_stages[] = { rx3_c6 };
static const uint8_t            rx3_c6_step_cnts[] = { 2 };
static uint32_t rx3_c6_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程7 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx3_c7[] = {
    {RX_STEP_STIM, THERAPY_CH1, 45, 300, 1, 2, 1, 0, 0, 0, 1},  /* 45Hz/300us 1s/2s */
    {RX_STEP_STIM, THERAPY_CH1, 60, 300, 1, 2, 1, 0, 0, 0, 2},  /* 60Hz/300us 1s/2s */
};
static const rx_step_t * const rx3_c7_stages[] = { rx3_c7 };
static const uint8_t            rx3_c7_step_cnts[] = { 2 };
static uint32_t rx3_c7_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程8 [阶段1/2+阶段2/2] (电刺激) ---- */
static const rx_step_t rx3_c8[] = {
    {RX_STEP_STIM, THERAPY_CH1, 30, 300, 24, 6, 1, 0, 0, 0, 1},  /* 30Hz/300us 24s/6s */
    {RX_STEP_STIM, THERAPY_CH1, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 75Hz/300us 1s/2s  */
    {RX_STEP_STIM, THERAPY_CH1, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 75Hz/300us 1s/2s  */
    {RX_STEP_STIM, THERAPY_CH1, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 75Hz/300us 1s/2s  */
    {RX_STEP_STIM, THERAPY_CH1, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 75Hz/300us 1s/2s  */
    {RX_STEP_STIM, THERAPY_CH1, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 75Hz/300us 1s/2s  */
};
static const rx_step_t * const rx3_c8_stages[] = { rx3_c8 };
static const uint8_t            rx3_c8_step_cnts[] = { 6 };
static uint32_t rx3_c8_time_cnts[] = {(30*60*1000)}; 
/* ---- 疗程9 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx3_c9_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c9_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 10, 300, 5, 2, 1, 0, 0, 0, 1},  /* 10Hz/300us 5s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 5, 300, 6, 2, 1, 0, 0, 0, 2},  /* 5Hz/300us 6s/2s */
};
/* ---- 疗程10 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx3_c9_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD,0,RX_TRIGGER_BELOW_REF},  /* 收缩并保持 14s */
    {RX_STEP_COND, THERAPY_BOTH, 50, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/50Hz 10s */
};
static const rx_step_t * const rx3_c9_stages[] = { rx3_c9_s0,rx3_c9_s1,rx3_c9_s2};
static const uint8_t            rx3_c9_step_cnts[] = { 2,2,3};
static uint32_t rx3_c9_time_cnts[] = { 30*1000,(((14*60)+30)*1000),(15*60*1000)};   

/* ---- 疗程10 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx3_c10_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c10_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 10, 300, 5, 2, 1, 0, 0, 0, 1},  /* 10Hz/300us 5s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 5, 300, 6, 2, 1, 0, 0, 0, 2},  /* 5Hz/300us 6s/2s */
};
static const rx_step_t rx3_c10_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 14, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG,0,RX_TRIGGER_3FAIL},  /* 大力收缩 14s */
    {RX_STEP_COND, THERAPY_BOTH, 50, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/50Hz 10s */
};
static const rx_step_t * const rx3_c10_stages[] = { rx3_c10_s0,rx3_c10_s1,rx3_c10_s2 };
    static const uint8_t            rx3_c10_step_cnts[] = { 2,2,3};
static uint32_t rx3_c10_time_cnts[] = { 30*1000,(((14*60)+30)*1000),(15*60*1000)};   

/* ---- 疗程11 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx3_c11_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c11_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 10, 300, 5, 2, 1, 0, 0, 0, 1},  /* 10Hz/300us 5s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 5, 300, 6, 2, 1, 0, 0, 0, 2},  /* 5Hz/300us 6s/2s */
};
static const rx_step_t rx3_c11_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD,0,RX_TRIGGER_BELOW_REF},  /*收缩保持 12s */
    {RX_STEP_COND, THERAPY_BOTH, 50, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/50Hz 10s */
};
static const rx_step_t * const rx3_c11_stages[] = { rx3_c11_s0,rx3_c11_s1,rx3_c11_s2 };
static const uint8_t            rx3_c11_step_cnts[] = { 2,2,3};
static uint32_t rx3_c11_time_cnts[] = { 30*1000,(((14*60)+30)*1000),(15*60*1000)};   
/* ---- 疗程12 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx3_c12_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c12_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 10, 300, 5, 2, 1, 0, 0, 0, 1},  /* 10Hz/300us 5s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 5, 300, 6, 2, 1, 0, 0, 0, 2},  /* 5Hz/300us 6s/2s */
};
static const rx_step_t rx3_c12_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 14, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG,0,RX_TRIGGER_3FAIL},  /*大力收缩 12s */
    {RX_STEP_COND, THERAPY_BOTH, 50, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/50Hz 10s */
};
static const rx_step_t * const rx3_c12_stages[] = { rx3_c12_s0,rx3_c12_s1,rx3_c12_s2 };
static const uint8_t            rx3_c12_step_cnts[] = { 2,2,3};
static uint32_t rx3_c12_time_cnts[] =  { 30*1000,(((14*60)+30)*1000),(15*60*1000)};   
/* ---- 疗程13 [阶段1/2+阶段2/2] (电刺激+条件性电刺激) ---- */
static const rx_step_t rx3_c13_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c13_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 10, 300, 5, 2, 1, 0, 0, 0, 1},  /* 10Hz/300us 5s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 5, 300, 6, 2, 1, 0, 0, 0, 2},  /* 5Hz/300us 6s/2s */
};
static const rx_step_t rx3_c13_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD,0,RX_TRIGGER_BELOW_REF},  /*收缩保持 12s */
    {RX_STEP_COND, THERAPY_BOTH, 50, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/50Hz 10s */
};
static const rx_step_t * const rx3_c13_stages[] = { rx3_c13_s0,rx3_c13_s1,rx3_c13_s2 };
static const uint8_t            rx3_c13_step_cnts[] = { 2,2,3};
static uint32_t rx3_c13_time_cnts[] =  { 30*1000,(((14*60)+30)*1000),(15*60*1000)};   
/* ---- 疗程14 [阶段1/2+阶段2/2] (电刺激+条件性电刺激+生物反馈) ---- */
static const rx_step_t rx3_c14_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c14_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 18, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 18, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c14_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 14, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG,0,RX_TRIGGER_3FAIL},  /*大力收缩 14s */
    {RX_STEP_COND, THERAPY_BOTH, 30, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/30Hz 10s */
};
static const rx_step_t rx3_c14_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD},  /*收缩保持 12s */
};
static const rx_step_t * const rx3_c14_stages[] = { rx3_c14_s0,rx3_c14_s1,rx3_c14_s2,rx3_c14_s3 };
static const uint8_t            rx3_c14_step_cnts[] = { 2,2,3,2};
static uint32_t rx3_c14_time_cnts[] = { 30*1000,(((9*60)+30)*1000),((10*60)*1000),(10*60*1000)}; 
/* ---- 疗程15 [阶段1/2+阶段2/2] (电刺激+条件性电刺激+生物反馈) ---- */
static const rx_step_t rx3_c15_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c15_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 18, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 18, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c15_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD,0,RX_TRIGGER_ABOVE_BLUE},  /*收缩保持 12s */
    {RX_STEP_COND, THERAPY_BOTH, 30, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/30Hz 10s */
};
static const rx_step_t rx3_c15_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 15, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG},  /* 大力收缩 15s */
};
static const rx_step_t * const rx3_c15_stages[] = { rx3_c15_s0,rx3_c15_s1,rx3_c15_s2,rx3_c15_s3 };
static const uint8_t            rx3_c15_step_cnts[] = { 2,2,3,2};
static uint32_t rx3_c15_time_cnts[] = { 30*1000,(((9*60)+30)*1000),((10*60)*1000),(10*60*1000)}; 
/* ---- 疗程16 [阶段1/2+阶段2/2] (电刺激+条件性电刺激+生物反馈) ---- */
static const rx_step_t rx3_c16_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c16_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 18, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 18, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c16_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 15, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG,0,RX_TRIGGER_3FAIL},  /*大力收缩 15s */
    {RX_STEP_COND, THERAPY_BOTH, 30, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/50Hz 10s */
};
static const rx_step_t rx3_c16_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD},  /*收缩保持 12s */
};
static const rx_step_t * const rx3_c16_stages[] = { rx3_c16_s0,rx3_c16_s1,rx3_c16_s2,rx3_c16_s3 };
static const uint8_t            rx3_c16_step_cnts[] = { 2,2,3,2};
static uint32_t rx3_c16_time_cnts[] ={ 30*1000,(((9*60)+30)*1000),((10*60)*1000),(10*60*1000)}; 
/* ---- 疗程17 [阶段1/2+阶段2/2] (电刺激+条件性电刺激+生物反馈) ---- */
static const rx_step_t rx3_c17_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c17_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 18, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 18, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c17_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_HOLD_CN, RX_TRAIN_HOLD,0,RX_TRIGGER_ABOVE_BLUE},  /*收缩保持 12s */
    {RX_STEP_COND, THERAPY_BOTH, 30, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/30Hz 10s */
};
static const rx_step_t rx3_c17_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG},  /* 大力收缩 12s */
};
static const rx_step_t * const rx3_c17_stages[] = { rx3_c17_s0,rx3_c17_s1,rx3_c17_s2,rx3_c17_s3 };
static const uint8_t            rx3_c17_step_cnts[] = { 2,2,3,2};
static uint32_t rx3_c17_time_cnts[] = { 30*1000,(((9*60)+30)*1000),((10*60)*1000),(10*60*1000)}; 
/* ---- 疗程18 [阶段1/2+阶段2/2] (电刺激+条件性电刺激+生物反馈) ---- */
static const rx_step_t rx3_c18_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c18_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 18, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 8, 300, 18, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c18_s2[] = {
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN, 0, 0, 0, 0, 0, 0, 15, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG,0,RX_TRIGGER_3FAIL},  /*大力收缩 15s */
    {RX_STEP_COND, THERAPY_BOTH, 30, 300, 0, 0, 0, 10, 0, 0,1},  /* 条件->电刺激 300us/50Hz 10s */
};
static const rx_step_t rx3_c18_s3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_CN, RX_TRAIN_CONTRACT},  /* 用力 12s */
};
static const rx_step_t * const rx3_c18_stages[] = { rx3_c18_s0,rx3_c18_s1,rx3_c18_s2,rx3_c18_s3 };
static const uint8_t            rx3_c18_step_cnts[] = { 2,2,3,2};
static uint32_t rx3_c18_time_cnts[] = { 30*1000,(((9*60)+30)*1000),((10*60)*1000),(10*60*1000)}; 
/* ---- 疗程19 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx3_c19_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c19_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 8, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c19_s2[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG},  /*大力收缩 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_CN, RX_TRAIN_CONTRACT},  /* 用力 12s */
    
};

static const rx_step_t * const rx3_c19_stages[] = { rx3_c19_s0,rx3_c19_s1,rx3_c19_s2 };
static const uint8_t            rx3_c19_step_cnts[] = { 2,6,4};
static uint32_t rx3_c19_time_cnts[] = { 30*1000,(((9*60)+30)*1000),(20*60*1000)};
/* ---- 疗程20 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx3_c20_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c20_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 8, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c20_s2[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 15, VOICE_ID_CONTRACT_CN, RX_TRAIN_STRONG},  /*大力收缩 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_CONTRACT_CN, RX_TRAIN_CONTRACT},  /* 用力 12s */

};

static const rx_step_t * const rx3_c20_stages[] = { rx3_c20_s0,rx3_c20_s1,rx3_c20_s2 };
static const uint8_t            rx3_c20_step_cnts[] = { 2,6,4};
static uint32_t rx3_c20_time_cnts[] ={ 30*1000,(((9*60)+30)*1000),(20*60*1000)};
/* ---- 疗程21 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx3_c21_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c21_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 8, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c21_s2[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_CONTRACT_CN, RX_TRAIN_CONTRACT},  /* 用力 10s */
};

static const rx_step_t * const rx3_c21_stages[] = { rx3_c21_s0,rx3_c21_s1,rx3_c21_s2 };
static const uint8_t            rx3_c21_step_cnts[] = { 2,6,2};
static uint32_t rx3_c21_time_cnts[] ={ 30*1000,(((9*60)+30)*1000),(20*60*1000)};
/* ---- 疗程22 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx3_c22_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c22_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 8, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c22_s2[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG},  /* 大力收缩 18s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 16, VOICE_ID_CONTRACT_CN, RX_TRAIN_CONTRACT},  /*  用力 16s  */
  
};

static const rx_step_t * const rx3_c22_stages[] = { rx3_c22_s0,rx3_c22_s1,rx3_c22_s2 };
static const uint8_t            rx3_c22_step_cnts[] = { 2,6,4};
static uint32_t rx3_c22_time_cnts[] = { 30*1000,(((9*60)+30)*1000),(20*60*1000)};
/* ---- 疗程23 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx3_c23_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c23_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 8, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c23_s2[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG},  /* 大力收缩 18s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 16, VOICE_ID_CONTRACT_CN, RX_TRAIN_CONTRACT},  /*  用力 16s  */
   
};

static const rx_step_t * const rx3_c23_stages[] = { rx3_c23_s0,rx3_c23_s1,rx3_c23_s2 };
static const uint8_t            rx3_c23_step_cnts[] = { 2,6,4};
static uint32_t rx3_c23_time_cnts[] = { 30*1000,(((9*60)+30)*1000),(20*60*1000)};
/* ---- 疗程24 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */

static const rx_step_t rx3_c24_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c24_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 8, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 2, 1, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c24_s2[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_CONTRACT_CN, RX_TRAIN_CONTRACT},  /* 用力 10s */

};

static const rx_step_t * const rx3_c24_stages[] = { rx3_c24_s0,rx3_c24_s1,rx3_c24_s2 };
static const uint8_t            rx3_c24_step_cnts[] = { 2,6,2};
static uint32_t rx3_c24_time_cnts[] = { 30*1000,(((9*60)+30)*1000),(20*60*1000)};
/* ---- 疗程25 [阶段1/2+阶段2/2] (电刺激+生物反馈) ---- */
static const rx_step_t rx3_c25_s0[] = {
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BASELINE_EMG_CN, 0},  /* 采集 12s */
    {RX_STEP_ACQ, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_PEAK_EMG_CN, 0},  /* 采集 18s */
};
static const rx_step_t rx3_c25_s1[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 30, 300, 8, 2, 1, 0, 0, 0, 1},  /* 30Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
    {RX_STEP_STIM, THERAPY_BOTH, 75, 300, 1, 2, 1, 0, 0, 0, 2},  /* 8Hz/300us 18s/2s */
};
static const rx_step_t rx3_c25_s2[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_CONTRACT2_CN, RX_TRAIN_STRONG},  /* 大力收缩 18s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, RX_TRAIN_RELAX},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_CONTRACT_CN, RX_TRAIN_CONTRACT},  /*  用力 16s  */
   
};

static const rx_step_t * const rx3_c25_stages[] = { rx3_c25_s0,rx3_c25_s1,rx3_c25_s2 };
static const uint8_t            rx3_c25_step_cnts[] = { 2,6,4};
static uint32_t rx3_c25_time_cnts[] = { 30*1000,(((9*60)+30)*1000),(20*60*1000)};

/* --- rx3 方案数组（疗程 -> 步骤数组 + 时间/通道/电流/备注） --- */
static const rx_scheme_t rx3_schemes[] = {
    {2, 1, rx3_c1_step_cnts, rx3_c1_stages,rx3_c1_time_cnts,   THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程1 [阶段1/2+阶段2/2] */
    {2, 1, rx3_c2_step_cnts, rx3_c2_stages,rx3_c2_time_cnts,   THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程2 [阶段1/2+阶段2/2] */
    {2, 1, rx3_c3_step_cnts, rx3_c3_stages,rx3_c3_time_cnts,   THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程3 [阶段1/2+阶段2/2] */
    {2, 1, rx3_c4_step_cnts, rx3_c4_stages,rx3_c4_time_cnts,   THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程4 [阶段1/2+阶段2/2] */
    {2, 1, rx3_c5_step_cnts, rx3_c5_stages,rx3_c5_time_cnts,   THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程5 [阶段1/2+阶段2/2] */
    {2, 1, rx3_c6_step_cnts, rx3_c6_stages,rx3_c6_time_cnts,   THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程6 [阶段1/2+阶段2/2] */
    {2, 1, rx3_c7_step_cnts, rx3_c7_stages,rx3_c7_time_cnts,   THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程7 [阶段1/2+阶段2/2] */
    {2, 1, rx3_c8_step_cnts, rx3_c8_stages,rx3_c8_time_cnts,   THERAPY_CH1, "30min", "通道1 1个阴道电极棒 1个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激" },  /* 疗程8 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c9_step_cnts, rx3_c9_stages,rx3_c9_time_cnts,   THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程9 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c10_step_cnts,rx3_c10_stages,rx3_c10_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程10 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c11_step_cnts,rx3_c11_stages,rx3_c11_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程11 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c12_step_cnts,rx3_c12_stages,rx3_c12_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程12 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c13_step_cnts,rx3_c13_stages,rx3_c13_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激" },  /* 疗程13 [阶段1/2+阶段2/2] */
    {2, 4, rx3_c14_step_cnts,rx3_c14_stages,rx3_c14_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激+生物反馈" },  /* 疗程14 [阶段1/2+阶段2/2] */
    {2, 4, rx3_c15_step_cnts,rx3_c15_stages,rx3_c15_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激+生物反馈" },  /* 疗程15 [阶段1/2+阶段2/2] */
    {2, 4, rx3_c16_step_cnts,rx3_c16_stages,rx3_c16_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激+生物反馈" },  /* 疗程16 [阶段1/2+阶段2/2] */
    {2, 4, rx3_c17_step_cnts,rx3_c17_stages,rx3_c17_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激+生物反馈" },  /* 疗程17 [阶段1/2+阶段2/2] */
    {2, 4, rx3_c18_step_cnts,rx3_c18_stages,rx3_c18_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+条件性电刺激+生物反馈" },  /* 疗程18 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c19_step_cnts,rx3_c19_stages,rx3_c19_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程19 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c20_step_cnts,rx3_c20_stages,rx3_c20_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程20 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c21_step_cnts,rx3_c21_stages,rx3_c21_time_cnts, THERAPY_BOTH, "", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程21 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c22_step_cnts,rx3_c22_stages,rx3_c22_time_cnts, THERAPY_BOTH, "", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程22 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c23_step_cnts,rx3_c23_stages,rx3_c23_time_cnts, THERAPY_BOTH, "", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程23 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c24_step_cnts,rx3_c24_stages,rx3_c24_time_cnts, THERAPY_BOTH, "", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程24 [阶段1/2+阶段2/2] */
    {2, 3, rx3_c25_step_cnts,rx3_c25_stages,rx3_c25_time_cnts, THERAPY_BOTH, "", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "电刺激+生物反馈" },  /* 疗程25 [阶段1/2+阶段2/2] */
};
rx_prescription_t rx3 = { "处方3: 盆底肌治疗", "电刺激/条件性/生物反馈", 25, rx3_schemes};

/* ================= 处方4 凯格尔训练 ================= */
/* 每个疗程一个数组；疗程号 = 文档治疗方案行序 */
/* ---- 疗程1 (生物反馈（初级）) ---- */
static const rx_step_t rx4_c1[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 10s */
};
static const rx_step_t * const rx4_c1_stages[] = { rx4_c1};
static const uint8_t            rx4_c1_step_cnts[] = { 4};
static uint32_t rx4_c1_time_cnts[] = { 30*60*1000};
/* ---- 疗程2 (生物反馈（初级）) ---- */
static const rx_step_t rx4_c2[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 10s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
};
static const rx_step_t * const rx4_c2_stages[] = { rx4_c2};
static const uint8_t            rx4_c2_step_cnts[] = { 4};
static uint32_t rx4_c2_time_cnts[] = { 30*60*1000};
/* ---- 疗程3 (生物反馈（初级）) ---- */
static const rx_step_t rx4_c3[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 10s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
};
static const rx_step_t * const rx4_c3_stages[] = { rx4_c3};
static const uint8_t            rx4_c3_step_cnts[] = { 4};
static uint32_t rx4_c3_time_cnts[] = { 30*60*1000};
/* ---- 疗程4 (生物反馈（初级）) ---- */
static const rx_step_t rx4_c4[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 10s */
};
static const rx_step_t * const rx4_c4_stages[] = { rx4_c4};
static const uint8_t            rx4_c4_step_cnts[] = { 4};
static uint32_t rx4_c4_time_cnts[] = { 30*60*1000};
/* ---- 疗程5 (生物反馈（初级）) ---- */
static const rx_step_t rx4_c5[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 10s */
};
static const rx_step_t * const rx4_c5_stages[] = { rx4_c5};
static const uint8_t            rx4_c5_step_cnts[] = { 4};
static uint32_t rx4_c5_time_cnts[] = { 30*60*1000};
/* ---- 疗程6 (生物反馈（初级）) ---- */
static const rx_step_t rx4_c6[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 10, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 10s */
};
static const rx_step_t * const rx4_c6_stages[] = { rx4_c6};
static const uint8_t            rx4_c6_step_cnts[] = { 4};
static uint32_t rx4_c6_time_cnts[] = { 30*60*1000};
/* ---- 疗程7 (生物反馈（中级）) ---- */
static const rx_step_t rx4_c7[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 18s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
};
static const rx_step_t * const rx4_c7_stages[] = { rx4_c7};
static const uint8_t            rx4_c7_step_cnts[] = { 4};
static uint32_t rx4_c7_time_cnts[] = { 30*60*1000};
/* ---- 疗程8 (生物反馈（中级）) ---- */
static const rx_step_t rx4_c8[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
};
static const rx_step_t * const rx4_c8_stages[] = { rx4_c8};
static const uint8_t            rx4_c8_step_cnts[] = { 4};
static uint32_t rx4_c8_time_cnts[] = { 30*60*1000};
/* ---- 疗程9 (生物反馈（中级）) ---- */
static const rx_step_t rx4_c9[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
};
static const rx_step_t * const rx4_c9_stages[] = { rx4_c9};
static const uint8_t            rx4_c9_step_cnts[] = { 4};
static uint32_t rx4_c9_time_cnts[] = { 30*60*1000};
/* ---- 疗程10 (生物反馈（中级）) ---- */
static const rx_step_t rx4_c10[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 18s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 20s */
};
static const rx_step_t * const rx4_c10_stages[] = { rx4_c10};
static const uint8_t            rx4_c10_step_cnts[] = { 4};
static uint32_t rx4_c10_time_cnts[] = { 30*60*1000};
/* ---- 疗程11 (生物反馈（中级）) ---- */
static const rx_step_t rx4_c11[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 20s */
};
static const rx_step_t * const rx4_c11_stages[] = { rx4_c11};
static const uint8_t            rx4_c11_step_cnts[] = { 4};
static uint32_t rx4_c11_time_cnts[] = { 30*60*1000};
/* ---- 疗程12 (生物反馈（中级）) ---- */
static const rx_step_t rx4_c12[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 12, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 12s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 20s */
};
static const rx_step_t * const rx4_c12_stages[] = { rx4_c12};
static const uint8_t            rx4_c12_step_cnts[] = { 4};
static uint32_t rx4_c12_time_cnts[] = { 30*60*1000};
/* ---- 疗程13 (生物反馈（高级）) ---- */
static const rx_step_t rx4_c13[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 20s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 18s */
};
static const rx_step_t * const rx4_c13_stages[] = { rx4_c13};
static const uint8_t            rx4_c13_step_cnts[] = { 4};
static uint32_t rx4_c13_time_cnts[] = { 30*60*1000};
/* ---- 疗程14 (生物反馈（高级）) ---- */
static const rx_step_t rx4_c14[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 20s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 18s */
};
static const rx_step_t * const rx4_c14_stages[] = { rx4_c14};
static const uint8_t            rx4_c14_step_cnts[] = { 4};
static uint32_t rx4_c14_time_cnts[] = { 30*60*1000};
/* ---- 疗程15 (生物反馈（高级）) ---- */
static const rx_step_t rx4_c15[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 14, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 14s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 20s */
};
static const rx_step_t * const rx4_c15_stages[] = { rx4_c15};
static const uint8_t            rx4_c15_step_cnts[] = { 4};
static uint32_t rx4_c15_time_cnts[] = { 30*60*1000};
/* ---- 疗程16 (生物反馈（高级）) ---- */
static const rx_step_t rx4_c16[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 22, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 22s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 20s */
};
static const rx_step_t * const rx4_c16_stages[] = { rx4_c16};
static const uint8_t            rx4_c16_step_cnts[] = { 4};
static uint32_t rx4_c16_time_cnts[] = { 30*60*1000};
/* ---- 疗程17 (生物反馈（高级）) ---- */
static const rx_step_t rx4_c17[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 18, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 18s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 20s */
};
static const rx_step_t * const rx4_c17_stages[] = { rx4_c17};
static const uint8_t            rx4_c17_step_cnts[] = { 4};
static uint32_t rx4_c17_time_cnts[] = { 30*60*1000};
/* ---- 疗程18 (生物反馈（高级）) ---- */
static const rx_step_t rx4_c18[] = {
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 14, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 14s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 8, VOICE_ID_RELAX_CN, 0},  /* 放松 8s */
    {RX_STEP_TRAIN_INSTR, 0, 0, 0, 0, 0, 0, 20, VOICE_ID_BY_GRAPH_CN, 3},  /* 按图形发力 20s */
};
static const rx_step_t * const rx4_c18_stages[] = { rx4_c18};
static const uint8_t            rx4_c18_step_cnts[] = { 4};
static uint32_t rx4_c18_time_cnts[] = { 30*60*1000};
/* --- rx4 方案数组（疗程 -> 步骤数组 + 时间/通道/电流/备注） --- */
static const rx_scheme_t rx4_schemes[] = {
    {0, 1, rx4_c1_step_cnts, rx4_c1_stages,rx4_c1_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（初级）" },  /* 疗程1 */
    {0, 1, rx4_c2_step_cnts, rx4_c2_stages,rx4_c2_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（初级）" },  /* 疗程2 */
    {0, 1, rx4_c3_step_cnts, rx4_c3_stages,rx4_c3_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（初级）" },  /* 疗程3 */
    {0, 1, rx4_c4_step_cnts, rx4_c4_stages,rx4_c4_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（初级）" },  /* 疗程4 */
    {0, 1, rx4_c5_step_cnts, rx4_c5_stages,rx4_c5_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（初级）" },  /* 疗程5 */
    {0, 1, rx4_c6_step_cnts, rx4_c6_stages,rx4_c6_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（初级）" },  /* 疗程6 */
    {0, 1, rx4_c7_step_cnts, rx4_c7_stages,rx4_c7_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（中级）" },  /* 疗程7 */
    {0, 1, rx4_c8_step_cnts, rx4_c8_stages,rx4_c8_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（中级）" },  /* 疗程8 */
    {0, 1, rx4_c9_step_cnts, rx4_c9_stages,rx4_c9_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（中级）" },  /* 疗程9 */
    {0, 1, rx4_c10_step_cnts, rx4_c10_stages,rx4_c10_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（中级）" },  /* 疗程10 */
    {0, 1, rx4_c11_step_cnts, rx4_c11_stages,rx4_c11_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（中级）" },  /* 疗程11 */
    {0, 1, rx4_c12_step_cnts, rx4_c12_stages,rx4_c12_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（中级）" },  /* 疗程12 */
    {0, 1, rx4_c13_step_cnts, rx4_c13_stages,rx4_c13_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（高级）" },  /* 疗程13 */
    {0, 1, rx4_c14_step_cnts, rx4_c14_stages,rx4_c14_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（高级）" },  /* 疗程14 */
    {0, 1, rx4_c15_step_cnts, rx4_c15_stages,rx4_c15_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（高级）" },  /* 疗程15 */
    {0, 1, rx4_c16_step_cnts, rx4_c16_stages,rx4_c16_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（高级）" },  /* 疗程16 */
    {0, 1, rx4_c17_step_cnts, rx4_c17_stages,rx4_c17_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（高级）" },  /* 疗程17 */
    {0, 1, rx4_c18_step_cnts, rx4_c18_stages,rx4_c18_time_cnts, THERAPY_BOTH, "30min", "通道1、2 1个阴道电极棒 3个电极片", "治疗前和治疗中均可调节，步进1mA，最大90 mA", "生物反馈（高级）" },  /* 疗程18 */
};
const rx_prescription_t rx4 = { "处方4: 凯格尔训练", "生物反馈", 18, rx4_schemes };

/* ===== 4 处方汇总（按处方号） ===== */
const rx_prescription_t *const g_rx_all[RX_MAX] = { &rx1, &rx2, &rx3, &rx4 };
