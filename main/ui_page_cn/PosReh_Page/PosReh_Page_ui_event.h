#ifndef __POSREH_PAGE_UI_EVENT_H__
#define __POSREH_PAGE_UI_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "lvgl.h"

    void InstrPage_Event_cb(lv_event_t* e);
    void SelTreatPage_Event_cb(lv_event_t* e);
    void PosReh_TreIns_Event_cb(lv_event_t* e);
    void PosReh_P4SetBtn_Event_cb(lv_event_t* e);
    void PosReh_P5Btn_Event_cb(lv_event_t* e);

    /* [我们] B4 按键导航回调 */
    void PosReh_Page1_Key_cb(lv_event_t* e);
    void PosReh_Page2_Key_cb(lv_event_t* e);
    void PosReh_Page3_Key_cb(lv_event_t* e);
    void PosReh_Page4_Key_cb(lv_event_t* e);
    void PosReh_Page5_Key_cb(lv_event_t* e);
    void PosReh_MsgBox_Key_cb(lv_event_t* e);
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
