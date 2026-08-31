#include "test.h"
#include "lvgl/lvgl.h"

lv_obj_t* obj1;
lv_obj_t* obj2;
void creat_label(void)
{
    /*Change the active screen's background color*/
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x003a57), LV_PART_MAIN);

    /*创建一个白色标签，设置它文本并对其中心*/
    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Hello world");
    lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_align(label,LV_ALIGN_CENTER,0,0);  //参照父对象对齐，再进行偏移
   
}
/*按钮回调函数*/
static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* btn = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED) {
        static uint8_t cnt = 0;
        cnt++;
        lv_obj_t* label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button:%d", cnt);
    }

}
/*创建按钮*/
void create_button(void)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_bg_color(&style, lv_color_hex(0x9E23BA));

    lv_obj_t* btn = lv_button_create(lv_screen_active());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_set_pos(btn, 10, 10);
    /*添加事件*/
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_add_style(btn, &style, LV_STATE_DEFAULT);

    lv_obj_t* label = lv_label_create(btn);
    lv_label_set_text(label, "Button");
    lv_obj_center(label);

    lv_obj_t* obj1 = lv_obj_create(lv_screen_active());
    lv_obj_set_align(obj1, LV_ALIGN_CENTER);
    /*设置边框的颜色*/
    lv_obj_set_style_border_color(obj1, lv_color_hex(0x1ED76D), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj1, 5, LV_STATE_DEFAULT);
    lv_obj_set_style_border_opa(obj1, 20, LV_STATE_DEFAULT);
    /*设置轮廓的颜色*/
    lv_obj_set_style_outline_color(obj1, lv_color_hex(0xC43E1C), LV_STATE_DEFAULT);
    lv_obj_set_style_outline_width(obj1, 5, LV_STATE_DEFAULT);
    lv_obj_set_style_outline_opa(obj1, 200, LV_STATE_DEFAULT);
}

void create_slider(void)
{
    lv_obj_t* slider = lv_slider_create(lv_screen_active());
    lv_obj_set_align(slider, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xC8A36F), LV_STATE_DEFAULT | LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xFD5C00), LV_STATE_DEFAULT | LV_PART_KNOB);

}
/*事件*/
void my_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);    //获取事件类型
    lv_obj_t* target = lv_event_get_target_obj(e);  //获取触发事件的部件
    if (target == obj1) {                           //判断触发事件的部件
        lv_obj_align(obj1, LV_ALIGN_LEFT_MID,0,0);
    }
    else if (target == obj2) {
        lv_obj_align(obj2, LV_ALIGN_RIGHT_MID, 100, 0);
    }
    if (code == LV_EVENT_CLICKED) {
        printf("LV_EVENT_CLICKED\r\n");
    }
    else if (code == LV_EVENT_LONG_PRESSED) {
        printf("LV_EVENT_LONG_PRESSED\r\n");
    }
}

