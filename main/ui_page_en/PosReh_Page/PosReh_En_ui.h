/**
  ******************************************************************************
  * @文件名称   PosReh_En_ui.h
  * @文件描述   英文产后康复页（Postnatal Rehabilitation，对应中文 ui_page_cn/PosReh_Page）
  *            完全独立实现：英文数据实例 + 英文页面 + 英文共享控件（shared_En_ui.c），
  *            与中文页仅共享 NVS 参数存储与页面管理结构（lv_my_ui_pagePosReh_t 等）。
  *            当前功能状态复用中文 PosReh_Get_CurFunc/PosReh_Set_CurFunc（g_Cur_Func）。
  *            命名规则：.c/.h 统一 *_En，符号统一 PosReh_En_*（防链接冲突）。
  ******************************************************************************
  */

#ifndef __POSREH_EN_UI_H__
#define __POSREH_EN_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "PosReh_Page_ui.h"   /* 复用 PosReh_Widget_t / PosReh_PageID_t / Func_SUM / 页面管理结构声明 */

    /* ===== 英文数据实例（独立于中文，含英文文本） ===== */
    extern TreIns_Data_t*    PosReh_En_Treins_list[Func_SUM];   /* 疗程说明（英文） */
    extern Param_Data_t*     PosReh_En_Param_list[Func_SUM];    /* 参数设置（英文） */
    extern Treat_Timer_Ctx_t* PosReh_En_CtxData[Func_SUM];      /* 电刺激治疗上下文（英文） */
    extern PosReh_Widget_t   PosReh_En_Widget;                  /* 页面控件数据（英文） */

    /* ===== 页面入口 / 通用守则清理（对应中文 PosReh_* 接口） ===== */
    void PosReh_En_Page_Load(PosReh_PageID_t page_id);
    void PosReh_En_LeaveGroup(void);
    void PosReh_En_StopTimer(void);
    void PosReh_En_BackToMenu(void);

    /* ===== 参数存储（NVS）：英文数据实例 ↔ 同一 NVS 存储 ===== */
    void PosReh_En_Param_ApplySaved(void);
    void PosReh_En_Param_SaveAll(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __POSREH_EN_UI_H__ */
