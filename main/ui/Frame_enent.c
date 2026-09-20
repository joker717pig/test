/**
  ******************************************************************************
  * @文件名称   Frame_enent.c
  * @文件描述   Frame 顶栏时间刷新回调
  *            移植自 EDA_EMG_LVGL_PC\Frame\Frame_enent.c（同事 zqt 原装）
  * @适配说明  localtime_s → localtime_r（POSIX，ESP32 newlib 可用）
  ******************************************************************************
  */

#include "Frame_enent.h"
#include "Frame.h"
#include "time.h"
#include "basic.h"
#include "app_heartbeat.h"
#include "app_modbus_reg.h"
extern lv_my_ui_page_date_t* g_page_list[APP_SUM];
extern Frame_Widget_t Frame_widget;


/**
 * @author zqt
 * @brief 获取系统时间回调函数
 * @param timer
 */
void update_time_cb(lv_timer_t* timer)
{
    time_t now;
    struct tm tm;

    // 获取当前本地时间
    now = time(NULL);
    localtime_r(&now, &tm);

    static time_t start_time = 0;//初始时长为0
    if (start_time == 0) { start_time = now; }

    char time_str[9]; // 存放格式化后的时间字符串，HH:MM:SS
    char date_str[20]; // 存放格式化后的日期字符串，YYYY-MM-DD
    int total_minute = (now - start_time) / 60; // 计算累计时长（分钟）

    lv_label_set_text_fmt(Frame_widget.Label_Time, "%02d：%02d", tm.tm_hour, tm.tm_min); // 更新时间标签的文本
    //lv_label_set_text_fmt(ui_widget.Date_Label, "%04d/%02d/%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); // 更新日期标签的文本

}

/**
 * @author ai
 * @brief 顶栏电池信息刷新回调（电量百分比 + 充电图标显隐）
 * @param timer
 */
void update_batt_cb(lv_timer_t* timer)
{
    app_batt_t batt;

    /* 尚无有效数据：保留占位显示 */
    if (!app_heartbeat_get_batt(&batt)) return;

    lv_label_set_text_fmt(Frame_widget.Label_Batt, "%d%%", (int)batt.soc);

    /* 充电中显示闪电图标，其余隐藏 */
    if (batt.status == BAT_STATUS_CHARGING)
    {
        lv_obj_remove_flag(Frame_widget.Img_Charge, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(Frame_widget.Img_Charge, LV_OBJ_FLAG_HIDDEN);
    }
    if(batt.soc >= 100) lv_image_set_src(Frame_widget.Img_battery, BATTERY_100_ICON);
    else if(batt.soc >= 90) lv_image_set_src(Frame_widget.Img_battery, BATTERY_90_ICON);
    else if(batt.soc >= 80) lv_image_set_src(Frame_widget.Img_battery, BATTERY_80_ICON);
    else if(batt.soc >= 70) lv_image_set_src(Frame_widget.Img_battery, BATTERY_70_ICON);
    else if(batt.soc >= 60) lv_image_set_src(Frame_widget.Img_battery, BATTERY_60_ICON);
    else if(batt.soc >= 50) lv_image_set_src(Frame_widget.Img_battery, BATTERY_50_ICON);
    else if(batt.soc >= 40) lv_image_set_src(Frame_widget.Img_battery, BATTERY_40_ICON);
    else if(batt.soc >= 30) lv_image_set_src(Frame_widget.Img_battery, BATTERY_30_ICON);
    else if(batt.soc >= 20) lv_image_set_src(Frame_widget.Img_battery, BATTERY_20_ICON);
    else if(batt.soc >= 10) lv_image_set_src(Frame_widget.Img_battery, BATTERY_10_ICON);
    else  lv_image_set_src(Frame_widget.Img_battery, BATTERY_10_ICON);
}