void my_event(void)
{
    obj1 = lv_obj_create(lv_screen_active());
    lv_obj_add_event_cb(obj1, my_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_size(obj1, 400, 200);
    lv_obj_align(obj1, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj1, lv_color_hex(0x7160E8), 0);

    obj2 = lv_obj_create(obj1);
    lv_obj_add_event_cb(obj2, my_event_cb, LV_EVENT_LONG_PRESSED, NULL);
    lv_obj_set_size(obj2, 200,100);
    lv_obj_align(obj2, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(obj2, lv_color_hex(0x4D4D4D), 0);
}

void my_label(void)
{
    //lv_obj_t* label1 = lv_label_create(lv_screen_active());
    //lv_label_set_long_mode(label1, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
    //lv_label_set_text(label1, "Recolor is not supported for v9 now.");
    //lv_obj_set_width(label1, 150);  /*Set smaller width to make the lines wrap*/
    //lv_obj_set_style_text_align(label1, LV_TEXT_ALIGN_CENTER, 0);
    //lv_obj_align(label1, LV_ALIGN_CENTER, 0, -40);

    //lv_obj_t* label2 = lv_label_create(lv_screen_active());
    //lv_label_set_long_mode(label2, LV_LABEL_LONG_SCROLL_CIRCULAR);     /*Circular scroll*/
    //lv_obj_set_width(label2, 150);
    //lv_label_set_text(label2, "It is a circularly scrolling text. ");
    //lv_obj_align(label2, LV_ALIGN_CENTER, 0, 40);

    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "It is a circularly \n scrolling text.");
    lv_obj_set_size(label, 100, 50);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    //设置背景颜色
    lv_obj_set_style_bg_color(label, lv_color_hex(0xFE4476), LV_STATE_DEFAULT);
    //设置背景透明度
    lv_obj_set_style_bg_opa(label, 100, LV_STATE_DEFAULT);
    //设置字体大小
    lv_obj_set_style_text_font(label, &lv_font_montserrat_30, LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(label, lv_color_hex(0x41A5EE), LV_STATE_DEFAULT);

}
lv_obj_t *label;
lv_obj_t *button_up;
lv_obj_t* button_down;
lv_obj_t* button_stop;
uint32_t speed_value = 0;
static void event_btn_cb(lv_event_t *e)
{
    lv_obj_t* target = lv_event_get_target(e);
    if (target == button_up) {
        speed_value += 30;
    }
    else if (target == button_down) {
        speed_value -= 30;
    }
    else if (target == button_stop) {
        speed_value = 0;
    }
    lv_label_set_text_fmt(label, "speed: %d RPM", speed_value);
}
static void my_examples_label(void)
{
    label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "speed: 0 RPM");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -180);
}
static void my_examples_button_up(void)
{
    button_up = lv_button_create(lv_screen_active());
    lv_obj_set_size(button_up, 200, 80);
    lv_obj_align(button_up, LV_ALIGN_CENTER, -250, 0);
    lv_obj_add_event_cb(button_up, event_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *button_up_label = lv_label_create(button_up);
    lv_obj_set_style_text_font(button_up_label, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_label_set_text(button_up_label, "Speed +");
    lv_obj_set_align(button_up_label, LV_ALIGN_CENTER);
}
static void my_examples_button_down(void)
{
    button_down = lv_button_create(lv_screen_active());
    lv_obj_set_size(button_down, 200, 80);
    lv_obj_align(button_down, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(button_down, event_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *button_down_label = lv_label_create(button_down);
    lv_obj_set_style_text_font(button_down_label, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_label_set_text(button_down_label, "Speed -");
    lv_obj_set_align(button_down_label, LV_ALIGN_CENTER);
}
static void my_examples_button_stop(void)
{
    button_stop = lv_button_create(lv_screen_active());
    lv_obj_set_size(button_stop, 200, 80);
    lv_obj_align(button_stop, LV_ALIGN_CENTER, 250, 0);
    /*设置按钮背景颜色（默认）*/
    lv_obj_set_style_bg_color(button_stop, lv_color_hex(0xD35230), LV_STATE_DEFAULT);
    /*设置按钮背景颜色（按下后）*/
    lv_obj_set_style_bg_color(button_stop, lv_color_hex(0xFF4378), LV_STATE_PRESSED);
    lv_obj_add_event_cb(button_stop, event_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* button_stop_label = lv_label_create(button_stop);
    lv_obj_set_style_text_font(button_stop_label, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_label_set_text(button_stop_label, "Stop");
    lv_obj_set_align(button_stop_label, LV_ALIGN_CENTER);
}
void my_examples_button(void)
{
    my_examples_label();
    my_examples_button_up();
    my_examples_button_down();
    my_examples_button_stop();
}
lv_obj_t *switch1;
static void event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        if (lv_obj_has_state(switch1, LV_STATE_CHECKED)) {
            printf("LED ON\n");
        }
        else {
            printf("LED OFF\n");
        }
    }

}
void my_switch(void)
{
    /*创建时默认关闭状态*/
    switch1 = lv_switch_create(lv_screen_active());
    /*设置选中状态时的背景颜色*/
    lv_obj_set_style_bg_color(switch1, lv_color_hex(0xF55762),LV_STATE_CHECKED | LV_PART_INDICATOR);
    lv_obj_add_state(switch1, LV_STATE_CHECKED | LV_STATE_DISABLED);
    lv_obj_clear_state(switch1, LV_STATE_CHECKED | LV_STATE_DISABLED);
    /*当事件的值发生改变时，进入回调函数*/
    lv_obj_add_event(switch1, event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
lv_obj_t *switch_cool;
lv_obj_t *switch_heat;
static void swt_event_cb(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    if (target == switch_cool) {
        if (lv_obj_has_state(switch_cool, LV_STATE_CHECKED)) {
            lv_obj_clear_state(switch_heat, LV_STATE_CHECKED);
        }
    }
    else if (target == switch_heat)
    {
        if (lv_obj_has_state(switch_heat, LV_STATE_CHECKED)) {
            lv_obj_clear_state(switch_cool, LV_STATE_CHECKED);
        }
    }
}
static void my_switch1(void)
{
    /*制冷模式基础对象（矩形背景）*/
    lv_obj_t* obj_cool = lv_obj_create(lv_screen_active());     /*创建基础对象*/
    lv_obj_set_size(obj_cool, 150, 100);                         /*设置大小*/
    lv_obj_align(obj_cool, LV_ALIGN_CENTER, -250, 0);           /*设置位置*/
    /*制冷模式开关标签*/
    lv_obj_t* label_cool = lv_label_create(obj_cool);           /*创建基础对象*/
    lv_label_set_text(label_cool, "cool");                      /*设置文本内容*/
    lv_obj_set_style_text_font(label_cool, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_align(label_cool, LV_ALIGN_CENTER, 0, -20);           /*设置位置*/
    /*制冷模式开关*/
    switch_cool = lv_switch_create(obj_cool);
    lv_obj_set_size(switch_cool,50,25);
    lv_obj_align(switch_cool, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(switch_cool, swt_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
static void my_switch2(void)
{
    /*制热模式基础对象（矩形背景）*/
    lv_obj_t* obj_heat = lv_obj_create(lv_screen_active());     /*创建基础对象*/
    lv_obj_set_size(obj_heat, 150, 100);                         /*设置大小*/
    lv_obj_align(obj_heat, LV_ALIGN_CENTER, 0, 0);           /*设置位置*/
    /*制热模式开关标签*/
    lv_obj_t* label_heat = lv_label_create(obj_heat);           /*创建基础对象*/
    lv_label_set_text(label_heat, "heat");                      /*设置文本内容*/
    lv_obj_set_style_text_font(label_heat, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_align(label_heat, LV_ALIGN_CENTER, 0, -20);           /*设置位置*/
    /*制热模式开关*/
    switch_heat = lv_switch_create(obj_heat);
    lv_obj_set_size(switch_heat, 50, 25);
    lv_obj_align(switch_heat, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(switch_heat, swt_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
static void my_switch3(void)
{
    /*干燥模式基础对象（矩形背景）*/
    lv_obj_t* obj_dry = lv_obj_create(lv_screen_active());     /*创建基础对象*/
    lv_obj_set_size(obj_dry, 150, 100);                         /*设置大小*/
    lv_obj_align(obj_dry, LV_ALIGN_CENTER, 250, 0);           /*设置位置*/
    /*干燥模式开关标签*/
    lv_obj_t* label_dry = lv_label_create(obj_dry);           /*创建基础对象*/
    lv_label_set_text(label_dry, "dry");                      /*设置文本内容*/
    lv_obj_set_style_text_font(label_dry, &lv_font_montserrat_14, LV_STATE_DEFAULT);
    lv_obj_align(label_dry, LV_ALIGN_CENTER, 0, -20);           /*设置位置*/
    /*干燥模式开关*/
    lv_obj_t* switch_dry = lv_switch_create(obj_dry);
    lv_obj_set_size(switch_dry, 50, 25);
    lv_obj_align(switch_dry, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(switch_dry, swt_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_state(switch_dry, LV_STATE_CHECKED | LV_STATE_DISABLED);
}
void my_examples_switch(void)
{
    my_switch1();
    my_switch2();
    my_switch3();
}
lv_obj_t* checkbox1;
lv_obj_t* checkbox2;
lv_obj_t* label_aggregate;
uint32_t aggregate = 0;
static void checkbox_event_cb(lv_event_t *e)
{
    lv_obj_t* target = lv_event_get_target(e);
    if (target == checkbox1) {
        lv_obj_has_state(checkbox1, LV_STATE_CHECKED) ? (aggregate += 19) : (aggregate -= 19);
    }
    else if (target == checkbox2) {
        lv_obj_has_state(checkbox2, LV_STATE_CHECKED) ? (aggregate += 29) : (aggregate -= 29);
    }
    lv_label_set_text_fmt(label_aggregate, "Aggregate : $%d", aggregate);
}
static void my_checkbox_label(void)
{
    /*菜单标题标签*/
    lv_obj_t* label_menu = lv_label_create(lv_screen_active());
    lv_label_set_text(label_menu, "MENU");
    lv_obj_set_style_text_font(label_menu, &lv_font_montserrat_30, LV_STATE_DEFAULT);
    lv_obj_align(label_menu, LV_ALIGN_CENTER, 0, -200);

    /*总价格标签*/
    label_aggregate = lv_label_create(lv_screen_active());
    lv_label_set_text(label_aggregate, "Aggregate : $0");
    lv_obj_set_style_text_font(label_aggregate, &lv_font_montserrat_30, LV_STATE_DEFAULT);
    lv_obj_align(label_aggregate, LV_ALIGN_CENTER, 0, 200);
}
static void my_checkbox(void)
{
    /*创建基础对象作为背景*/
    lv_obj_t* obj = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj, 500, 300);
    lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);
    /*菜品1复选框*/
    checkbox1 = lv_checkbox_create(obj);
    lv_checkbox_set_text(checkbox1, "Roast chicken   $19");
    lv_obj_align(checkbox1, LV_ALIGN_LEFT_MID, 0, -300/3);
    lv_obj_add_event(checkbox1, checkbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    /*菜品2复选框*/
    checkbox2 = lv_checkbox_create(obj);
    lv_checkbox_set_text(checkbox2, "Roast duck     $29");
    lv_obj_align_to(checkbox2,checkbox1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 300/6);
    lv_obj_add_event(checkbox2, checkbox_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    /*菜品3复选框*/
    lv_obj_t* checkbox3 = lv_checkbox_create(obj);
    lv_checkbox_set_text(checkbox3, "Roast fish     $39");
    lv_obj_align_to(checkbox3, checkbox2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 300 / 6);
    lv_obj_add_state(checkbox3, LV_STATE_DISABLED);
    /*菜品4复选框*/
    lv_obj_t* checkbox4 = lv_checkbox_create(obj);
    lv_checkbox_set_text(checkbox4, "Roast lamb     $49");
    lv_obj_align_to(checkbox4, checkbox3, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 300 / 6);
    lv_obj_add_state(checkbox4, LV_STATE_DISABLED);
}
void my_examples_checkbox(void)
{
    my_checkbox_label();
    my_checkbox();
}
uint8_t val = 0;
lv_obj_t * bar;
lv_obj_t * label_per;
static void bar_timer_cb(lv_timer_t *timer)
{
    if (val < 100)
    {
        val++;
        lv_bar_set_value(bar, val, LV_ANIM_ON);
        lv_label_set_text_fmt(label_per, "%d %%", lv_bar_get_value(bar));
    }
    else {
        lv_label_set_text(label_per, "FINSHED!");
    }
}
static void my_bar_label(void)
{
    /*加载标题标签*/
    lv_obj_t* label_load = lv_label_create(lv_screen_active());
    lv_label_set_text(label_load, "LOADING..");
    lv_obj_set_style_text_font(label_load, &lv_font_montserrat_30, LV_STATE_DEFAULT);
    lv_obj_align(label_load, LV_ALIGN_CENTER, 0, -100);
    /*百分比标签*/
    label_per = lv_label_create(lv_screen_active());
    lv_label_set_text(label_per, "0%");
    lv_obj_set_style_text_font(label_per, &lv_font_montserrat_30, LV_STATE_DEFAULT);
    lv_obj_align(label_per, LV_ALIGN_CENTER, 0, 40);
}
static void my_bar(void)
{
    bar = lv_bar_create(lv_screen_active());                    /*创建进度条*/
    lv_obj_set_size(bar, 400, 20);                              /*设置大小*/
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 0);                   /*设置位置*/
    lv_obj_set_style_anim_time(bar, 1000, LV_STATE_DEFAULT);    /*设置动画时间*/
    lv_timer_create(bar_timer_cb, 100, NULL);                   /*初始化定时器*/
}
void my_examples_bar(void)
{
    my_bar();
    my_bar_label();
}
lv_obj_t* list;
lv_obj_t* label;
static void list_event_cb(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    lv_label_set_text_fmt(label, "%s", lv_list_get_btn_text(list, target));
}
void my_list(void)
{
    /*创建左侧矩形背景*/
    lv_obj_t* obj_left = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj_left, 400, 400);
    lv_obj_align(obj_left,  LV_ALIGN_CENTER, -100, 0);
    lv_obj_update_layout(obj_left);
    /*创建右侧矩形背景*/
    lv_obj_t* obj_right = lv_obj_create(lv_screen_active());
    lv_obj_set_size(obj_right, 200, 400);
    lv_obj_align(obj_right, LV_ALIGN_CENTER, 200, 0);
    lv_obj_update_layout(obj_right);
    /*显示当前文本内容*/
    label = lv_label_create(obj_right);
    lv_obj_set_width(label, 180);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT,0,0);
    lv_obj_update_layout(label);

    list = lv_list_create(obj_left);
    lv_obj_set_size(list, 350, 350);
    lv_obj_center(list);
    /*添加列表文本*/
    lv_list_add_text(list, "File");
    /*添加列表按钮*/
    lv_obj_t* btn = lv_list_add_btn(list, LV_SYMBOL_FILE, "NEW");
    lv_obj_add_event_cb(btn, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* btn1 = lv_list_add_btn(list, LV_SYMBOL_DIRECTORY, "OPEN");
    lv_obj_add_event_cb(btn1, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* btn2 = lv_list_add_btn(list, LV_SYMBOL_AUDIO, "AUDIO");
    lv_obj_add_event_cb(btn2, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* btn3 = lv_list_add_btn(list, LV_SYMBOL_BARS, "BARS");
    lv_obj_add_event_cb(btn3, list_event_cb, LV_EVENT_CLICKED, NULL);
    /*添加列表文本*/
    lv_list_add_text(list, "Connectivity");
    lv_obj_t* btn4 = lv_list_add_btn(list, LV_SYMBOL_BLUETOOTH, "Bluetooth");
    lv_obj_add_event_cb(btn4, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* btn5 = lv_list_add_btn(list, LV_SYMBOL_CALL, "Call");
    lv_obj_add_event_cb(btn5, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* btn6 = lv_list_add_btn(list, LV_SYMBOL_COPY, "Copy");
    lv_obj_add_event_cb(btn6, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* btn7 = lv_list_add_btn(list, LV_SYMBOL_DIRECTORY, "Directory");
    lv_obj_add_event_cb(btn7, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* btn8 = lv_list_add_btn(list, LV_SYMBOL_DOWN, "Down");
    lv_obj_add_event_cb(btn8, list_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t* btn9 = lv_list_add_btn(list, LV_SYMBOL_EDIT, "Edit");
    lv_obj_add_event_cb(btn9, list_event_cb, LV_EVENT_CLICKED, NULL);
}
void my_examples_list(void)
{
    my_list();
}
/*创建下拉列表部件*/
static void drop_event_cb(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    /*获取索引*/
    printf("%d\n", lv_dropdown_get_selected(target));
    char buf[10];
    /*获取选项文本*/
    lv_dropdown_get_selected_str(target, buf, sizeof(buf));
    printf("%s\n", buf);
}
static void my_dropdown(void)
{
    lv_obj_t* dd = lv_dropdown_create(lv_screen_active());
    /*设置选项*/
    lv_dropdown_set_options(dd, "a\nb\nc\nd");
    /*lv_dropdown_set_options_static(dd, "a\nb\nc\nd");*/
    /*添加选项，索引从0开始*/
    lv_dropdown_add_option(dd, "e", 4);
    /*设置当前所选项*/
    lv_dropdown_set_selected(dd, 1);
    lv_obj_add_event_cb(dd, drop_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    /*设置列表展开方向*/
    lv_dropdown_set_dir(dd, LV_DIR_RIGHT);
    /*设置图标*/
    lv_dropdown_set_symbol(dd, LV_SYMBOL_RIGHT);
}
void my_examples_dropdown(void)
{
    my_dropdown();
}
lv_obj_t* roller;
lv_obj_t* roller1;
lv_obj_t* roller2;
lv_obj_t* roller3;
static void roller_event_cb(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);
    
    if (lv_roller_get_selected(target) == 0) {
        lv_obj_add_state(roller2, LV_STATE_DISABLED);
        lv_obj_add_state(roller3, LV_STATE_DISABLED);
    }
    else {
        lv_obj_clear_state(roller2, LV_STATE_DISABLED);
        lv_obj_clear_state(roller3, LV_STATE_DISABLED);
    }
}
/*创建滚轮部件*/
static void my_roller(void)
{
     roller = lv_roller_create(lv_screen_active());
    /*设置选项间距*/
    /*lv_obj_set_style_text_line_space(roller, 30, LV_STATE_DEFAULT);*/
    /*设置选项内容、模式*/
    lv_roller_set_options(roller, "a\nb\nc\nd\ne", LV_ROLLER_MODE_NORMAL);
    /*设置当前所选项*/
    lv_roller_set_selected(roller, 2, LV_ANIM_ON);
    /*设置可见行数*/
    lv_roller_set_visible_row_count(roller, 4);
    lv_obj_add_event_cb(roller, roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
}
/*模式选项*/
static const char *mode_options =   "Auto\n"
                                    "Heat\n"
                                    "Cool\n"
                                    "Fan";
static const char* temp_options =   "23\n"
                                    "24\n"
                                    "25\n"
                                    "26\n"
                                    "27\n"
                                    "28";
static const char* hum_options =    "30\n"
                                    "35\n"
                                    "40\n"
                                    "45\n"
                                    "50\n"
                                    "55";
static void my_mode_roller1(void)
{
    roller1 = lv_roller_create(lv_screen_active());
    lv_roller_set_options(roller1, mode_options, LV_ROLLER_MODE_NORMAL);
    lv_obj_align(roller1, LV_ALIGN_CENTER, -200, 0);
    lv_obj_set_width(roller1, 150);
    lv_roller_set_selected(roller1, 2, LV_ANIM_ON);
    lv_obj_add_event_cb(roller1, roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "MODE");
    lv_obj_align_to(label, roller1, LV_ALIGN_OUT_TOP_MID, 0, -20);

    roller2 = lv_roller_create(lv_screen_active());
    lv_roller_set_options(roller2, temp_options, LV_ROLLER_MODE_NORMAL);
    lv_obj_align(roller2, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_width(roller2, 150);
    lv_roller_set_selected(roller2, 2, LV_ANIM_ON);
    lv_obj_add_event_cb(roller2, roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t* label1 = lv_label_create(lv_screen_active());
    lv_label_set_text(label1, "TEMP");
    lv_obj_align_to(label1, roller2, LV_ALIGN_OUT_TOP_MID, 0, -20);

    roller3 = lv_roller_create(lv_screen_active());
    lv_roller_set_options(roller3, hum_options, LV_ROLLER_MODE_NORMAL);
    lv_obj_align(roller3, LV_ALIGN_CENTER, 200, 0);
    lv_obj_set_width(roller3, 150);
    lv_roller_set_selected(roller3, 2, LV_ANIM_ON);
    lv_obj_add_event_cb(roller3, roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t* label2 = lv_label_create(lv_screen_active());
    lv_label_set_text(label2, "DRY");
    lv_obj_align_to(label2, roller3, LV_ALIGN_OUT_TOP_MID, 0, -20);

}
void my_examples_roller(void)
{
    my_mode_roller1();
}
lv_obj_t* label_slider;
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t * target = lv_event_get_target(e);
    lv_event_code_t  code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_label_set_text_fmt(label_slider, "%d %%", lv_slider_get_value(target));
    }
}
/*创建滑动块部件*/
static void my_slider(void)
{
    /*滑块*/
    lv_obj_t* slider = lv_slider_create(lv_screen_active());
    lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(slider, 300, 20);
    lv_slider_set_value(slider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    /*百分比标签*/
    label_slider = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label_slider, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_label_set_text(label_slider, "50%");
    lv_obj_align_to(label_slider, slider, LV_ALIGN_OUT_RIGHT_MID, 20, 0);
    /*音量图标*/
    lv_obj_t* label_volum = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label_volum, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_label_set_text(label_volum, LV_SYMBOL_VOLUME_MAX);
    lv_obj_align_to(label_volum, slider, LV_ALIGN_OUT_LEFT_MID, -20, 0);
}
void my_examples_slider(void)
{
    my_slider();
}
static arc_event_cb(lv_event_t* e)
{

}
/*创建圆弧部件*/
static void my_arc(void)
{
    /*左侧圆弧*/
    lv_obj_t* arc_left = lv_arc_create(lv_screen_active());
    lv_obj_set_size(arc_left, 200, 200);
    lv_obj_align(arc_left, LV_ALIGN_CENTER, -200, 0);
    lv_arc_set_value(arc_left, 0);
    lv_obj_set_style_arc_width(arc_left, 20, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_left, 20, LV_PART_INDICATOR);
    /*左侧圆弧标签*/
    lv_obj_t* label_left = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label_left, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_label_set_text(label_left, "0%");
    lv_obj_align_to(label_left, arc_left, LV_ALIGN_CENTER, 0, 0);

    /*右侧圆弧*/
    lv_obj_t* arc_right = lv_arc_create(lv_screen_active());
    lv_obj_set_size(arc_right, 200, 200);
    lv_obj_align(arc_right, LV_ALIGN_CENTER, 200, 0);
    lv_arc_set_value(arc_right, 0);
    lv_obj_set_style_arc_width(arc_right, 20, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_right, 20, LV_PART_INDICATOR);
    /*设置背景弧度角*/
    lv_arc_set_bg_angles(arc_right, 0, 360);
    /*设置旋转角度*/
    lv_arc_set_rotation(arc_right, 270);

    /*右侧圆弧标签*/
    lv_obj_t* label_right = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(label_right, &lv_font_montserrat_30, LV_PART_MAIN);
    lv_label_set_text(label_right, "0%");
    lv_obj_align_to(label_right, arc_right, LV_ALIGN_CENTER, 0, 0);
}
void my_examples_arc(void)
{
    my_arc();
}
int32_t xdata[120] = { 7, 14, 21, 27, 34, 40, 47, 54, 60, 66, 73, 79,
86,92,98,104,110,116,122,128,134,139,145,150,156,161,166,171,176,181, 185,190,194,
199,203,207,211,214,218,221,225,228,231,233,236,239,241,243,245,247,248,250,251,252,
253,254,255,255,255,255,255,255,255,254,253,252,251,250,248,247,245,243,241,239,236,
233,231,228,225,221, 218,214,211,207,203,199,194,190,185,181,176, 171,166,161,156,150,
145,139,134,128,122,116,110,104,98,92,86, 79,73,66,60,54, 47,40,34,27,21,14,7,1 };
/*创建一个图表*/
static void add_data(lv_timer_t* t)
{
    static int32_t y = 0;
    lv_obj_t* chart = lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(chart, NULL);
    if (y >= 120) {
        y = 0;
    }
    lv_chart_set_next_value(chart, ser, xdata[y]);
    y++;
    uint16_t p = lv_chart_get_point_count(chart);
    uint16_t s = lv_chart_get_x_start_point(chart, ser);
    int32_t* a = lv_chart_get_y_array(chart, ser);

    a[(s + 1) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;

    lv_chart_refresh(chart);
}
static const data[] = { 25,26,27,28,30,31,32,33,34,35,36,37,38,39 };

void my_examples_char(void)
{ /*Create a stacked_area_chart.obj*/
    lv_obj_t* chart = lv_chart_create(lv_screen_active());
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    /*设置小圆点的透明度*/
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_INDICATOR);
    lv_obj_set_size(chart, 600, 400);
    lv_obj_center(chart);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 260);
   /* lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_X, 0, 125);*/
    lv_chart_set_point_count(chart,500);
    lv_chart_series_t* ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    ///*Prefill with data*/
    int32_t i;
    for (i = 0; i < 120; i++) {
        lv_chart_set_next_value(chart, ser, xdata[i]);
    }
    lv_timer_create(add_data, 300, chart);
    /*lv_chart_refresh(chart);*/
}
static lv_obj_t* chart;
static lv_chart_series_t* ser;
static lv_chart_cursor_t* cursor;

static void value_changed_event_cb(lv_event_t* e)
{
    static int32_t last_id = -1;
    lv_obj_t* obj = lv_event_get_target(e);

    last_id = lv_chart_get_pressed_point(obj);
    if (last_id != LV_CHART_POINT_NONE) {
        lv_chart_set_cursor_point(obj, cursor, NULL, last_id);
    }
}

/**
 * Show cursor on the clicked point
 */
void lv_example_chart(void)
{
    chart = lv_chart_create(lv_screen_active());
    lv_obj_set_size(chart, 200, 150);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, -10);

    //    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 10, 5, 6, 5, true, 40);
    //    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_X, 10, 5, 10, 1, true, 30);

    lv_obj_add_event_cb(chart, value_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_refresh_ext_draw_size(chart);

    cursor = lv_chart_add_cursor(chart, lv_palette_main(LV_PALETTE_BLUE), LV_DIR_LEFT | LV_DIR_BOTTOM);

    ser = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    uint32_t i;
    for (i = 0; i < 10; i++) {
        lv_chart_set_next_value(chart, ser, lv_rand(10, 90));
    }

    //    lv_chart_set_scale_x(chart, 500);

    lv_obj_t* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Click on a point");
    lv_obj_align_to(label, chart, LV_ALIGN_OUT_TOP_MID, 0, -5);
}
