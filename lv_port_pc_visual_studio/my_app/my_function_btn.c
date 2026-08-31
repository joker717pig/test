#include "my_function_btn.h"
#include "lvgl/lvgl.h"

lv_obj_t* btn_back;
lv_obj_t* btn_record;
lv_obj_t* btn_fetch;
lv_obj_t* btn_real_time;
lv_obj_t* obj_trigeer;
lv_obj_t* obj_cycle;
lv_obj_t* obj_pressure;
/**
 * @brief 按钮事件回调
 * @param  
 */

/**
 * @brief 触发模式按钮
 * @param  
 */
static void my_btn_trigger(void)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(TRIGGER_BTN_BG_COLOR));
    /*心电触发按钮*/
    lv_obj_t* btn_ecg_trigger = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn_ecg_trigger, 120, 60);
    lv_obj_align(btn_ecg_trigger, LV_ALIGN_BOTTOM_LEFT, 40, -40);
    /*添加事件*/
   /* lv_obj_add_event_cb(btn_ecg_trigger, btn_event_cb, LV_EVENT_ALL, NULL);*/
    lv_obj_add_style(btn_ecg_trigger, &style, LV_STATE_DEFAULT);
    /*心电触发按钮标签*/
    lv_obj_t* label_ecg_trigger = lv_label_create(btn_ecg_trigger);
    lv_label_set_text(label_ecg_trigger, "Ecg");
    lv_obj_set_style_text_font(label_ecg_trigger,&lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(label_ecg_trigger);

    /*固有频率触发按钮*/
    lv_obj_t* btn_fixed_frequency = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn_fixed_frequency, 120, 60);
    lv_obj_align_to(btn_fixed_frequency, btn_ecg_trigger, LV_ALIGN_OUT_RIGHT_MID, 40,0);
    /*添加事件*/
    lv_obj_add_event_cb(btn_fixed_frequency, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_style(btn_fixed_frequency, &style, LV_STATE_DEFAULT);
    /*固有频率按钮标签*/
    lv_obj_t* label_fixed_frequency = lv_label_create(btn_fixed_frequency);
    lv_label_set_text(label_fixed_frequency, "Fixation");
    lv_obj_set_style_text_font(label_fixed_frequency, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(label_fixed_frequency);

    /*记录按钮*/
    lv_obj_t* btn_record = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn_record, BTN_RECORD_WIDE, BTN_RECORD_HIGH);
    lv_obj_align(btn_record, LV_ALIGN_TOP_LEFT, 20, 20);
    /*添加事件*/
    lv_obj_add_event_cb(btn_record, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_style(btn_record, &style, LV_STATE_DEFAULT);
    /*记录按钮标签*/
    lv_obj_t* label_record = lv_label_create(btn_record);
    lv_label_set_text(label_record, "Record");
    lv_obj_set_style_text_font(label_record, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(label_record);

    //lv_obj_t* obj1 = lv_obj_create(lv_screen_active());
    //lv_obj_set_align(obj1, LV_ALIGN_CENTER);
    ///*设置边框的颜色*/
    //lv_obj_set_style_border_color(obj1, lv_color_hex(0x1ED76D), LV_STATE_DEFAULT);
    //lv_obj_set_style_border_width(obj1, 5, LV_STATE_DEFAULT);
    //lv_obj_set_style_border_opa(obj1, 20, LV_STATE_DEFAULT);
    ///*设置轮廓的颜色*/
    //lv_obj_set_style_outline_color(obj1, lv_color_hex(0xC43E1C), LV_STATE_DEFAULT);
    //lv_obj_set_style_outline_width(obj1, 5, LV_STATE_DEFAULT);
    //lv_obj_set_style_outline_opa(obj1, 200, LV_STATE_DEFAULT);
}
/**
 * @brief 心电数据记录按钮
 * @param  
 */
static void my_btn_record(void)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(BTN_RECORD_BG_COLOR));
    /*记录按钮*/
    btn_record = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn_record, BTN_RECORD_WIDE, BTN_RECORD_HIGH);
    lv_obj_align(btn_record, LV_ALIGN_TOP_LEFT, 20, 20);
    /*添加事件*/
    lv_obj_add_event_cb(btn_record, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_style(btn_record, &style, LV_STATE_DEFAULT);
    /*记录按钮标签*/
    lv_obj_t* label_record = lv_label_create(btn_record);
    lv_label_set_text(label_record, "Record");
    lv_obj_set_style_text_font(label_record, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(label_record);
}

/**
 * @brief 心电数据追溯按钮
 * @param
 */
static void my_btn_back(void)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(BTN_BACK_BG_COLOR));
    /*追溯按钮*/
    btn_back = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn_back, BTN_BACK_WIDE, BTN_BACK_HIGH);
    lv_obj_align_to(btn_back,btn_record , LV_ALIGN_OUT_RIGHT_MID,20, 0);
    /*添加事件*/
    lv_obj_add_event_cb(btn_back, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_style(btn_back, &style, LV_STATE_DEFAULT);
    /*追溯按钮标签*/
    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Back");
    lv_obj_set_style_text_font(label_back, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(label_back);
}
/**
 * @brief 心电数据调取按钮
 * @param
 */
static void my_btn_fetch(void)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(BTN_FETCH_BG_COLOR));
    /*调取按钮*/
    btn_fetch = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn_fetch, BTN_FETCH_WIDE, BTN_FETCH_HIGH);
    lv_obj_align_to(btn_fetch, btn_back, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    /*添加事件*/
    lv_obj_add_event_cb(btn_fetch, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_style(btn_fetch, &style, LV_STATE_DEFAULT);
    /*调取按钮标签*/
    lv_obj_t* label_fetch = lv_label_create(btn_fetch);
    lv_label_set_text(label_fetch, "Fetch");
    lv_obj_set_style_text_font(label_fetch, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(label_fetch);
}
/**
 * @brief 心电数据实时显示按钮
 * @param
 */
static void my_btn_real_display(void)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(BTN_REAL_BG_COLOR));
    /*实时显示按钮*/
    btn_real_time = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn_real_time, BTN_REAL_WIDE, BTN_REAL_HIGH);
    lv_obj_align_to(btn_real_time, btn_fetch, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    /*添加事件*/
    lv_obj_add_event_cb(btn_real_time, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_style(btn_real_time, &style, LV_STATE_DEFAULT);
    /*实时显示按钮标签*/
    lv_obj_t* label_real_time = lv_label_create(btn_real_time);
    lv_label_set_text(label_real_time, "Real");
    lv_obj_set_style_text_font(label_real_time, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_center(label_real_time);
}

/**
 * @brief 触发模式背景
 * @param
 */
static void my_obj_trigeer(void)
{
    /*触发模式背景*/
    obj_trigeer = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj_trigeer,350, 150);
    lv_obj_align(obj_trigeer, LV_ALIGN_BOTTOM_MID, -320, 0);
    lv_obj_set_style_bg_color(obj_trigeer, lv_color_hex(BTN_REAL_BG_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_trigeer, 100, LV_PART_MAIN);
    ///*隐藏边框和轮廓*/
    lv_obj_set_style_outline_opa(obj_trigeer, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj_trigeer, 0, LV_STATE_DEFAULT);
    /*选择模式标签*/
    lv_obj_t* label_trigeer = lv_label_create(obj_trigeer);
    lv_label_set_text(label_trigeer, "Mode");
    lv_obj_set_style_text_font(label_trigeer, &lv_font_montserrat_14, LV_PART_MAIN);
    
    /*周期调节背景*/
    obj_cycle = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj_cycle, 300, 150);
    lv_obj_align_to(obj_cycle, obj_trigeer, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_obj_set_style_bg_color(obj_cycle, lv_color_hex(BTN_REAL_BG_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_cycle, 100, LV_PART_MAIN);
    ///*隐藏边框和轮廓*/
    lv_obj_set_style_outline_opa(obj_cycle, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj_cycle, 0, LV_STATE_DEFAULT);
    /*选择周期标签*/
    lv_obj_t* label_period = lv_label_create(obj_cycle);
    lv_label_set_text(label_period, "Period");
    lv_obj_set_style_text_font(label_period, &lv_font_montserrat_14, LV_PART_MAIN);

    /*压力调节背景背景*/
    obj_pressure = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj_pressure, 300, 150);
    lv_obj_align_to(obj_pressure, obj_cycle, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    lv_obj_set_style_bg_color(obj_pressure, lv_color_hex(BTN_REAL_BG_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj_pressure, 100, LV_PART_MAIN);
    ///*隐藏边框和轮廓*/
    lv_obj_set_style_outline_opa(obj_pressure, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj_pressure, 0, LV_STATE_DEFAULT);
    /*选择压力标签*/
    lv_obj_t* label_pressure = lv_label_create(obj_pressure);
    lv_label_set_text(label_pressure, "Pressure");
    lv_obj_set_style_text_font(label_pressure, &lv_font_montserrat_14, LV_PART_MAIN);

}
/**
 * @brief 触发模式下拉列表
 * @param  
 */
static void my_trigger_dropdown(void)
{
    lv_obj_t* dd_trigeer = lv_dropdown_create(obj_trigeer);
    /*设置选项*/
    lv_dropdown_set_options(dd_trigeer, "ECG\nFixation");
    lv_obj_set_size(dd_trigeer, 100, 50);
    lv_obj_align(dd_trigeer, LV_ALIGN_LEFT_MID, 0, 0);
    /*设置当前所选项*/
    lv_dropdown_set_selected(dd_trigeer, 0);
    /*lv_obj_add_event_cb(dd_trigeerd, drop_event_cb, LV_EVENT_VALUE_CHANGED, NULL);*/
    /*设置列表展开方向*/
    lv_dropdown_set_dir(dd_trigeer, LV_DIR_TOP);
    /*设置图标*/
    lv_dropdown_set_symbol(dd_trigeer, LV_SYMBOL_UP);

    /*心电触发模式*/
    lv_obj_t* dd_ecg = lv_dropdown_create(obj_trigeer);
    /*设置选项*/
    lv_dropdown_set_options(dd_ecg, "mode1\nmode2");
    lv_obj_set_size(dd_ecg, 100, 50);
    lv_obj_align_to(dd_ecg, dd_trigeer, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    /*设置当前所选项*/
    lv_dropdown_set_selected(dd_ecg, 0);
    /*lv_obj_add_event_cb(dd_trigeerd, drop_event_cb, LV_EVENT_VALUE_CHANGED, NULL);*/
    /*设置列表展开方向*/
    lv_dropdown_set_dir(dd_ecg, LV_DIR_TOP);
    /*设置图标*/
    lv_dropdown_set_symbol(dd_ecg, LV_SYMBOL_UP);

    /*心电触发的可调节模式*/
    lv_obj_t* dd_adjusted = lv_dropdown_create(obj_trigeer);
    /*设置选项*/
    lv_dropdown_set_options(dd_adjusted, "1:1\n2:1");
    lv_obj_set_size(dd_adjusted, 100, 50);
    lv_obj_align_to(dd_adjusted, dd_ecg, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    /*设置当前所选项*/
    lv_dropdown_set_selected(dd_adjusted, 0);
    /*lv_obj_add_event_cb(dd_trigeerd, drop_event_cb, LV_EVENT_VALUE_CHANGED, NULL);*/
    /*设置列表展开方向*/
    lv_dropdown_set_dir(dd_adjusted, LV_DIR_TOP);
    /*设置图标*/
    lv_dropdown_set_symbol(dd_adjusted, LV_SYMBOL_UP);
}
/**
 * @brief 气压调节下拉列表
 * @param
 */
static void my_pressure_dropdown(void)
{
    /*设置正压调节*/
    lv_obj_t* dd_pressure_plus = lv_dropdown_create(obj_pressure);
    /*设置选项*/
    lv_dropdown_set_options(dd_pressure_plus, "-0.8mpa\n-0.7mpa\n-0.6mpa");
    lv_obj_set_size(dd_pressure_plus, 100, 50);
    lv_obj_align(dd_pressure_plus, LV_ALIGN_LEFT_MID, 30, 0);
    /*设置当前所选项*/
    lv_dropdown_set_selected(dd_pressure_plus, 0);
    /*lv_obj_add_event_cb(dd_trigeerd, drop_event_cb, LV_EVENT_VALUE_CHANGED, NULL);*/
    /*设置列表展开方向*/
    lv_dropdown_set_dir(dd_pressure_plus, LV_DIR_TOP);
    /*设置图标*/
    lv_dropdown_set_symbol(dd_pressure_plus, LV_SYMBOL_UP);

    /*心电触发模式*/
    lv_obj_t* dd_dd_pressure_minus = lv_dropdown_create(obj_pressure);
    /*设置选项*/
    lv_dropdown_set_options(dd_dd_pressure_minus, "0.4mpa\n0.5mpa\n0.6mpa\n0.7mpa\n0.8mpa");
    lv_obj_set_size(dd_dd_pressure_minus, 100, 50);
    lv_obj_align_to(dd_dd_pressure_minus, dd_pressure_plus, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    /*设置当前所选项*/
    lv_dropdown_set_selected(dd_dd_pressure_minus, 0);
    /*lv_obj_add_event_cb(dd_trigeerd, drop_event_cb, LV_EVENT_VALUE_CHANGED, NULL);*/
    /*设置列表展开方向*/
    lv_dropdown_set_dir(dd_dd_pressure_minus, LV_DIR_TOP);
    /*设置图标*/
    lv_dropdown_set_symbol(dd_dd_pressure_minus, LV_SYMBOL_UP);
}
/**
 * @brief 周期设置
 * @param  
 */
static void my_btn_period(void)
{
    /*文本显示背景*/
    lv_obj_t *obj_label = lv_obj_create(obj_cycle);
    lv_obj_set_size(obj_label, 80, 50);
    lv_obj_align(obj_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj_label, lv_color_hex(BTN_REAL_BG_COLOR), LV_PART_MAIN);
    /*lv_obj_set_style_bg_opa(obj_trigeer, 100, LV_PART_MAIN);*/

    /*显示周期数据文本*/
    lv_obj_t* label_obj_period = lv_label_create(obj_label);
    lv_label_set_text(label_obj_period, "0.5S");
    lv_obj_set_style_text_font(label_obj_period, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(label_obj_period);

    /*创建按钮+*/
    lv_obj_t* btn_period_add = lv_button_create(obj_cycle);
    lv_obj_set_size(btn_period_add, 100, 50);
    lv_obj_align_to(btn_period_add, obj_label, LV_ALIGN_OUT_LEFT_MID, 0, 0);
    /*添加事件*/
   /* lv_obj_add_event_cb(btn_period_add, btn_event_cb, LV_EVENT_ALL, NULL);*/
    /*创建按钮+标签*/
    lv_obj_t* label_period_add = lv_label_create(btn_period_add);
    lv_label_set_text(label_period_add, "Add");
    lv_obj_set_style_text_font(label_period_add, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(label_period_add);

    /*创建按钮-*/
    lv_obj_t* btn_period_subtract = lv_button_create(obj_cycle);
    lv_obj_set_size(btn_period_subtract, 100, 50);
    lv_obj_align_to(btn_period_subtract, obj_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);
    /*添加事件*/
   /* lv_obj_add_event_cb(btn_period_add, btn_event_cb, LV_EVENT_ALL, NULL);*/
    /*创建按钮加标签*/
    lv_obj_t* label_period_subtract = lv_label_create(btn_period_subtract);
    lv_label_set_text(label_period_subtract, "subtract");
    lv_obj_set_style_text_font(label_period_subtract, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_obj_center(label_period_subtract);
}
/**
 * @brief 按钮界面
 * @param  
 */
void my_btn_ui(void)
{
    my_btn_record();
    my_btn_back();
    my_btn_fetch();
    my_btn_real_display();
    my_obj_trigeer();
    my_trigger_dropdown();
    my_btn_period();
    my_pressure_dropdown();
}
