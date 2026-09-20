/**
  ******************************************************************************
  * @文件名称   therapy_pn.c
  * @文件描述   产后康复处方表（功能1）：3 方案，复用 rx_scheme_t（纯电刺激单段）
  *             数据源：EDA_EMG/doc/治疗处方/利玮生物刺激反馈仪处方2026052501.md
  *
  *             方案核对（与 md 全表一致）：
  *               腹直肌分离  40Hz/400us 10s/10s  双通道
  *               子宫复旧    35Hz/200us  5s/5s   通道1
  *               催乳        40Hz/320us 10s/10s  双通道
  ******************************************************************************
  */
#include "therapy_pn.h"
#include "voice.h"   /* VOICE_ID_* 语音段枚举 */

/* 方案1 腹直肌分离 */
static const rx_step_t pn1_steps[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 40, 400, 10, 10, 1, 0, 0, 0, 1},  /* 40Hz/400us 10s/10s */
};
static const rx_step_t * const  pn1_stages[] = { pn1_steps };
static const uint8_t            pn1_step_cnts[] = { 1 };
static const uint32_t           pn1_time_cnts[] = {(30*60*1000)}; 
/* 方案2 子宫复旧 */
static const rx_step_t pn2_steps[] = {
    {RX_STEP_STIM, THERAPY_CH1, 35, 200, 5, 5, 1, 0, 0, 0, 1},  /* 35Hz/200us 5s/5s */
};
static const rx_step_t * const  pn2_stages[] = { pn2_steps };
static const uint8_t            pn2_step_cnts[] = { 1 };
static const uint32_t           pn2_time_cnts[] = {(30*60*1000)}; 
/* 方案3 催乳 */
static const rx_step_t pn3_steps[] = {
    {RX_STEP_STIM, THERAPY_BOTH, 40, 320, 10, 10, 1, 0, 0, 0, 1},  /* 40Hz/320us 10s/10s */
};
static const rx_step_t * const  pn3_stages[] = { pn3_steps };
static const uint8_t            pn3_step_cnts[] = { 1 };
static const uint32_t           pn3_time_cnts[] = {(30*60*1000)}; 
static const rx_scheme_t pn_schemes[] = {
    {1, 1, pn1_step_cnts,pn1_stages, pn1_time_cnts,THERAPY_BOTH, "30 min", "双通道", "治疗前和治疗中均可调节，步进1mA", "电刺激" },  /* 腹直肌分离 */
    {1, 1, pn2_step_cnts,pn2_stages, pn2_time_cnts, THERAPY_CH1,  "30 min", "通道1",   "治疗前和治疗中均可调节，步进1mA", "电刺激" },  /* 子宫复旧 */
    {1, 1, pn3_step_cnts,pn3_stages, pn3_time_cnts, THERAPY_BOTH, "30 min", "双通道", "治疗前和治疗中均可调节，步进1mA", "电刺激" },  /* 催乳 */
};

const rx_prescription_t rx_pn = { "产后康复", "电刺激", 3, pn_schemes };
