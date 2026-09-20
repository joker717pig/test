/**
  ******************************************************************************
  * @文件名称   shared_En_ui.h
  * @文件描述   英文共享控件（ui_page_en 各英文页面复用）
  *            对应中文 menu_ui.c 的 Warn_Window / Treat_Widget / ParSet_Widget，
  *            仅标签文字翻译为英文；TreIns_Widget 无硬编码中文（文本全部来自
  *            TreIns_Data_t 数据），英文页面直接复用中文版 TreIns_Widget。
  *            事件回调复用中文语言无关版（menu_ui_event_cb.h）：
 *              Back_En_Window    → Back_Btn_Event_cb
  *              Treat_En_Widget   → Treat_Widget_Event_cb
  *              ParSet_En_Widget  → Child_Slider_Event_cb
  *            命名：符号统一 *_En（防与中文链接冲突）。
  ******************************************************************************
  */

#ifndef __SHARED_EN_UI_H__
#define __SHARED_EN_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "menu_ui.h"   /* TreIns_Data_t / Param_Data_t / Treat_Timer_Ctx_t 类型 */

    /* 提示窗（无按钮，纯提示，英文） */
    void Warn_En_Window(Window_Data_t* data);

    /* 返回确认弹窗（Yes/No，英文）：CLICKED 事件绑 Back_Btn_Event_cb */
    void Back_En_Window(Window_Data_t* data, const char* txt);

    /* 电刺激治疗界面（英文标签） */
    void Treat_En_Widget(lv_obj_t* page_cont, Treat_Timer_Ctx_t* data);

    /* 电刺激参数设置界面（英文标签） */
    void ParSet_En_Widget(lv_obj_t* page_cont, Param_Data_t* data);

    /* 阶段疗程说明界面（英文标签，字体/坐标对齐客户定稿） */
    void TreIns_En_Widget(lv_obj_t* page_cont, TreIns_Data_t* data);

    /* 疗程说明界面（英文；对应中文 Instr_Widget，字体/坐标对齐客户定稿） */
    void Instr_En_Widget(lv_obj_t* page_cont, Instr_Data_t* data);

    /* 选择疗程界面（英文；Flex 换行自动填充全部疗程格，对应中文 Select_Widget2） */
    void Select_En_Widget2(lv_obj_t* page_cont, Select_Data_t* data);

    /* 曲线图页面（英文；对应中文 Chart_widget，内部 Creat_En_Chart/draw_pressure_excel_En） */
    void Chart_En_widget(lv_obj_t* page_cont, Chart_Data_t* widget);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __SHARED_EN_UI_H__ */
