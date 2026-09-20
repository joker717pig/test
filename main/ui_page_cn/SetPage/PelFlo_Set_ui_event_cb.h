#ifndef __PELFLO_SET_UI_EVENT_H__
#define __PELFLO_SET_UI_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "lang.h"
#include "PelFlo_Set_ui.h"   /* [我们] OTA_Widget_t（OTA_Page_Enter 入参） */

    void roller_event_cb(lv_event_t* e);
    void SetPage_List_event_handler(lv_event_t* e);
    void Set_timer_cb(lv_timer_t* t);
    void SetPage_Slider_Event_cb(lv_event_t* e);
    void SetPage_Select_Language_event_cb(lv_event_t* e);
    void SetPage_Date_event_cb(lv_event_t* e);
    void SetPage_Time_event_cb(lv_event_t* e);
    void Set_Page_Esc_cb(lv_event_t* e);        /* [我们] 无按钮页面的 ESC 处理 */
    void Set_Roller_Key_cb(lv_event_t* e);      /* [我们] 滚轮 ENTER 前进焦点 / ESC 返回 */
    void SetPage_OTABtn_Event_cb(lv_event_t* e);
    void OTA_Page_Enter(OTA_Widget_t* w);   /* [我们] 进 OTA 页：发起检查 + 启动轮询 */
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif 

