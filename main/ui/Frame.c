/**
  ******************************************************************************
  * @文件名称   Frame.c
  * @文件描述   顶部状态栏实现：logo + 电池 + 时间 + 页面容器
  *            移植自 EDA_EMG_LVGL_PC\Frame\Frame.c（同事 zqt 原装）
  * @适配说明  路径原样（由 ui_port/app_fs 'C' 盘翻译）；仅 #include 修正
  ******************************************************************************
  */

#include "Frame.h"
#include "Frame_enent.h"
#include "basic.h"
/********************************************************************************
*   Frame.c 主要是页面上的总体框架，发生页面切换时 始终不变 的控件管理，
*   其中包含了页面头部，和跳转按钮，按钮的事件在 #include "./Frame/Frame_enent.h"
*   中实现。
*
*
*   @author zqt
*   @time   2025/3/31
********************************************************************************/

static uint16_t page_open(void);
static uint16_t page_close(void);
static uint16_t page_jump(lv_obj_t* page);

static void Init_TopBar(Frame_Widget_t* widget, lv_obj_t* page_cont);

extern lv_my_ui_page_date_t* g_page_list[APP_SUM];


lv_my_ui_page_date_t lv_my_ui_pageFrame_t = {
    .page_cont = NULL,
    .open = page_open,
    .close = page_close,
    .jump = page_jump
};
Frame_Widget_t Frame_widget;

/**
 * @brief 主界面打开界面操作
 * @param  none
 * @return 0
 */
static uint16_t page_open(void)
{
    return 0;
}

/**
 * @brief 当前界面关闭界面操作
 * @param  none
 * @return 0
 */
static uint16_t page_close(void)
{
    return 0;
}

/**
 * @brief 页面跳转操作,跳转到page 页面
 * @param  page 跳转到的页面
 * @return 0
 */
static uint16_t page_jump(lv_obj_t* page)
{
    //lv_obj_add_flag(lv_my_ui_pageMain_t.page_cont, LV_OBJ_FLAG_HIDDEN); // 先隐藏自身
    lv_obj_remove_flag(page, LV_OBJ_FLAG_HIDDEN); // 打开需要显示的页面
    return 0;
}



void Frame_ui(void)
{

    Init_TopBar(&Frame_widget, lv_my_ui_pageFrame_t.page_cont);
}


/**
 * @author zqt
 * @brief 创建顶部状态栏 固定不动
 * @param widget
 * @param page_cont
 */
static void Init_TopBar(Frame_Widget_t* widget, lv_obj_t* page_cont)
{
    static lv_point_precise_t line_1[] = { {0, 0},{0, 100} };
    /*屏幕头顶部分*/
    lv_obj_t* top_obj = Create_Obj(lv_screen_active(), MAIN_HEAD_PART_WIDE, MAIN_HEAD_PART_HIGE, TIME_BG_COLOR, 0);
    lv_obj_align(top_obj, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_radius(top_obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(top_obj, 0, 0);   /* 消除默认 padding，确保子对象 set_pos 坐标精确 */
    g_Ui.top_bar = top_obj;

    /*公司简称log照片*/
    lv_obj_t* NameLog_Img = Creat_Image(top_obj, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/log.bin");
    lv_obj_set_pos(NameLog_Img, 18, 4);
    /*电池照片（y 顶部对齐到顶栏内完整显示；x 左移避免遮挡右侧时间）*/
    lv_obj_t* Battery_Img = Creat_Image(top_obj, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/battery.bin");
    lv_obj_set_pos(Battery_Img, 372, 2);
    lv_obj_set_size(Battery_Img, 27, 22);
    widget->Img_battery = Battery_Img;
    /*电量百分比标签（电池图标左侧，montserrat_14 保证 % 可显示）*/
    widget->Label_Batt = Creat_Label(top_obj, "--%", &lv_font_montserrat_14);
    lv_obj_set_pos(widget->Label_Batt, 335, 5);
    lv_obj_set_style_text_color(widget->Label_Batt, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /*充电状态图标（闪电，电池图标左侧，默认隐藏，充电时显示）*/
    widget->Img_Charge = Creat_Image(top_obj, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/charge.bin");
    lv_obj_set_pos(widget->Img_Charge, 378, 6);
    // lv_obj_set_size(widget->Img_Charge, 12, 12);
    lv_obj_add_flag(widget->Img_Charge, LV_OBJ_FLAG_HIDDEN);

    /*电池 UI 刷新 timer（每秒；数据来自 app_heartbeat 1s 轮询缓存，无串口开销）*/
    lv_timer_create(update_batt_cb, 1000, NULL);

    /*显示时间标签*/
    lv_obj_t* time_label = Creat_Label(top_obj, "09:02", basic_widget.Chinise_Font_Unit);
    lv_obj_align(time_label, LV_ALIGN_RIGHT_MID, -20, 2);
    lv_obj_set_style_text_color(time_label, lv_color_hex(FONT_GRAY_COLOR),LV_PART_MAIN);
    lv_timer_create(update_time_cb, 1000, NULL);
    widget->Label_Time = time_label;

     //创建页面容器（所有页面的父对象）
    lv_obj_t* cont = Create_Obj(lv_screen_active(), 480, 290, FONT_GRAY_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_set_pos(cont, 0, 30);
    lv_obj_set_style_pad_all(cont, 0, 0);
    g_Ui.page_container = cont;


}
