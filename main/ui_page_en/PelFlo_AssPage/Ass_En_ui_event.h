/**
  ******************************************************************************
  * @文件名称   Ass_En_ui_event.h
  * @文件描述   英文盆底评估页事件回调声明（仅跳转相关的 3 个；其余语言无关的
  *            中文回调在 PelFlo_Ass_ui_event.h 中直接复用）。
  ******************************************************************************
  */

#ifndef __ASS_EN_UI_EVENT_H__
#define __ASS_EN_UI_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

    /* 英文评估页第1页按钮焦点/导航/点击回调（跳转改英文 Ass_En_Page_Load） */
    void Ass_En_Page1_Btn_FocEvent_cb(lv_event_t* e);

    /* 英文生成报告定时器回调（跳转改英文 Ass_En_Page_Load） */
    void Ass_En_report_timer_cb(lv_timer_t* t);

    /* 英文曲线定时器回调（评估完成跳转改英文 Ass_En_Page_Load） */
    void Ass_En_Aemg_add_data(lv_timer_t* t);
    void Ass_En_Aemg_add_data_real(lv_timer_t* t);

    /* 英文健康档案记录项回调（生成英文记录表格） */
    void Ass_En_Record_Menu_event_cb(lv_event_t* e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __ASS_EN_UI_EVENT_H__ */
