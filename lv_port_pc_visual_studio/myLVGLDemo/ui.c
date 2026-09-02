#include "ui.h"
#include "lvgl.h"
/*========================================================
 * Resources
 *========================================================*/
LV_IMAGE_DECLARE(my_logo);
LV_IMAGE_DECLARE(img_temperature);
LV_IMAGE_DECLARE(img_humidity);
LV_FONT_DECLARE(ui_font_cn_20);
/*========================================================
 * Function declarations
 *========================================================*/
static void btn_event_cb(lv_event_t* e);
static void slider_event_cb(lv_event_t* e);
static void ui_style_init(void);

static void textarea_event_cb(lv_event_t* e);
static void keyboard_event_cb(lv_event_t* e);
static void tabview_event_cb(lv_event_t* e);
/*========================================================
 * Shared styles
 *========================================================*/
static lv_style_t style_card;
static lv_style_t style_section_title;
static lv_style_t style_value;
static lv_style_t style_button;
static lv_style_t style_button_pressed;
/*========================================================
 * UI Init
 *========================================================*/
void ui_init(void)
{
    /*----------------------------------------------------
     * Initialize shared styles
     *----------------------------------------------------*/
    ui_style_init();
    /*----------------------------------------------------
     * Screen
     *----------------------------------------------------*/
    lv_obj_t* screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFFFFE0), LV_PART_MAIN);
    /*====================================================
     * TABVIEW
     *====================================================*/
    lv_obj_t* tabview = lv_tabview_create(screen);
    lv_obj_set_size(tabview, lv_pct(100), lv_pct(100));
    lv_obj_center(tabview);
    lv_tabview_set_tab_bar_position(tabview, LV_DIR_TOP);
    lv_tabview_set_tab_bar_size(tabview, 48);
    /*----------------------------------------------------
     * Dashboard Tab
     *----------------------------------------------------*/
    lv_obj_t* dashboard_tab = lv_tabview_add_tab(tabview, "Dashboard");
    lv_obj_set_style_bg_color(dashboard_tab, lv_color_hex(0xF3D5DD), LV_PART_MAIN);
    lv_obj_set_style_pad_all(dashboard_tab, 16, LV_PART_MAIN);
    lv_obj_set_layout(dashboard_tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(dashboard_tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(dashboard_tab, 16, LV_PART_MAIN);
    /*----------------------------------------------------
     * Control Tab
     *----------------------------------------------------*/
    lv_obj_t* control_tab = lv_tabview_add_tab(tabview, "Control");
    lv_obj_set_style_bg_color(control_tab, lv_color_hex(0xF3D5DD), LV_PART_MAIN);
    lv_obj_set_style_pad_all(control_tab, 16, LV_PART_MAIN);
    lv_obj_set_layout(control_tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(control_tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(control_tab, 16, LV_PART_MAIN);
    /*----------------------------------------------------
     * Settings Tab
     *----------------------------------------------------*/
    lv_obj_t* settings_tab = lv_tabview_add_tab(tabview, "Settings");
    lv_obj_set_style_bg_color(settings_tab, lv_color_hex(0xF3D5DD), LV_PART_MAIN);
    lv_obj_set_style_pad_all(settings_tab, 16, LV_PART_MAIN);
    lv_obj_set_layout(settings_tab, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(settings_tab, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(settings_tab, 16, LV_PART_MAIN);
    /*====================================================
     * TAB BAR STYLE
     *====================================================*/
    lv_obj_t* tab_bar = lv_tabview_get_tab_bar(tabview);
    /* 整个 Tab Bar 背景 */
    lv_obj_set_style_bg_color(tab_bar, lv_color_hex(0xFFF6E5), LV_PART_MAIN);
    lv_obj_set_style_border_width(tab_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(tab_bar, 0, LV_PART_MAIN);
    /*
     * LVGL 9.2.2:
     *
     * tab_bar
     * ├── lv_button  Dashboard
     * ├── lv_button  Control
     * └── lv_button  Settings
     *
     * LV_STATE_CHECKED 在这些 Button 上。
     */
    uint32_t tab_count = lv_obj_get_child_count(tab_bar);
    for (uint32_t i = 0; i < tab_count; i++)
    {
        lv_obj_t* tab_btn = lv_obj_get_child(tab_bar, (int32_t)i);
        /*---------------- Normal Tab ----------------*/
        lv_obj_set_style_bg_opa(tab_btn, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(tab_btn, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(tab_btn, 0, LV_PART_MAIN);
        lv_obj_set_style_text_color(tab_btn, lv_color_hex(0x777077), LV_PART_MAIN);
        lv_obj_set_style_text_font(tab_btn, &lv_font_montserrat_16, LV_PART_MAIN);
        /*---------------- Selected Tab ----------------*/
        lv_obj_set_style_text_color(tab_btn, lv_color_hex(0xE9954E), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_width(tab_btn, 3, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_color(tab_btn, lv_color_hex(0xE9954E), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_border_side(tab_btn, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN | LV_STATE_CHECKED);
    }
    /*
     * 默认打开 Dashboard。
     * 同时确保三个 Tab Button 的 CHECKED 状态正确。
     */
    lv_tabview_set_active(tabview, 0, LV_ANIM_OFF);
    /*====================================================
     * DASHBOARD
     *====================================================*/
     /*----------------------------------------------------
      * Top Row
      *----------------------------------------------------*/
    lv_obj_t* top_row = lv_obj_create(dashboard_tab);
    lv_obj_set_width(top_row, lv_pct(100));
    lv_obj_set_height(top_row, 110);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(top_row, 0, LV_PART_MAIN);
    lv_obj_remove_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(top_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(top_row, 16, LV_PART_MAIN);
    lv_obj_set_flex_align(top_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    /*----------------------------------------------------
     * Temperature Card
     *----------------------------------------------------*/
    lv_obj_t* temperature_card = lv_obj_create(top_row);
    lv_obj_add_style(temperature_card, &style_card, LV_PART_MAIN);
    lv_obj_set_height(temperature_card, 110);
    lv_obj_set_flex_grow(temperature_card, 1);
    lv_obj_remove_flag(temperature_card, LV_OBJ_FLAG_SCROLLABLE);
    /* Temperature Title */
    lv_obj_t* name_label = lv_label_create(temperature_card);
    lv_label_set_text(name_label, "Temperature");
    lv_obj_add_style(name_label, &style_section_title, LV_PART_MAIN);
    lv_obj_align(name_label, LV_ALIGN_TOP_LEFT, 0, 0);
    /* Temperature Icon */
    lv_obj_t* temperature_icon = lv_image_create(temperature_card);
    lv_image_set_src(temperature_icon, &img_temperature);
    lv_obj_align(temperature_icon, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_image_set_scale(temperature_icon, 200);
    /* Temperature Value */
    lv_obj_t* value_label = lv_label_create(temperature_card);
    lv_label_set_text(value_label, "25.6 C");
    lv_obj_add_style(value_label, &style_value, LV_PART_MAIN);
    lv_obj_align(value_label, LV_ALIGN_BOTTOM_MID, 0, -20);
    /* Temperature Status */
    lv_obj_t* status_label = lv_label_create(temperature_card);
    lv_label_set_text(status_label, "Normal");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFA500), LV_PART_MAIN);
    /*----------------------------------------------------
     * Humidity Card
     *----------------------------------------------------*/
    lv_obj_t* humidity_card = lv_obj_create(top_row);
    lv_obj_add_style(humidity_card, &style_card, LV_PART_MAIN);
    lv_obj_set_height(humidity_card, 110);
    lv_obj_set_flex_grow(humidity_card, 1);
    lv_obj_remove_flag(humidity_card, LV_OBJ_FLAG_SCROLLABLE);
    /* Humidity Title */
    lv_obj_t* name_label1 = lv_label_create(humidity_card);
    lv_label_set_text(name_label1, "Humidity");
    lv_obj_add_style(name_label1, &style_section_title, LV_PART_MAIN);
    lv_obj_align(name_label1, LV_ALIGN_TOP_LEFT, 0, 0);
    /* Humidity Icon */
    lv_obj_t* humidity_icon = lv_image_create(humidity_card);
    lv_image_set_src(humidity_icon, &img_humidity);
    lv_obj_align(humidity_icon, LV_ALIGN_TOP_RIGHT, 0, -2);
    lv_image_set_scale(humidity_icon, 200);
    /* Humidity Value */
    lv_obj_t* value_label1 = lv_label_create(humidity_card);
    lv_label_set_text(value_label1, "70%");
    lv_obj_add_style(value_label1, &style_value, LV_PART_MAIN);
    lv_obj_align(value_label1, LV_ALIGN_BOTTOM_MID, 0, -20);
    /* Humidity Status */
    lv_obj_t* status_label1 = lv_label_create(humidity_card);
    lv_label_set_text(status_label1, "Humid");
    lv_obj_align(status_label1, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_set_style_text_color(status_label1, lv_color_hex(0xFFA500), LV_PART_MAIN);
    /*----------------------------------------------------
     * Bottom Row
     *----------------------------------------------------*/
    lv_obj_t* bottom_row = lv_obj_create(dashboard_tab);
    lv_obj_set_width(bottom_row, lv_pct(100));
    lv_obj_set_height(bottom_row, 110);
    lv_obj_set_style_bg_opa(bottom_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(bottom_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(bottom_row, 0, LV_PART_MAIN);
    lv_obj_remove_flag(bottom_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(bottom_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottom_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bottom_row, 16, LV_PART_MAIN);
    lv_obj_set_flex_align(bottom_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    /*----------------------------------------------------
     * START Button
     *----------------------------------------------------*/
    lv_obj_t* start_btn = lv_button_create(bottom_row);
    lv_obj_add_style(start_btn, &style_button, LV_PART_MAIN);
    lv_obj_add_style(start_btn, &style_button_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_set_height(start_btn, 110);
    lv_obj_set_flex_grow(start_btn, 1);
    lv_obj_t* start_label = lv_label_create(start_btn);
    lv_label_set_text(start_label, "START");
    lv_obj_center(start_label);
    lv_obj_set_style_text_color(start_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_add_event_cb(start_btn, btn_event_cb, LV_EVENT_CLICKED, start_label);
    /*----------------------------------------------------
     * Brightness Card
     *----------------------------------------------------*/
    lv_obj_t* brightness_card = lv_obj_create(bottom_row);
    lv_obj_add_style(brightness_card, &style_card, LV_PART_MAIN);
    lv_obj_set_height(brightness_card, 110);
    lv_obj_set_flex_grow(brightness_card, 1);
    lv_obj_remove_flag(brightness_card, LV_OBJ_FLAG_SCROLLABLE);
    /* Brightness Title */
    lv_obj_t* brightness_label = lv_label_create(brightness_card);
    lv_label_set_text(brightness_label, "Brightness");
    lv_obj_add_style(brightness_label, &style_section_title, LV_PART_MAIN);
    lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 0, 0);
    /* Brightness Bar */
    lv_obj_t* brightness_bar = lv_bar_create(brightness_card);
    lv_obj_set_size(brightness_bar, 160, 8);
    lv_obj_align(brightness_bar, LV_ALIGN_TOP_LEFT, 0, 68);
    lv_bar_set_range(brightness_bar, 0, 100);
    lv_bar_set_value(brightness_bar, 80, LV_ANIM_OFF);
    /* Brightness Slider */
    lv_obj_t* brightness_slider = lv_slider_create(brightness_card);
    lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0xDDD8D5), LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0xE9954E), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0xE9954E), LV_PART_KNOB);
    lv_obj_set_size(brightness_slider, 160, 15);
    lv_obj_align(brightness_slider, LV_ALIGN_TOP_LEFT, 0, 42);
    lv_slider_set_range(brightness_slider, 0, 100);
    lv_slider_set_value(brightness_slider, 80, LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness_slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, brightness_bar);
    /* Brightness Value */
    lv_obj_t* brightness_value = lv_label_create(brightness_card);
    lv_label_set_text(brightness_value, "80%");
    lv_obj_align(brightness_value, LV_ALIGN_TOP_RIGHT, 0, 2);
    /*----------------------------------------------------
     * Chart Card
     *----------------------------------------------------*/
    lv_obj_t* chart_card = lv_obj_create(dashboard_tab);
    lv_obj_add_style(chart_card, &style_card, LV_PART_MAIN);
    lv_obj_set_width(chart_card, lv_pct(100));
    lv_obj_set_height(chart_card, 200);
    lv_obj_remove_flag(chart_card, LV_OBJ_FLAG_SCROLLABLE);
    /* Chart Card Title */
    lv_obj_t* chart_title = lv_label_create(chart_card);
    lv_label_set_text(chart_title, "Temperature Trend");
    lv_obj_add_style(chart_title, &style_section_title, LV_PART_MAIN);
    lv_obj_align(chart_title, LV_ALIGN_TOP_LEFT, 0, 0);
    /* Chart */
    lv_obj_t* temperature_chart = lv_chart_create(chart_card);
    lv_obj_set_size(temperature_chart, lv_pct(100), 135);
    lv_obj_align(temperature_chart, LV_ALIGN_BOTTOM_MID, 0, 0);
    /* 折线图 */
    lv_chart_set_type(temperature_chart, LV_CHART_TYPE_LINE);//用线连接点
    lv_chart_set_point_count(temperature_chart, 10);
    lv_chart_set_range(temperature_chart, LV_CHART_AXIS_PRIMARY_Y, 15, 35);//y轴范围
    lv_chart_set_div_line_count(temperature_chart, 5, 5);//网格线
    /* Series */
    lv_chart_series_t* temp_series = lv_chart_add_series(temperature_chart, lv_color_hex(0xE9954E), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_next_value(temperature_chart, temp_series, 23);
    lv_chart_set_next_value(temperature_chart, temp_series, 25);
    lv_chart_set_next_value(temperature_chart, temp_series, 26);
    lv_chart_set_next_value(temperature_chart, temp_series, 24);
    lv_chart_set_next_value(temperature_chart, temp_series, 23);
    lv_chart_set_next_value(temperature_chart, temp_series, 30);
    lv_chart_set_next_value(temperature_chart, temp_series, 28);
    lv_chart_set_next_value(temperature_chart, temp_series, 29);
    lv_chart_set_next_value(temperature_chart, temp_series, 27);
    lv_chart_set_next_value(temperature_chart, temp_series, 25);

    /*====================================================
     * CONTROL PAGE
     *====================================================*/
    lv_obj_t* control_card = lv_obj_create(control_tab);
    lv_obj_add_style(control_card, &style_card, LV_PART_MAIN);
    lv_obj_set_width(control_card, lv_pct(100));
    lv_obj_set_height(control_card, 150);
    lv_obj_remove_flag(control_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(control_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(control_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(control_card, 10, LV_PART_MAIN);
    /* Control Title */
    lv_obj_t* control_title = lv_label_create(control_card);
    lv_label_set_text(control_title, "Device Control");
    lv_obj_add_style(control_title, &style_section_title, LV_PART_MAIN);
    /* Fan Row */
    lv_obj_t* fan_row = lv_obj_create(control_card);
    lv_obj_set_width(fan_row, lv_pct(100));
    lv_obj_set_height(fan_row, 40);
    lv_obj_set_style_bg_opa(fan_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(fan_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(fan_row, 0, LV_PART_MAIN);
    lv_obj_remove_flag(fan_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(fan_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(fan_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(fan_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    /* Fan Label */
    lv_obj_t* fan_label = lv_label_create(fan_row);
    lv_label_set_text(fan_label, "Fan");
    lv_obj_set_style_text_color(fan_label, lv_color_hex(0x333333), LV_PART_MAIN);
    /* Fan Switch */
    lv_obj_t* fan_switch = lv_switch_create(fan_row);
    lv_obj_set_size(fan_switch, 50, 26);
    lv_obj_set_style_bg_color(fan_switch, lv_color_hex(0xE9954E), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(fan_switch, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
    lv_obj_add_state(fan_switch, LV_STATE_CHECKED);
    /* Auto Mode Checkbox */
    lv_obj_t* auto_checkbox = lv_checkbox_create(control_card);
    lv_checkbox_set_text(auto_checkbox, "Auto Mode");
    lv_obj_set_style_text_color(auto_checkbox, lv_color_hex(0x333333), LV_PART_MAIN);
    /*====================================================
     * SETTINGS PAGE
     *====================================================*/
     /*----------------------------------------------------
      * Mode Settings Card
      *----------------------------------------------------*/
    lv_obj_t* mode_card = lv_obj_create(settings_tab);
    lv_obj_add_style(mode_card, &style_card, LV_PART_MAIN);
    lv_obj_set_width(mode_card, lv_pct(100));
    lv_obj_set_height(mode_card, 220);
    lv_obj_remove_flag(mode_card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(mode_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(mode_card, 12, LV_PART_MAIN);
    /* Mode Settings Title */
    lv_obj_t* mode_title = lv_label_create(mode_card);
    lv_label_set_text(mode_title, "Mode Settings");
    lv_obj_add_style(mode_title, &style_section_title, LV_PART_MAIN);
    /* Work Mode Row */
    lv_obj_t* mode_row = lv_obj_create(mode_card);
    lv_obj_set_width(mode_row, lv_pct(100));
    lv_obj_set_height(mode_row, 42);
    lv_obj_set_style_bg_opa(mode_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(mode_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(mode_row, 0, LV_PART_MAIN);
    lv_obj_remove_flag(mode_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_layout(mode_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(mode_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(mode_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    /* Work Mode Label */
    lv_obj_t* mode_label = lv_label_create(mode_row);
    lv_label_set_text(mode_label, "Work Mode");
    lv_obj_set_style_text_color(mode_label, lv_color_hex(0x333333), LV_PART_MAIN);
    /* Dropdown */
    lv_obj_t* mode_dropdown = lv_dropdown_create(mode_row);
    lv_obj_set_width(mode_dropdown, 140);
    lv_dropdown_set_options(mode_dropdown, "Automatic\nManual\nSleep");
    lv_dropdown_set_selected(mode_dropdown, 0);
    lv_obj_set_style_radius(mode_dropdown, 10, LV_PART_MAIN);
    /* Fan Speed */
    lv_obj_t* speed_label = lv_label_create(mode_card);
    lv_label_set_text(speed_label, "Fan Speed");
    lv_obj_set_style_text_color(speed_label, lv_color_hex(0x333333), LV_PART_MAIN);
    /* Roller */
    lv_obj_t* speed_roller = lv_roller_create(mode_card);
    lv_roller_set_options(speed_roller, "Low\nMedium\nHigh", LV_ROLLER_MODE_NORMAL);
    lv_obj_set_width(speed_roller, 160);
    lv_obj_set_style_radius(speed_roller, 10, LV_PART_MAIN);
    lv_roller_set_visible_row_count(speed_roller, 3);
    lv_roller_set_selected(speed_roller, 1, LV_ANIM_OFF);
    /*----------------------------------------------------
     * Device Settings Card
     *----------------------------------------------------*/
    lv_obj_t* input_card = lv_obj_create(settings_tab);
    lv_obj_add_style(input_card, &style_card, LV_PART_MAIN);
    lv_obj_set_width(input_card, lv_pct(100));
    lv_obj_set_height(input_card, 220);
    lv_obj_set_layout(input_card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(input_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(input_card, 12, LV_PART_MAIN);
    /* Device Settings Title */
    lv_obj_t* input_title = lv_label_create(input_card);
    lv_label_set_text(input_title, "Device Settings");
    lv_obj_add_style(input_title, &style_section_title, LV_PART_MAIN);
    /* Device Name */
    lv_obj_t* name_label2 = lv_label_create(input_card);
    lv_label_set_text(name_label2, "Device Name");
    lv_obj_t* name_textarea = lv_textarea_create(input_card);
    lv_obj_set_width(name_textarea, 300);
    lv_obj_set_height(name_textarea, 45);
    lv_textarea_set_text(name_textarea, "My Device");
    lv_textarea_set_max_length(name_textarea, 20);
    /* Password */
    lv_obj_t* password_label = lv_label_create(input_card);
    lv_label_set_text(password_label, "Password");
    lv_obj_t* password_textarea = lv_textarea_create(input_card);
    lv_obj_set_width(password_textarea, 300);
    lv_obj_set_height(password_textarea, 45);
    lv_textarea_set_text(password_textarea, "123456");
    lv_textarea_set_password_mode(password_textarea, true);
    /*====================================================
     * Keyboard
     *====================================================*/
    lv_obj_t* keyboard = lv_keyboard_create(screen);
    lv_obj_set_size(keyboard, lv_pct(100), 120);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);//程序启动时先隐藏
    lv_obj_add_event_cb(name_textarea, textarea_event_cb, LV_EVENT_FOCUSED, keyboard);
    lv_obj_add_event_cb(password_textarea, textarea_event_cb, LV_EVENT_FOCUSED, keyboard);
    lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_CANCEL, NULL);
    lv_obj_add_event_cb(tabview, tabview_event_cb, LV_EVENT_VALUE_CHANGED, keyboard);
}
/*========================================================
 * Shared Style Initialization
 *========================================================*/
static void ui_style_init(void)
{   
    /*----------------------------------------------------
     * Card
     *----------------------------------------------------*/
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, lv_color_hex(0xFFF6E5));
    lv_style_set_border_width(&style_card, 0);
    lv_style_set_radius(&style_card, 16);
    lv_style_set_pad_all(&style_card, 14);
    /*----------------------------------------------------
     * Section Title
     *----------------------------------------------------*/
    lv_style_init(&style_section_title);
    lv_style_set_text_color(&style_section_title, lv_color_hex(0xD9843D));
    lv_style_set_text_font(&style_section_title, &lv_font_montserrat_16);
    /*----------------------------------------------------
     * Important Value
     *----------------------------------------------------*/
    lv_style_init(&style_value);
    lv_style_set_text_color(&style_value, lv_color_hex(0xD982A6));
    lv_style_set_text_font(&style_value, &lv_font_montserrat_28);
    /*----------------------------------------------------
     * Button
     *----------------------------------------------------*/
    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, lv_color_hex(0xE9954E));
    lv_style_set_border_width(&style_button, 0);
    lv_style_set_radius(&style_button, 16);
    /*----------------------------------------------------
     * Button Pressed
     *----------------------------------------------------*/
    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, lv_color_hex(0xCF7432));
}
/*========================================================
 * START Button Event
 *========================================================*/
static void btn_event_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        lv_obj_t* label = lv_event_get_user_data(e);
        lv_label_set_text(label, "STOP");
    }
}
/*========================================================
 * Brightness Slider Event
 *========================================================*/
static void slider_event_cb(lv_event_t* e)
{
    lv_obj_t* bar = lv_event_get_user_data(e);
    lv_obj_t* slider = lv_event_get_target(e);
    int value = lv_slider_get_value(slider);
    lv_bar_set_value(bar, value, LV_ANIM_ON);
}
/*========================================================
 * keyboard Event
 *========================================================*/
static void textarea_event_cb(lv_event_t* e)
{
    lv_obj_t* textarea = lv_event_get_target(e);
    lv_obj_t* keyboard = lv_event_get_user_data(e);
    lv_keyboard_set_textarea(keyboard, textarea);
    lv_obj_remove_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(keyboard);
}

static void keyboard_event_cb(lv_event_t* e)
{
    lv_obj_t* keyboard = lv_event_get_target(e);
    lv_keyboard_set_textarea(keyboard, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}
static void tabview_event_cb(lv_event_t* e)
{
    lv_obj_t* keyboard = lv_event_get_user_data(e);
    lv_keyboard_set_textarea(keyboard, NULL);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}

