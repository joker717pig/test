#ifndef __FRAME_H__
#define __FRAME_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#define BATTERY_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery.bin"      // 电量10%图表
#define BATTERY_10_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery10.bin"      // 电量10%图表   
#define BATTERY_20_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery20.bin"      // 电量20%图表   
#define BATTERY_30_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery30.bin"      // 电量30%图表   
#define BATTERY_40_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery40.bin"      // 电量40%图表   
#define BATTERY_50_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery50.bin"      // 电量50%图表   
#define BATTERY_60_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery60.bin"      // 电量60%图表   
#define BATTERY_70_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery70.bin"      // 电量70%图表   
#define BATTERY_80_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery80.bin"      // 电量80%图表   
#define BATTERY_90_ICON     "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery90.bin"      // 电量90%图表   
#define BATTERY_100_ICON    "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery100.bin"     // 电量100%图表   

#include "lvgl.h"
    typedef struct {
        lv_obj_t* Label_Time;
        lv_obj_t* Label_Batt;   /* 电量百分比 */
        lv_obj_t* Img_Charge;   /* 充电状态图标 */
        lv_obj_t* Img_battery;  /* 电量状态图标 */
    }Frame_Widget_t;

void Frame_ui(void);

#endif

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
