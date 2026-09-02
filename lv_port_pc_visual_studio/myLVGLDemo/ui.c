#include "ui.h"
#include "lvgl.h"

LV_IMAGE_DECLARE(my_logo);
LV_IMAGE_DECLARE(img_temperature);
LV_IMAGE_DECLARE(img_humidity);

LV_FONT_DECLARE(ui_font_cn_20);

static void btn_event_cb(lv_event_t* e);
static void slider_event_cb(lv_event_t* e);
static void ui_style_init(void);

static lv_style_t style_card;
static lv_style_t style_section_title;
static lv_style_t style_value;
static lv_style_t style_button;
static lv_style_t style_button_pressed;


void ui_init(void)
{
    ui_style_init();

    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFE0), LV_PART_MAIN);
    lv_obj_t* page = lv_obj_create(screen);
    lv_obj_set_size(page, lv_pct(100), lv_pct(100));
    lv_obj_center(page);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_radius(page, 0, 0);
    lv_obj_set_style_bg_color(page, lv_color_hex(0xF3D5DD), 0);
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
    lv_obj_add_style(temperature_card, &style_card, LV_PART_MAIN);
    lv_obj_set_height(temperature_card, 110);
    lv_obj_set_flex_grow(temperature_card, 1);
    lv_obj_remove_flag(temperature_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name_label = lv_label_create(temperature_card);
    lv_label_set_text(name_label, "Temperature");
    lv_obj_add_style(name_label, &style_section_title, LV_PART_MAIN);
    lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, 0);
    //增加温度图标
    lv_obj_t* temperature_icon = lv_image_create(temperature_card);
    lv_image_set_src(temperature_icon, &img_temperature);
    lv_obj_align(temperature_icon, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_image_set_scale(temperature_icon, 200);

    lv_obj_t* value_label = lv_label_create(temperature_card);
    lv_label_set_text(value_label, "25.6 C");
    lv_obj_add_style(value_label, &style_section_title, LV_PART_MAIN);
    lv_obj_align(value_label, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t* status_label = lv_label_create(temperature_card);
    lv_label_set_text(status_label, "Normal");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFA500), 0);

    /*======================================*/
    lv_obj_t* humidity_card = lv_obj_create(top_row);
    lv_obj_add_style(humidity_card, &style_card, LV_PART_MAIN);
    lv_obj_set_height(humidity_card, 110);
    lv_obj_set_flex_grow(humidity_card, 1);
    lv_obj_remove_flag(humidity_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* name_label1 = lv_label_create(humidity_card);
    lv_label_set_text(name_label1, "Humidity");
    lv_obj_add_style(name_label1, &style_section_title, LV_PART_MAIN);
    lv_obj_align(name_label1, LV_ALIGN_TOP_LEFT, 0, 0);
    //增加湿度图标
    lv_obj_t* humidity_icon = lv_image_create(humidity_card);
    lv_image_set_src(humidity_icon, &img_humidity);
    lv_obj_align(humidity_icon, LV_ALIGN_TOP_RIGHT, 0, -2);
    lv_image_set_scale(humidity_icon, 200);

    lv_obj_t* value_label1 = lv_label_create(humidity_card);
    lv_label_set_text(value_label1, "70%");
    lv_obj_add_style(value_label1, &style_section_title, LV_PART_MAIN);
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
    lv_obj_add_style(start_btn, &style_button, LV_PART_MAIN);
    lv_obj_add_style(start_btn, &style_button_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_height(start_btn, 110);
    lv_obj_set_flex_grow(start_btn, 1);
    lv_obj_t* start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, "START");
    lv_obj_center(start_label);
    lv_obj_set_style_text_color(start_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_event_cb(start_btn, btn_event_cb, LV_EVENT_CLICKED, start_label);


    /*===================================================*/
    lv_obj_t* brightness_card = lv_obj_create(bottom_row);
    lv_obj_add_style(brightness_card, &style_card, LV_PART_MAIN);
    lv_obj_set_height(brightness_card, 110);
    lv_obj_set_flex_grow(brightness_card, 1);
    lv_obj_remove_flag(brightness_card, LV_OBJ_FLAG_SCROLLABLE);
  

    lv_obj_t* brightness_label = lv_label_create(brightness_card);
    lv_label_set_text(brightness_label, "Brightness");
    lv_obj_add_style(brightness_label, &style_section_title, LV_PART_MAIN);
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_pad_all(brightness_card, 12, 0);

    lv_obj_t* brightness_bar = lv_bar_create(brightness_card);
    lv_obj_set_size(brightness_bar, 160, 8);
    lv_obj_align(brightness_bar, LV_ALIGN_TOP_LEFT, 0, 68);
    lv_bar_set_range(brightness_bar, 0, 100);
    lv_bar_set_value(brightness_bar, 80, LV_ANIM_OFF);

    lv_obj_t* brightness_slider = lv_slider_create(brightness_card);
    lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0xDDD8D5), 0);
    lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0xE9954E), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0xE9954E), LV_PART_KNOB);
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
    lv_obj_add_style(control_card, &style_card, LV_PART_MAIN);
    lv_obj_set_width(control_card, lv_pct(100));
    lv_obj_set_height(control_card, 150);
    lv_obj_remove_flag(control_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(control_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(control_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(control_card, 10, 0);

    lv_obj_t* control_title = lv_label_create(control_card);
    lv_label_set_text(control_title, "Device Control");
    lv_obj_add_style(control_title, &style_section_title, LV_PART_MAIN);

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
    //Switch控件/Checkbox控件 布尔状态
    lv_obj_t* fan_swith = lv_switch_create(fan_row);  //加入开关键
    lv_obj_set_size(fan_swith, 50, 26);
    lv_obj_set_style_bg_color(fan_swith, lv_color_hex(0xE9954E), LV_PART_INDICATOR|LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(fan_swith, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_add_state(fan_swith, LV_STATE_CHECKED); //默认开启状态
    lv_obj_t* auto_checkbox = lv_checkbox_create(control_card);
    lv_checkbox_set_text(auto_checkbox, "Auto Mode");
    lv_obj_set_style_text_color(auto_checkbox, lv_color_hex(0x333333), LV_PART_MAIN);

    /*============================================================*/
    lv_obj_t* mode_card = lv_obj_create(content);
    lv_obj_add_style(mode_card, &style_card, LV_PART_MAIN);
    lv_obj_set_width(mode_card, lv_pct(100));
    lv_obj_set_height(mode_card, 220);
    lv_obj_remove_flag(mode_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(mode_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mode_card, 12, 0);

    lv_obj_t* mode_title =lv_label_create(mode_card);
    lv_label_set_text(  mode_title, "Mode Settings" );
    lv_obj_set_style_text_font( mode_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color( mode_title,lv_color_hex(0xFFA500), 0);

    lv_obj_t* mode_row = lv_obj_create(mode_card);
    lv_obj_set_width(mode_row, lv_pct(100));
    lv_obj_set_height(mode_row, 42);
    lv_obj_set_style_bg_opa(mode_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mode_row, 0, 0);
    lv_obj_set_style_pad_all(mode_row, 0, 0);
    lv_obj_remove_flag(mode_row, LV_OBJ_FLAG_SCROLLABLE);
    //横向排列
    lv_obj_set_layout(mode_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    //创建文字
    lv_obj_t* mode_label = lv_label_create(mode_row);
    lv_label_set_text(mode_label, "Work Mode");
    lv_obj_set_style_text_color(mode_label, lv_color_hex(0x333333), 0);
    //创建Dropdown
    lv_obj_t* mode_dropdown = lv_dropdown_create(mode_row);
    lv_obj_set_width(mode_dropdown, 140);
    lv_dropdown_set_options(mode_dropdown, "Automatic\nManual\nSleep");
    lv_dropdown_set_selected(mode_dropdown, 0); //默认选择第一个选项
    lv_obj_set_style_radius(mode_dropdown, 10, LV_PART_MAIN);
    lv_obj_t* speed_label = lv_label_create(mode_card);
    lv_label_set_text(speed_label, "Fan Speed");
    lv_obj_set_style_text_color(speed_label, lv_color_hex(0x333333), 0);
    //创建Roller(更适合在相邻的值做选择)
    lv_obj_t* speed_roller = lv_roller_create(mode_card);
    lv_roller_set_options(speed_roller, "Low\nMedium\nHigh", LV_ROLLER_MODE_NORMAL);//普通滚动（不循环） 
    lv_obj_set_width(speed_roller, 160);
    lv_obj_set_style_radius(speed_roller, 10, LV_PART_MAIN);
    lv_roller_set_visible_row_count(speed_roller, 3);
    lv_roller_set_selected(speed_roller, 1, LV_ANIM_OFF); //默认选择第二个选项
    uint32_t selected = lv_dropdown_get_selected(mode_dropdown);

    lv_obj_t* input_card = lv_obj_create(content);
    lv_obj_add_style(input_card, &style_card, LV_PART_MAIN);
    lv_obj_set_width(input_card, lv_pct(100));
    lv_obj_set_height(input_card, 220);
    //上下排列
    lv_obj_set_layout(input_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(input_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(input_card, 12, 0);//设置行间距
    lv_obj_t* input_title = lv_label_create(input_card);
    lv_label_set_text(input_title, "Device Settings");
    lv_obj_set_style_text_font(input_title, &lv_font_montserrat_16, 0);
    lv_obj_t* name_label2 = lv_label_create(input_card);
    lv_label_set_text(name_label2, "Device Name");
    //创建textarea
    lv_obj_t* name_textarea = lv_textarea_create(input_card);
    lv_obj_set_width(name_textarea, 300);
    lv_obj_set_height(name_textarea, 45);
    lv_textarea_set_text(name_textarea, "My Device");
    lv_textarea_set_max_length(name_textarea, 20);
    lv_obj_t* password_label = lv_label_create(input_card);
    lv_label_set_text(password_label, "Password");
    lv_obj_t* password_textarea = lv_textarea_create(input_card);
    lv_obj_set_width(password_textarea, 300);
    lv_obj_set_height(password_textarea, 45);
    lv_textarea_set_text(password_textarea, "123456");
    lv_textarea_set_password_mode(password_textarea, true);//设置为密码模式

    //添加键盘
    lv_obj_t* keyboard = lv_keyboard_create(lv_screen_active());
    lv_obj_set_size(keyboard, 480, 120);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    //绑定输入框和键盘
    lv_keyboard_set_textarea(keyboard, name_textarea);

}

static void ui_style_init(void)
{
    //卡片样式
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, lv_color_hex(0xFFF6E5));
    lv_style_set_border_width(&style_card, 0);
    lv_style_set_radius(&style_card, 16);
    lv_style_set_pad_all(&style_card, 14);

    //标题样式
    lv_style_init(&style_section_title);
    lv_style_set_text_color(&style_section_title, lv_color_hex(0xD9843D));
    lv_style_set_text_font(&style_section_title, &lv_font_montserrat_16);

    //数值样式
    lv_style_init(&style_value);
    lv_style_set_text_color(&style_value, lv_color_hex(0xD982A6));
    lv_style_set_text_font(&style_value, &lv_font_montserrat_28);

    //按钮样式
    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, lv_color_hex(0xE9954E));
    lv_style_set_border_width(&style_button, 0);
    lv_style_set_radius(&style_button, 16);

    //按钮按下样式
    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, lv_color_hex(0xCF7432));



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
