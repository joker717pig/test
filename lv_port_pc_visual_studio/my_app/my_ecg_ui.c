#include "my_ecg_ui.h"
#include "lvgl/lvgl.h"

lv_obj_t* chart;
lv_obj_t* obj_ecg;
lv_obj_t* label_heart_display;
lv_obj_t* label_mpa1_display;
lv_obj_t* label_mpa2_display;

int32_t ecg_data[120] = { 7, 14, 21, 27, 34, 40, 47, 54, 60, 66, 73, 79,
86,92,98,104,110,116,122,128,134,139,145,150,156,161,166,171,176,181, 185,190,194,
199,203,207,211,214,218,221,225,228,231,233,236,239,241,243,245,247,248,250,251,252,
253,254,255,255,255,255,255,255,255,254,253,252,251,250,248,247,245,243,241,239,236,
233,231,228,225,221, 218,214,211,207,203,199,194,190,185,181,176, 171,166,161,156,150,
145,139,134,128,122,116,110,104,98,92,86, 79,73,66,60,54, 47,40,34,27,21,14,7,1 };
static const lv_coord_t ecg_sample[] = {
-2, 2, 0, -15, -39, -63, -71, -68, -67, -69, -84, -95, -104, -107, -108, -107, -107, -107, -107, -114, -118, -117,
-112, -100, -89, -83, -71, -64, -58, -58, -62, -62, -58, -51, -46, -39, -27, -10, 4, 7, 1, -3, 0, 14, 24, 30, 25, 19,
13, 7, 12, 15, 18, 21, 13, 6, 9, 8, 17, 19, 13, 11, 11, 11, 23, 30, 37, 34,25, 14, 15, 19, 28, 31, 26, 23, 25, 31,
39, 37, 37, 34, 30, 32, 22, 29, 31, 33, 37, 23, 13, 7, 2, 4, -2, 2, 11, 22,33, 19, -1, -27, -55, -67, -72, -71, -63,
-49, -18, 35, 113, 230, 369, 525, 651, 722, 730, 667, 563, 454, 357, 305, 288,274, 255, 212, 173, 143, 117, 82, 39,
-13, -53, -78, -91, -101, -113, -124, -131, -131, -131, -129, -128, -129, -125, -123, -123, -129, -139, -148, -153,
-159, -166, -183, -205, -227, -243, -248, -246, -254, -280, -327, -381, -429,-473, -517, -556, -592, -612, -620,
-620, -614, -604, -591, -574, -540, -497, -441, -389, -358, -336, -313, -284,-222, -167, -114, -70, -47, -28, -4, 12,
38, 52, 58, 56, 56, 57, 68, 77, 86, 86, 80, 69, 67, 70, 82, 85, 89, 90, 89,89, 88, 91, 96, 97, 91, 83, 78, 82, 88, 95,
96, 105, 106, 110, 102, 100, 96, 98, 97, 101, 98, 99, 100, 107, 113, 119, 115,110, 96, 85, 73, 64, 69, 76, 79,
78, 75, 85, 100, 114, 113, 105, 96, 84, 74, 66, 60, 75, 85, 89, 83, 67, 61,67, 73, 79, 74, 63, 57, 56, 58, 61, 55,
48, 45, 46, 55, 62, 55, 49, 43, 50, 59, 63, 57, 40, 31, 23, 25, 27, 31, 35,34, 30, 36, 34, 42, 38, 36, 40, 46, 50,
47, 32, 30, 32, 52, 67, 73, 71, 63, 54, 53, 45, 41, 28, 13, 3, 1, 4, 4, -8, -23, -32, -31, -19, -5, 3, 9, 13, 19,
24, 27, 29, 25, 22, 26, 32, 42, 51, 56, 60, 57, 55, 53, 53, 54, 59, 54, 49,26, -3, -11, -20, -47, -100, -194, -236,
-212, -123, 8, 103, 142, 147, 120, 105, 98, 93, 81, 61, 40, 26, 28, 30, 30,27, 19, 17, 21, 20, 19, 19, 22, 36, 40,
35, 20, 7, 1, 10, 18, 27, 22, 6, -4, -2, 3, 6, -2, -13, -14, -10, -2, 3, 2, -1, -5, -10, -19, -32, -42, -55, -60,
-68, -77, -86, -101, -110, -117, -115, -104, -92, -84, -85, -84, -73, -65, -52, -50, -45, -35, -20, -3, 12, 20, 25,
26, 28, 28, 30, 28, 25, 28, 33, 42, 42, 36, 23, 9, 0, 1, -4, 1, -4, -4, 1, 5,9, 9, -3, -1, -18, -50, -108, -190,
-272, -340, -408, -446, -537, -643, -777, -894, -920, -853, -697, -461, -251,-60, 58, 103, 129, 139, 155, 170, 173,
178, 185, 190, 193, 200, 208, 215, 225, 224, 232, 234, 240, 240, 236, 229,226, 224, 232, 233, 232, 224, 219, 219,
223, 231, 226, 223, 219, 218, 223, 223, 223, 233, 245, 268, 286, 296, 295,283, 271, 263, 252, 243, 226, 210, 197,
186, 171, 152, 133, 117, 114, 110, 107, 96, 80, 63, 48, 40, 38, 34, 28, 15, 2,-7, -11, -14, -18, -29, -37, -44, -50,
-58, -63, -61, -52, -50, -48, -61, -59, -58, -54, -47, -52, -62, -61, -64, -54, -52, -59, -69, -76, -76, -69, -67,
-74, -78, -81, -80, -73, -65, -57, -53, -51, -47, -35, -27, -22, -22, -24, -21, -17, -13, -10, -11, -13, -20, -20,
-12, -2, 7, -1, -12, -16, -13, -2, 2, -4, -5, -2, 9, 19, 19, 14, 11, 13, 19,21, 20, 18, 19, 19, 19, 16, 15, 13, 14,
9, 3, -5, -9, -5, -3, -2, -3, -3, 2, 8, 9, 9, 5, 6, 8, 8, 7, 4, 3, 4, 5, 3, 5,5, 13, 13, 12, 10, 10, 15, 22, 17,
14, 7, 10, 15, 16, 11, 12, 10, 13, 9, -2, -4, -2, 7, 16, 16, 17, 16, 7, -1, -16, -18, -16, -9, -4, -5, -10, -9, -8,
-3, -4, -10, -19, -20, -16, -9, -9, -23, -40, -48, -43, -33, -19, -21, -26, -31, -33, -19, 0, 17, 24, 9, -17, -47,
-63, -67, -59, -52, -51, -50, -49, -42, -26, -21, -15, -20, -23, -22, -19, -12, -8, 5, 18, 27, 32, 26, 25, 26, 22,
23, 17, 14, 17, 21, 25, 2, -45, -121, -196, -226, -200, -118, -9, 73, 126,131, 114, 87, 60, 42, 29, 26, 34, 35, 34,
25, 12, 9, 7, 3, 2, -8, -11, 2, 23, 38, 41, 23, 9, 10, 13, 16, 8, -8, -17, -23, -26, -25, -21, -15, -10, -13, -13,
-19, -22, -29, -40, -48, -48, -54, -55, -66, -82, -85, -90, -92, -98, -114, -119, -124, -129, -132, -146, -146, -138,
-124, -99, -85, -72, -65, -65, -65, -66, -63, -64, -64, -58, -46, -26, -9, 2,2, 4, 0, 1, 4, 3, 10, 11, 10, 2, -4,
0, 10, 18, 20, 6, 2, -9, -7, -3, -3, -2, -7, -12, -5, 5, 24, 36, 31, 25, 6, 3,7, 12, 17, 11, 0, -6, -9, -8, -7, -5,
-6, -2, -2, -6, -2, 2, 14, 24, 22, 15, 8, 4, 6, 7, 12, 16, 25, 20, 7, -16, -41, -60, -67, -65, -54, -35, -11, 30,
84, 175, 302, 455, 603, 707, 743, 714, 625, 519, 414, 337, 300, 281, 263, 239,197, 163, 136, 109, 77, 34, -18, -50,
-66, -74, -79, -92, -107, -117, -127, -129, -135, -139, -141, -155, -159, -167, -171, -169, -174, -175, -178, -191,
-202, -223, -235, -243, -237, -240, -256, -298, -345, -393, -432, -475, -518,-565, -596, -619, -623, -623, -614,
-599, -583, -559, -524, -477, -425, -383, -357, -331, -301, -252, -198, -143,-96, -57, -29, -8, 10, 31, 45, 60, 65,
70, 74, 76, 79, 82, 79, 75, 62,
};
static void ecg_add_data(lv_timer_t* t)
{
    static int32_t y = 0;
    lv_obj_t* chart = lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(chart, NULL);
    if (y >= 800) {
        y = 0;
    }
    lv_chart_set_next_value(chart, ser, ecg_sample[y]);
    y++;
    uint16_t p = lv_chart_get_point_count(chart);
    uint16_t s = lv_chart_get_x_start_point(chart, ser);
    int32_t* a = lv_chart_get_y_array(chart, ser);

    a[(s + 1) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;

    lv_chart_refresh(chart);
}
/**
 * @brief 心电图信号界面
 * @param  
 */
void ecg_chart(void)
{
    //static lv_style_t style;
    //lv_style_init(&style);

    ///*Set a background color and a radius*/
    //lv_style_set_radius(&style, 10);
    //lv_style_set_bg_opa(&style, LV_OPA_COVER);
    //lv_style_set_bg_color(&style, lv_palette_lighten(LV_PALETTE_GREY, 1));

    ///*Add border to the bottom+right*/
    //lv_style_set_border_color(&style, lv_palette_main(LV_PALETTE_BLUE));
    //lv_style_set_border_width(&style, 5);
    //lv_style_set_border_opa(&style, LV_OPA_10);
    //lv_style_set_border_side(&style, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_RIGHT);
    /*ECG折线图*/
    chart = lv_chart_create(obj_ecg);
    /*lv_obj_add_style(chart, &style, 0);*/
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    /*设置小圆点的透明度*/
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(chart, 100, LV_PART_MAIN);
    lv_obj_set_size(chart, ECG_CHART_WIDE, ECG_CHART_HIGE);
    lv_obj_align(chart, LV_ALIGN_LEFT_MID, 0,-30);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, -1100, 1100);
    lv_chart_set_div_line_count(chart, 0, 0);
    
    /* lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, 125);*/
    lv_chart_set_point_count(chart, 700);
    lv_obj_set_style_bg_color(chart, lv_color_hex(ECG_CHART_BG_COLOR), LV_PART_MAIN);
    lv_chart_series_t* ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    ///*Prefill with data*/
    int32_t i;
    for (i = 0; i < 200; i++) {
        lv_chart_set_next_value(chart, ser, ecg_sample[i]);
    }
    lv_timer_create(ecg_add_data, 4, chart);

    /*ECG标签*/
    lv_obj_t* ecg_label = lv_label_create(obj_ecg);           /*创建基础对象*/
    lv_label_set_text_fmt(ecg_label, "ECG");                      /*设置文本内容*/
    lv_obj_set_style_text_font(ecg_label, &lv_font_montserrat_20, LV_STATE_DEFAULT);
    lv_obj_align_to(ecg_label, chart, LV_ALIGN_TOP_LEFT, 0, 0);           /*设置位置*/
    lv_obj_set_style_text_color(ecg_label, lv_color_hex(ECG_LABEL_BG_COLOR), LV_PART_MAIN);
    /*lv_chart_refresh(chart);*/

    ///*隐藏边框和轮廓*/
   /* lv_obj_set_style_outline_opa(chart, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(chart, 0, LV_STATE_DEFAULT);*/

}
/**
 * @brief 心电图的标尺
 * @param
 */
