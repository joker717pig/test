/**
  ******************************************************************************
  * @文件名称   set_En_ui_event.h
  * @文件描述   英文设置页事件回调声明（两套 UI，阶段 E2）
  ******************************************************************************
  */

#ifndef __SET_EN_UI_EVENT_H__
#define __SET_EN_UI_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "lang.h"

    void Set_En_Roller_event_cb(lv_event_t* e);
    void Set_En_List_event_handler(lv_event_t* e);
    void Set_En_timer_cb(lv_timer_t* t);
    void Set_En_Slider_Event_cb(lv_event_t* e);
    void Set_En_Select_Language_event_cb(lv_event_t* e);
    void Set_En_Date_event_cb(lv_event_t* e);
    void Set_En_Time_event_cb(lv_event_t* e);
    void Set_En_Page_Esc_cb(lv_event_t* e);        /* [我们] 无按钮页面的 ESC 处理 */
    void Set_En_Roller_Key_cb(lv_event_t* e);      /* [我们] 滚轮 ENTER 前进焦点 / ESC 返回 */
    void Set_En_OTABtn_Event_cb(lv_event_t* e);
  

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __SET_EN_UI_EVENT_H__ */
