#include "ui.h"
#include "lvgl.h"

static void btn_event_cb(lv_event_t* e);
static void slider_event_cb(lv_event_t* e);


void ui_init(void)
{
    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFE0), LV_PART_MAIN);
    lv_obj_t* page = lv_obj_create(screen);
    lv_obj_set_size(page, lv_pct(100), lv_pct(100));
    lv_obj_center(page);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_radius(page, 0, 0);
    lv_obj_set_style_bg_color(page, lv_color_hex(0xFFB6C1), 0);
    lv_obj_set_style_pad_all(page, 20, 0);
    //lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE); //禁止滚动
    lv_obj_set_scrollbar_mode(page, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_scroll_dir(page, LV_DIR_VER); //上下滚动

    lv_obj_t* content = lv_obj_create(page);
    lv_obj_set_width(content, lv_pct(100));
    lv_obj_set_height(content, LV_SIZE_CONTENT);
    lv_obj_set_layout(content, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(content, 16, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(content, LV_ALIGN_TOP_LEFT, 0, 50);

    lv_obj_t* top_row = lv_obj_create(content);
    lv_obj_set_width(top_row, lv_pct(100));
    lv_obj_set_height(top_row, 110);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(top_row, 0, 0);
    lv_obj_set_style_pad_all(top_row, 0, 0);
    lv_obj_remove_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(top_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(top_row, 16, 0);
    lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);


    lv_obj_t* title = lv_label_create(page);
    lv_label_set_text(title, "Device Dashboard");
   // lv_obj_set_style_text_color(title, lv_color_hex(0xEEA9B8), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28,0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* temperature_card = lv_obj_create(top_row);
    lv_obj_set_height(temperature_card, 110);
    lv_obj_set_flex_grow(temperature_card, 1);
    //lv_obj_align(temperature_card, LV_ALIGN_TOP_LEFT, 0, 50);
    lv_obj_set_style_bg_color(temperature_card, lv_color_hex(0xFFE4B5), 0);
    lv_obj_set_style_border_width(temperature_card, 0, 0);
    lv_obj_set_style_radius(temperature_card, 16, 0);
    lv_obj_remove_flag(temperature_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name_label = lv_label_create(temperature_card);
    lv_label_set_text(name_label, "Temperature");
    lv_obj_set_style_text_color(name_label, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_text_font(name_label, &lv_font_montserrat_16, 0);
    lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* value_label = lv_label_create(temperature_card);
    lv_label_set_text(value_label, "25.6 C");
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(value_label, lv_color_hex(0xFFB5C5), 0);
    lv_obj_align(value_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t* status_label = lv_label_create(temperature_card);
    lv_label_set_text(status_label, "Normal");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFA500), 0);

    /*======================================*/
    lv_obj_t* humidity_card = lv_obj_create(top_row);
    lv_obj_set_height(humidity_card, 110);
    lv_obj_set_flex_grow(humidity_card, 1);
    // lv_obj_align(humidity_card,LV_ALIGN_TOP_RIGHT,0,50);
    lv_obj_set_style_bg_color(humidity_card, lv_color_hex(0xFFE4B5), 0);
    lv_obj_set_style_border_width(humidity_card, 0, 0);
    lv_obj_set_style_radius(humidity_card, 16, 0);
    lv_obj_remove_flag(humidity_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name_label1 = lv_label_create(humidity_card);
    lv_label_set_text(name_label1, "Humidity");
    lv_obj_set_style_text_color(name_label1, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_text_font(name_label1, &lv_font_montserrat_16, 0);
    lv_obj_align(name_label1, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* value_label1 = lv_label_create(humidity_card);
    lv_label_set_text(value_label1, "70%");
    lv_obj_set_style_text_font(value_label1, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(value_label1, lv_color_hex(0xFFB5C5), 0);
    lv_obj_align(value_label1, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t* status_label1 = lv_label_create(humidity_card);
    lv_label_set_text(status_label1, "Humid");
    lv_obj_align(status_label1, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_color(status_label1, lv_color_hex(0xFFA500), 0);

    /*======================================================*/
    lv_obj_t* bottom_row = lv_obj_create(content);
    lv_obj_set_width(bottom_row, lv_pct(100));
    lv_obj_set_height(bottom_row, 110);
    lv_obj_set_style_bg_opa(bottom_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bottom_row, 0, 0);
    lv_obj_set_style_pad_all(bottom_row, 0, 0);
    lv_obj_remove_flag(bottom_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(bottom_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bottom_row, 16, 0);
    lv_obj_set_flex_align(bottom_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    /*=================================================*/
    lv_obj_t* start_btn = lv_button_create(bottom_row);
    lv_obj_set_height(start_btn, 110);
    lv_obj_set_flex_grow(start_btn, 1);
    lv_obj_t* start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, "START");
    lv_obj_center(start_label);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(0xEE9A49), LV_PART_MAIN);
    lv_obj_set_style_radius(start_btn, 12, LV_PART_MAIN);
    lv_obj_set_style_text_color(start_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(start_btn, lv_color_hex(0xEE3B3B), LV_STATE_PRESSED);
    lv_obj_add_event_cb(start_btn, btn_event_cb, LV_EVENT_CLICKED, start_label);


    /*===================================================*/
    lv_obj_t* brightness_card = lv_obj_create(bottom_row);
    lv_obj_set_height(brightness_card, 110);
    lv_obj_set_flex_grow(brightness_card, 1);
    lv_obj_set_style_radius(brightness_card, 16, 0);
    lv_obj_remove_flag(brightness_card, LV_OBJ_FLAG_SCROLLABLE);
    //lv_obj_align(brightness_card, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_set_style_border_width(brightness_card, 0, 0);
    lv_obj_set_style_bg_color(brightness_card, lv_color_hex(0xFFE4B5), 0);

    lv_obj_t* brightness_label = lv_label_create(brightness_card);
    lv_label_set_text(brightness_label, "Brightness");
    lv_obj_set_style_text_color(brightness_label, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_text_font(brightness_label, &lv_font_montserrat_16, 0);
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_pad_all(brightness_card, 12, 0);

    lv_obj_t* brightness_bar = lv_bar_create(brightness_card);
    lv_obj_set_size(brightness_bar, 160, 8);
    lv_obj_align(brightness_bar, LV_ALIGN_TOP_LEFT, 0, 68);
    lv_bar_set_range(brightness_bar, 0, 100);
    lv_bar_set_value(brightness_bar, 80, LV_ANIM_OFF);

    lv_obj_t* brightness_slider = lv_slider_create(brightness_card);
    lv_obj_set_size(brightness_slider, 160, 15);
    lv_obj_align(brightness_slider, LV_ALIGN_TOP_LEFT, 0, 42);
    //lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0xEE9A49), 0);
    lv_slider_set_range(brightness_slider, 0, 100);
    lv_slider_set_value(brightness_slider, 80, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, brightness_bar);

    lv_obj_t* brightness_value = lv_label_create(brightness_card);
    lv_label_set_text(brightness_value, "80%");
    lv_obj_align(brightness_value, LV_ALIGN_TOP_RIGHT, 0, 2);

    /*=========================================================================*/
    lv_obj_t* control_card = lv_obj_create(content);
    lv_obj_set_width(control_card, lv_pct(100));
    lv_obj_set_height(control_card, 150);
    lv_obj_set_style_bg_color(control_card, lv_color_hex(0xFFE4B5), 0);
    lv_obj_set_style_border_width(control_card, 0, 0);
    lv_obj_set_style_radius(control_card, 16, 0);
    lv_obj_set_style_pad_all(control_card, 14, 0);
    lv_obj_remove_flag(control_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(control_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(control_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(control_card, 10, 0);

    lv_obj_t* control_title = lv_label_create(control_card);
    lv_label_set_text(control_title, "Device Control");
    lv_obj_set_style_text_font(control_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(control_title, lv_color_hex(0xFFA500), 0);

    lv_obj_t* fan_row = lv_obj_create(control_card);
    lv_obj_set_width(fan_row, lv_pct(100));
    lv_obj_set_height(fan_row, 40);
    lv_obj_set_style_bg_opa(fan_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(fan_row, 0, 0);
    lv_obj_set_style_pad_all(fan_row, 0, 0);
    lv_obj_remove_flag(fan_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(fan_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(fan_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(fan_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* fan_label = lv_label_create(fan_row);
    lv_label_set_text(fan_label, "Fan");
    lv_obj_set_style_text_color(fan_label, lv_color_hex(0x333333), 0);

    lv_obj_t* fan_swith = lv_switch_create(fan_row);  //加入开关键
    lv_obj_set_size(fan_swith, 50, 26);
    lv_obj_add_state(fan_swith, LV_STATE_CHECKED); //默认开启状态

    lv_obj_t* auto_checkbox = lv_checkbox_create(control_card);
    lv_checkbox_set_text(auto_checkbox, "Auto Mode");
    lv_obj_set_style_text_color(auto_checkbox, lv_color_hex(0x333333), LV_PART_MAIN);

}



/*=========================================================================*/
static void btn_event_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        lv_obj_t* label = lv_event_get_user_data(e);
        lv_label_set_text(label, "STOP");
    }
}

/*=================================================================*/
static void slider_event_cb(lv_event_t* e)
{
    lv_obj_t* bar = lv_event_get_user_data(e);
    lv_obj_t* slider = lv_event_get_target(e);

    int value = lv_slider_get_value(slider);
    lv_bar_set_value(bar, value, LV_ANIM_ON);

}
