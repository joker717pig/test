#ifndef __PELFLO_THECHILD_EVENT_H__
#define __PELFLO_THECHILD_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1


#include "lvgl.h"
#include "menu_ui.h"    /* [我们] Treat_Timer_Ctx_t（The_Page5_ShowConfirm 参数） */

    void Child_Page_StartBtn_Event_cb(lv_event_t* e);
    void ChildPage_StageBtn_Event_cb(lv_event_t* e);
    void ChildPage_TreIns_Event_cb(lv_event_t* e);
    void ChildPage_P4SetBtn_Event_cb(lv_event_t* e);
    void ChildPage_P5Btn_Event_cb(lv_event_t* e);

    void Chart_Stage3_Add_data(lv_timer_t* t);
    void Chart2_Stage3_Add_data(lv_timer_t* t);
     void Chart_Stage2_4_Add_data(lv_timer_t* t);
    /* Chart_Draw_event_cb / Chart_Btn_Event_cb / Chart_Stage2_4_Add_data / Chart3_Stage3_Add_data
     * 已由 B5-B 实现在 menu_ui_event_cb.c（声明见 menu_ui_event_cb.h），本文件不再定义 */

    /* [我们] B5-C 通用守则清理接口（定义在 Child_ThePage.c） */
    void The_BackToMenu(void);
    void The_Page5_ShowConfirm(Treat_Timer_Ctx_t* ctx);
#endif /*LV_USE_BUTTON*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*MY_UI_PAGE1_H*/
