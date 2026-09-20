#ifndef __MENU_UI_EVNET_CB_H__
#define __MENU_UI_EVNET_CB_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#include "lvgl.h"
    void Menu_button_event_cb(lv_event_t* e);

    /* [我们] B4/B5 共享控件回调（同事 MenuPage 原装；B5-A Warn_Btn_Event_cb→Back_Btn_Event_cb） */
    void Back_Btn_Event_cb(lv_event_t* e);
    void TreIns_Widget_event_cb(lv_event_t* e);
    void Treat_Widget_Event_cb(lv_event_t* e);
    void Child_Slider_Event_cb(lv_event_t* e);
    void ThePage_Obj_scroll_cb(lv_event_t* e);
    void ThePage_HisBtn_scroll_cb(lv_event_t* e);
    void ThePage_StrBtn_scroll_cb(lv_event_t* e);

    /* [我们] B5-B 治疗页图表事件（同事 PelFlo_TheChild_event.c 原装；timer_next/治疗双 timer 留 B5-C） */
    void ThePage2_Obj_scroll_cb(lv_event_t* e);
    void Chart_Btn_Event_cb(lv_event_t* e);
    void Chart_Draw_event_cb(lv_event_t* e);
    void Chart3_Stage3_Add_data(lv_timer_t* t);
   

#endif /*LV_USE_BUTTON*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
