#ifndef __MAIN_UI_EVENT_H__
#define __MAIN_UI_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

    void HisBtn_scroll_cb(lv_event_t* e);
    void AssBtn_scroll_cb(lv_event_t* e);
    void Obj_scroll_cb(lv_event_t* e);
    void AssPage1_Btn_FocEvent_cb(lv_event_t* e);
    void Aemg_add_data(lv_timer_t* t);
    void Mydraw_event_cb(lv_event_t* e);
    void Mydraw2_event_cb(lv_event_t* e);
    void chart_draw_event_cb(lv_event_t* e);
    void Draw_EcgTable_event_cb(lv_event_t* e);
    void Record_Menu_event_cb(lv_event_t* e);
    void report_timer_cb(lv_timer_t* t);
    void BackToMenu_Event_cb(lv_event_t* e);
    void Ass_Page_Esc_cb(lv_event_t* e);
    void Table_Header_scroll_cb(lv_event_t* e);
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*MY_UI_PAGE1_H*/
