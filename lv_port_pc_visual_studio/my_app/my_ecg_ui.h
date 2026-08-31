#ifndef MY_ECG_UI_H
#define MY_ECG_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAIN_BG_COLOR                   0X3A5F7E        /*主屏幕背景颜色*/
#define ECG_CHART_BG_COLOR              0X0C0C0C        /*心电波形背景颜色*/
#define ECG_LABEL_BG_COLOR              0XF44336        /*心电波形标签字体颜色*/
#define ECG_CHART_WIDE                  700             /*心电波形折线图宽度*/
#define ECG_CHART_HIGE                  360             /*心电波形折线图高度度*/
#define ECG_OBJ_HEART_BG_COLOR          0X0C0C0C        /*心电数据背景颜色*/
#define ECG_OBJ_HEART_BG_WIDE           300             /*心电数据背景宽度*/
#define ECG_OBJ_HEART_BG_HIGH           180             /*心电数据背景宽度*/
#define ECG_LABEL_HEART_TEXT_COLOR      0XF44336        /*心电数据标签字体颜色*/
#define ECG_OBJ_MPA_BG_COLOR            0X0C0C0C        /*气压数据背景颜色*/
#define ECG_OBJ_MPA_BG_WIDE             150             /*气压数据背景宽度*/
#define ECG_OBJ_MAP_BG_HIGH             180             /*气压数据背景宽度*/
#define ECG_LABEL_MPA_TEXT_COLOR        0X55B155        /*气压数据标签字体颜色*/

    /*********************
     *      INCLUDES
     *********************/
    void ecg_chart(void);
    void ecg_scale(void);
    void my_ecg_ui(void);
    void my_ecg_background(void);
    void my_ecg_data_display(void);
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* LVGL_SRC_H */#pragma once
