/**
  ******************************************************************************
  * @文件名称   PosReh_En_ui_event.h
  * @文件描述   英文产后康复页事件回调声明（对应中文 PosReh_Page_ui_event.h）。
  ******************************************************************************
  */

#ifndef __POSREH_EN_UI_EVENT_H__
#define __POSREH_EN_UI_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

    /* 页面按钮事件回调 */
    void PosReh_En_InstrPage_Event_cb(lv_event_t* e);
    void PosReh_En_SelTreatPage_Event_cb(lv_event_t* e);
    void PosReh_En_TreIns_Event_cb(lv_event_t* e);
    void PosReh_En_P4SetBtn_Event_cb(lv_event_t* e);
    void PosReh_En_P5Btn_Event_cb(lv_event_t* e);

    /* [我们] B4 按键导航回调 */
    void PosReh_En_Page1_Key_cb(lv_event_t* e);
    void PosReh_En_Page2_Key_cb(lv_event_t* e);
    void PosReh_En_Page3_Key_cb(lv_event_t* e);
    void PosReh_En_Page4_Key_cb(lv_event_t* e);
    void PosReh_En_Page5_Key_cb(lv_event_t* e);
    void PosReh_En_MsgBox_Key_cb(lv_event_t* e);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __POSREH_EN_UI_EVENT_H__ */
