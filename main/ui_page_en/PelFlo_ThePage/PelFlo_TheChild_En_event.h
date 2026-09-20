#ifndef __PELFLO_THECHILD_EN_EVENT_H__
#define __PELFLO_THECHILD_EN_EVENT_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#include "lvgl.h"
#include "menu_ui.h"    /* [我们] Treat_Timer_Ctx_t（The_En_Page5_ShowConfirm 参数） */

    void Child_Page_StartBtn_En_Event_cb(lv_event_t* e);
    void ChildPage_StageBtn_En_Event_cb(lv_event_t* e);
    void ChildPage_TreIns_En_Event_cb(lv_event_t* e);
    void ChildPage_P4SetBtn_En_Event_cb(lv_event_t* e);
    void ChildPage_P5Btn_En_Event_cb(lv_event_t* e);

    void Chart_Stage3_En_Add_data(lv_timer_t* t);
    void Chart2_Stage3_En_Add_data(lv_timer_t* t);
    /* [我们] 中文 menu_ui_event_cb.c 的 Chart3_Stage3_Add_data / Chart_Stage2_4_Add_data
     * 与 Chart_Btn_Event_cb 含中文弹窗文字，英文版自建（文字换英文） */
    void Chart3_Stage3_En_Add_data(lv_timer_t* t);
    void Chart_Stage2_4_En_Add_data(lv_timer_t* t);
    void Chart_Btn_En_Event_cb(lv_event_t* e);

    /* [我们] B5-C 通用守则清理接口（定义在 Child_ThePage_En.c） */
    void The_En_BackToMenu(void);
    void The_En_Page5_ShowConfirm(Treat_Timer_Ctx_t* ctx);
#endif /*LV_USE_BUTTON*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*__PELFLO_THECHILD_EN_EVENT_H__*/