void ecg_scale(void)
{
    /*设置心电图Y轴标尺*/
    lv_obj_t* ecg_scale_y = lv_scale_create(lv_screen_active());
    lv_obj_set_size(ecg_scale_y, 60,200);
    lv_scale_set_mode(ecg_scale_y, LV_SCALE_MODE_VERTICAL_LEFT);
    lv_obj_center(ecg_scale_y);
    /*设置总刻度*/
    lv_scale_set_total_tick_count(ecg_scale_y, 21);
    /*设置绘制主要刻度线的频率*/
    lv_scale_set_major_tick_every(ecg_scale_y, 5);
    /*设置主要刻度及其标签的长度*/
    lv_obj_set_style_length(ecg_scale_y, 10, LV_PART_INDICATOR);
    /*设置每个主刻度之间的次刻度的长度*/
    lv_obj_set_style_length(ecg_scale_y, 5, LV_PART_ITEMS);
    lv_scale_set_range(ecg_scale_y, 0, 500);
    lv_obj_align_to(ecg_scale_y, chart,LV_ALIGN_OUT_LEFT_MID, 5, 0);
    /*static const char* custom_labels[] = { "0 °C", "25 °C", "50 °C", "75 °C", "100 °C", NULL };
    lv_scale_set_text_src(ecg_scale, custom_labels);*/
    /* 设置主要刻度及标签颜色 */
    static lv_style_t indicator_style;
    lv_style_init(&indicator_style);
    lv_style_set_text_font(&indicator_style, LV_FONT_DEFAULT);
    lv_style_set_text_color(&indicator_style, lv_palette_darken(LV_PALETTE_RED, 3));
    lv_style_set_line_color(&indicator_style, lv_palette_darken(LV_PALETTE_RED, 3));
    lv_obj_add_style(ecg_scale_y, &indicator_style, LV_PART_INDICATOR);
    /*设置次要刻度线的颜色*/
    static lv_style_t minor_ticks_style;
    lv_style_init(&minor_ticks_style);
    lv_style_set_line_color(&minor_ticks_style, lv_palette_lighten(LV_PALETTE_RED, 2));
    lv_style_set_width(&minor_ticks_style, 5U);         /*Tick length*/
    lv_style_set_line_width(&minor_ticks_style, 2U);    /*Tick width*/
    lv_obj_add_style(ecg_scale_y, &minor_ticks_style, LV_PART_ITEMS);
    static lv_style_t main_line_style;
    lv_style_init(&main_line_style);
    /* 设置主线颜色 */
    lv_style_set_line_color(&main_line_style, lv_palette_darken(LV_PALETTE_RED, 3));
    lv_style_set_line_width(&main_line_style, 2U); // Tick width
    lv_obj_add_style(ecg_scale_y, &main_line_style, LV_PART_MAIN);

    /*设置心电图X轴标尺*/
    lv_obj_t* ecg_scale_x = lv_scale_create(lv_screen_active());
    lv_obj_set_size(ecg_scale_x, 600, 60);
    lv_scale_set_mode(ecg_scale_x, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
    /*设置总刻度*/
    lv_scale_set_total_tick_count(ecg_scale_x, 21);
    /*设置绘制主要刻度线的频率*/
    lv_scale_set_major_tick_every(ecg_scale_x, 5);
    /*设置主要刻度及其标签的长度*/
    lv_obj_set_style_length(ecg_scale_x, 10, LV_PART_INDICATOR);
    /*设置每个主刻度之间的次刻度的长度*/
    lv_obj_set_style_length(ecg_scale_x, 5, LV_PART_ITEMS);
    lv_scale_set_range(ecg_scale_x, 0, 500);
    lv_obj_align_to(ecg_scale_x, chart, LV_ALIGN_OUT_BOTTOM_MID, 5, 0);
    /*static const char* custom_labels[] = { "0 °C", "25 °C", "50 °C", "75 °C", "100 °C", NULL };
    lv_scale_set_text_src(ecg_scale, custom_labels);*/
    /* 设置主要刻度及标签颜色 */
    static lv_style_t indicator_style_x;
    lv_style_init(&indicator_style_x);
    lv_style_set_text_font(&indicator_style_x, LV_FONT_DEFAULT);
    lv_style_set_text_color(&indicator_style_x, lv_palette_darken(LV_PALETTE_RED, 3));
    lv_style_set_line_color(&indicator_style_x, lv_palette_darken(LV_PALETTE_RED, 3));
    lv_obj_add_style(ecg_scale_x, &indicator_style_x, LV_PART_INDICATOR);
    /*设置次要刻度线的颜色*/
    static lv_style_t minor_ticks_style_x;
    lv_style_init(&minor_ticks_style_x);
    lv_style_set_line_color(&minor_ticks_style_x, lv_palette_lighten(LV_PALETTE_RED, 2));
    lv_style_set_width(&minor_ticks_style_x, 5U);         /*Tick length*/
    lv_style_set_line_width(&minor_ticks_style_x, 2U);    /*Tick width*/
    lv_obj_add_style(ecg_scale_x, &minor_ticks_style_x, LV_PART_ITEMS);
    static lv_style_t main_line_style_x;
    lv_style_init(&main_line_style_x);
    /* 设置主线颜色 */
    lv_style_set_line_color(&main_line_style_x, lv_palette_darken(LV_PALETTE_RED, 3));
    lv_style_set_line_width(&main_line_style_x, 2U); // Tick width
    lv_obj_add_style(ecg_scale_x, &main_line_style_x, LV_PART_MAIN);

}
/**
 * @brief 创建背景
 * @param  
 */
void my_ecg_background(void)
{
    
    obj_ecg = lv_obj_create(lv_screen_active());     /*创建基础对象*/
    lv_obj_set_size(obj_ecg, 1024, 600);                         /*设置大小*/
    lv_obj_align(obj_ecg, LV_ALIGN_CENTER, 0, 0);           /*设置位置*/
    lv_obj_set_style_bg_color(obj_ecg, lv_color_hex(MAIN_BG_COLOR), LV_PART_MAIN);
}
/**
 * @brief 显示数据
 * @param  
 */
void my_ecg_data_display(void)
{

    /*显示心电数据背景*/
    lv_obj_t* obj_heart = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj_heart, ECG_OBJ_HEART_BG_WIDE, ECG_OBJ_HEART_BG_HIGH);
    lv_obj_align_to(obj_heart, chart, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
    lv_obj_set_style_bg_color(obj_heart, lv_color_hex(ECG_OBJ_HEART_BG_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_heart, 100, LV_PART_MAIN);
    ///*隐藏边框和轮廓*/
    lv_obj_set_style_outline_opa(obj_heart, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj_heart, 0, LV_STATE_DEFAULT);
    /*心电数据标签*/
    lv_obj_t *label_heart = lv_label_create(obj_heart);
    lv_label_set_text(label_heart, "bpm");
    lv_obj_set_style_text_color(label_heart, lv_color_hex(ECG_LABEL_HEART_TEXT_COLOR), LV_PART_MAIN);
    /*心电数据BPM*/
    label_heart_display = lv_label_create(obj_heart);
    lv_label_set_text_fmt(label_heart_display, "115");
    lv_obj_set_style_text_font(label_heart_display, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_heart_display, lv_color_hex(ECG_LABEL_HEART_TEXT_COLOR), LV_PART_MAIN);
    lv_obj_align(label_heart_display, LV_ALIGN_CENTER, 0, 0);

    /*正压数据背景*/
    lv_obj_t* obj_mpa1 = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj_mpa1, ECG_OBJ_MPA_BG_WIDE, ECG_OBJ_MAP_BG_HIGH);
    lv_obj_align_to(obj_mpa1, chart, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 0);
    lv_obj_set_style_bg_color(obj_mpa1, lv_color_hex(ECG_OBJ_MPA_BG_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_mpa1, 100, LV_PART_MAIN);
    ///*隐藏边框和轮廓*/
    lv_obj_set_style_outline_opa(obj_mpa1, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj_mpa1, 0, LV_STATE_DEFAULT);
    /*心电数据标签*/
    lv_obj_t* label_mpa = lv_label_create(obj_mpa1);
    lv_label_set_text(label_mpa, "mpa");
    lv_obj_set_style_text_color(label_mpa, lv_color_hex(ECG_LABEL_MPA_TEXT_COLOR), LV_PART_MAIN);
    /*正压数据MPA*/
    label_mpa1_display = lv_label_create(obj_mpa1);
    lv_label_set_text_fmt(label_mpa1_display, "115");
    lv_obj_set_style_text_font(label_mpa1_display, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_mpa1_display, lv_color_hex(ECG_LABEL_MPA_TEXT_COLOR), LV_PART_MAIN);
    lv_obj_align(label_mpa1_display, LV_ALIGN_CENTER, 0, 0);

    /*负压数据背景*/
    lv_obj_t* obj_mpa2= lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj_mpa2, ECG_OBJ_MPA_BG_WIDE, ECG_OBJ_MAP_BG_HIGH);
    lv_obj_align_to(obj_mpa2, obj_mpa1, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 0);
    lv_obj_set_style_bg_color(obj_mpa2, lv_color_hex(ECG_OBJ_MPA_BG_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_mpa2, 100, LV_PART_MAIN);
    ///*隐藏边框和轮廓*/
    lv_obj_set_style_outline_opa(obj_mpa2, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj_mpa2, 0, LV_STATE_DEFAULT);
    /*心电数据标签*/
    lv_obj_t* label_mpa2 = lv_label_create(obj_mpa2);
    lv_label_set_text(label_mpa2, "mpa");
    lv_obj_set_style_text_color(label_mpa2, lv_color_hex(ECG_LABEL_MPA_TEXT_COLOR), LV_PART_MAIN);
    /*负压数据MPA*/
    label_mpa2_display = lv_label_create(obj_mpa2);
    lv_label_set_text_fmt(label_mpa2_display, "115");
    lv_obj_set_style_text_font(label_mpa2_display, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(label_mpa2_display, lv_color_hex(ECG_LABEL_MPA_TEXT_COLOR), LV_PART_MAIN);
    lv_obj_align(label_mpa2_display, LV_ALIGN_CENTER, 0, 0);
}

void my_ecg_ui(void)
{
    my_ecg_background();
    ecg_chart();
    my_ecg_data_display();
    /*ecg_scale();*/
}
