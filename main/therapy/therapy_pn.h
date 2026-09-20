/**
  ******************************************************************************
  * @文件名称   therapy_pn.h
  * @文件描述   产后康复处方表（功能1）：复用 therapy_rx.h 的 rx_scheme_t / 通道掩码
  *             数据源：EDA_EMG/doc/治疗处方/利玮生物刺激反馈仪处方2026052501.md
  *             3 方案：腹直肌分离 / 子宫复旧 / 催乳（每方案单段电刺激）
  *             只落基础元数据（时间/通道/电流）；频次疗程不落
  ******************************************************************************
  */
#ifndef THERAPY_PN_H
#define THERAPY_PN_H

#include "therapy_rx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 产后康复处方（3 方案，定义在 therapy_pn.c） */
extern const rx_prescription_t rx_pn;

#ifdef __cplusplus
}
#endif

#endif /* THERAPY_PN_H */
