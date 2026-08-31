#ifndef MY_FUNCTION_BTN_H
#define MY_FUNCTION_BTN_H

#ifdef __cplusplus
extern "C" {
#endif

#define TRIGGER_BTN_BG_COLOR        0X273E50

#define BTN_RECORD_BG_COLOR         0X273E50        /*心电数据记录按钮背景颜色*/
#define BTN_RECORD_WIDE             80              /*心电数据记录按钮宽度*/
#define BTN_RECORD_HIGH             40              /*心电数据记录按钮高度*/
#define BTN_BACK_BG_COLOR           0X273E50        /*心电数据追溯按钮背景颜色*/
#define BTN_BACK_WIDE               80              /*心电数据追溯按钮宽度*/
#define BTN_BACK_HIGH               40              /*心电数据追溯按钮高度*/
#define BTN_FETCH_BG_COLOR          0X273E50        /*心电数据调取按钮背景颜色*/
#define BTN_FETCH_WIDE              80              /*心电数据调取按钮宽度*/
#define BTN_FETCH_HIGH              40              /*心电数据调取按钮高度*/
#define BTN_REAL_BG_COLOR           0X273E50        /*心电数据调取按钮背景颜色*/
#define BTN_REAL_WIDE               80              /*心电数据调取按钮宽度*/
#define BTN_REAL_HIGH               40              /*心电数据调取按钮高度*/

    /*********************
     *      INCLUDES
     *********************/
    static void my_btn_trigger(void);
    static void my_btn_record(void);
    static void my_btn_fetch(void);
    static void my_obj_trigeer(void);
    static void my_trigger_dropdown(void);
    static void my_btn_real_display(void);
    static void my_btn_period(void);
    static void my_pressure_dropdown(void);
    void my_btn_ui(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LVGL_SRC_H */#pragma oncea once
