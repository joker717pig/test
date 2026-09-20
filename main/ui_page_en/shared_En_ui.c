/**
  ******************************************************************************
  * @文件名称   shared_En_ui.c
  * @文件描述   英文共享控件实现（对应中文 menu_ui.c 的 Warn_Window / Treat_Widget /
  *            ParSet_Widget，仅标签翻译为英文）。事件回调全部复用中文语言无关版
  *            （menu_ui_event_cb.h：Back_Btn_Event_cb / Treat_Widget_Event_cb /
  *            Child_Slider_Event_cb），不重复实现。
  *            图标/路径宏复用 menu_ui.h（同事原装 Windows 路径，由 app_fs 翻译）。
  ******************************************************************************
  */

#include "shared_En_ui.h"
#include "menu_ui.h"
#include "menu_ui_event_cb.h"
#include "basic.h"

/**
 * @brief 提示窗（无按钮，纯提示）—— 英文版（对齐中文 Warn_Window(Window_Data_t*)）
 */
void Warn_En_Window(Window_Data_t* data)
{
    lv_obj_t* msgbox = lv_msgbox_create(NULL);
    lv_obj_set_size(msgbox, 280, 70);
    lv_obj_align(msgbox, LV_ALIGN_CENTER, 0, -5);
    lv_obj_set_style_border_width(msgbox, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x5e5f62), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(msgbox, 220, LV_PART_MAIN);
    data->Msgbox = msgbox;

    /*设置消息框内容*/
    lv_obj_t* content = lv_msgbox_get_content(msgbox);
    lv_obj_set_style_pad_top(content, 15, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(content, 12, LV_STATE_DEFAULT);

    lv_obj_t* text = Creat_Label(content, "High abdominal participation,\nrelax abdomen.", &lv_font_montserrat_14);
    lv_obj_set_width(text, 260);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    lv_obj_center(text);
}

/**
 * @brief 返回确认弹窗（Yes/No）—— 英文版（对齐中文 Back_Window(Window_Data_t*, txt)）
 *        [我们] 事件绑定在调用处（PosReh_En_*_ShowConfirm 键盘化时绑 Back_Btn_Event_cb）
 */
void Back_En_Window(Window_Data_t* data, const char* txt)
{
    lv_obj_t* msgbox = lv_msgbox_create(NULL);
    lv_obj_set_size(msgbox, 200, 90);
    lv_obj_set_scrollbar_mode(msgbox, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(msgbox, LV_DIR_NONE);
    lv_obj_center(msgbox);
    lv_obj_set_style_border_width(msgbox, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(msgbox, lv_color_hex(0x5e5f62), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(msgbox, 150, LV_PART_MAIN);
    data->Msgbox = msgbox;

    /*设置消息框内容*/
    lv_obj_t* content = lv_msgbox_get_content(msgbox);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(content, LV_DIR_NONE);
    /* Place the prompt immediately above the Yes/No footer. */
    lv_obj_set_style_pad_top(content, 20, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(content, 10, LV_STATE_DEFAULT);

    lv_obj_t* text = Creat_Label(content, (char*)txt, &lv_font_montserrat_14);
    lv_obj_set_width(text, 180);
    lv_obj_set_style_text_align(text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*设置底部按钮（Yes）*/
    lv_obj_t* btn = lv_msgbox_add_footer_button(msgbox, "Yes");
    lv_obj_set_style_text_color(btn, lv_color_hex(FONT_WHITE_COLOR), 0);
    lv_obj_set_style_text_font(btn, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_opa(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    data->Yes_Btn = btn;

    /*设置底部按钮（No）*/
    lv_obj_t* btn2 = lv_msgbox_add_footer_button(msgbox, "No");
    lv_obj_set_style_text_color(btn2, lv_color_hex(FONT_WHITE_COLOR), 0);
    lv_obj_set_style_text_font(btn2, &lv_font_montserrat_14, 0);
    lv_obj_set_style_bg_opa(btn2, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn2, 0, LV_PART_MAIN);
    data->No_Btn = btn2;
    lv_obj_t* footer = lv_msgbox_get_footer(msgbox);
    lv_obj_set_scrollbar_mode(footer, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(footer, LV_DIR_NONE);
    lv_obj_set_style_pad_bottom(footer, 5, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(footer, 40, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(footer, 40, LV_STATE_DEFAULT);
    lv_obj_set_flex_grow(footer, 1);
}

/**
 * @brief 电刺激治疗界面 —— 英文版（脉宽/频率/强度/暂停/开始/返回/剩余时间）
 */
void Treat_En_Widget(lv_obj_t* page_cont, Treat_Timer_Ctx_t* data)
{
    /*脉宽图标*/
    lv_obj_t* pulse_img = Creat_Image(page_cont, PULSE_ICON);
    lv_obj_set_pos(pulse_img, 15, 20);
    lv_obj_set_style_image_recolor_opa(pulse_img, 255, 0);
    lv_obj_set_style_image_recolor(pulse_img, lv_color_hex(data->treat_data.Color), 0);
    /*脉宽标签*/
    lv_obj_t* Label1 = Creat_Label(page_cont, "Pulse width", &lv_font_montserrat_14);
    lv_obj_set_pos(Label1, 43, 25);
    lv_obj_set_width(Label1, 100);
    lv_obj_set_style_text_color(Label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*脉宽容器*/
    lv_obj_t* Obj = Create_Obj(page_cont, 75, 30, FONT_BLUE_COLOR, 0);
    lv_obj_set_style_radius(Obj, 10, LV_PART_MAIN);
    lv_obj_set_pos(Obj, 30, 60);
    lv_obj_add_style(Obj, &style_gradient2, LV_PART_MAIN);
    /*参数*/
    lv_obj_t* param = Creat_Label(Obj, "", &lv_font_montserrat_14);
    lv_label_set_text_fmt(param, "%duS", (int)data->treat_data.Value_Pulse);
    lv_obj_set_pos(param, 5, -2);
    lv_obj_set_size(param, 50, 20);
    lv_obj_set_style_text_color(param, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Pulse = param;

    /*频率图标*/
    lv_obj_t* pre_img = Creat_Image(page_cont, FREQUENCY_ICON);
    lv_obj_set_pos(pre_img, 15, 110);
    lv_obj_set_style_image_recolor_opa(pre_img, 255, 0);
    lv_obj_set_style_image_recolor(pre_img, lv_color_hex(data->treat_data.Color), 0);
    /*标签*/
    lv_obj_t* Label2 = Creat_Label(page_cont, "Frequency", &lv_font_montserrat_14);
    lv_obj_set_pos(Label2, 43, 115);
    lv_obj_set_width(Label2, 90);
    lv_obj_set_style_text_color(Label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*频率容器*/
    lv_obj_t* Obj1 = Create_Obj(page_cont, 75, 30, FONT_BLUE_COLOR, 0);
    lv_obj_set_style_radius(Obj1, 10, LV_PART_MAIN);
    lv_obj_set_pos(Obj1, 30, 150);
    lv_obj_add_style(Obj1, &style_gradient2, LV_PART_MAIN);
    /*参数*/
    lv_obj_t* param2 = Creat_Label(Obj1, "", &lv_font_montserrat_14);
    lv_label_set_text_fmt(param2, "%dHz", (int)data->treat_data.Value_Freq);
    lv_obj_set_pos(param2, 8, -2);
    lv_obj_set_size(param2, 50, 20);
    lv_obj_set_style_text_color(param2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Freq = param2;

    /*时间显示图标*/
    lv_obj_t* time_img = Creat_Image(page_cont, TIME_ICON);
    lv_obj_set_pos(time_img, 85, -30);
    /*剩余时间显示（初始 Done）*/
    lv_obj_t* time = Creat_Label(page_cont, "", &lv_font_montserrat_28);
    lv_label_set_text_fmt(time, "%02d:%02d:%02d",
                                (int)(data->total_sec / 60000),
                                (int)((data->total_sec % 60000) / 1000),
                                (int)((data->total_sec % 1000) / 10));
    // lv_obj_align(time, LV_ALIGN_CENTER, -10, -30);
    lv_obj_align(time, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(time, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_RemTime = time;
    /*剩余时间标签*/
    lv_obj_t* time_label = Creat_Label(page_cont, "Remaining", &lv_font_montserrat_12);
    lv_obj_set_pos(time_label, 195, 140);
    lv_obj_set_style_text_color(time_label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Hint = time_label;
    /*强度标签*/
    lv_obj_t* Intens_label = Creat_Label(page_cont, "Intensity", &lv_font_montserrat_14);
    lv_obj_set_pos(Intens_label, 365, 25);
    lv_obj_set_style_text_color(Intens_label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*强度容器*/
    lv_obj_t* Obj2 = Create_Obj(page_cont, 95, 135, FONT_BLUE_COLOR, 0);
    lv_obj_set_style_radius(Obj2, 12, LV_PART_MAIN);
    lv_obj_set_pos(Obj2, 345, 50);
    lv_obj_add_style(Obj2, &style_gradient, LV_PART_MAIN);
    lv_obj_add_style(Obj2, &style_shadow, LV_PART_MAIN);

    /*按钮加*/
    lv_obj_t* Btn = Create_Button(Obj2, 75, 30, 0xffffff, 10, 0);
    lv_obj_set_pos(Btn, -3, 3);
    lv_obj_add_style(Btn, &style_gradient2, LV_PART_MAIN);
    lv_obj_add_style(Btn, &style_shadow, LV_PART_MAIN);
    data->treat_data.Plu_Btn = Btn;
    /*按钮加图标*/
    lv_obj_t* plu_img = Creat_Image(Btn, PLUUP_UNSEL_ICON);
    //lv_obj_set_pos(plu_img, 18, 3);
    lv_obj_center(plu_img);
    lv_obj_set_style_image_recolor_opa(plu_img, 255, 0);
    lv_obj_set_style_image_recolor(plu_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Plu_src = plu_img;
    lv_obj_add_event_cb(data->treat_data.Plu_Btn, Treat_Widget_Event_cb, LV_EVENT_ALL, data);

    /*按钮减*/
    lv_obj_t* Btn2 = Create_Button(Obj2, 75, 30, 0xffffff, 10, 0);
    lv_obj_set_pos(Btn2, -3, 85);
    //lv_obj_align(Btn2, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_style(Btn2, &style_gradient2, LV_PART_MAIN);
    lv_obj_add_style(Btn2, &style_shadow, LV_PART_MAIN);
    data->treat_data.Min_Btn = Btn2;
    /*按钮减图标*/
    lv_obj_t* min_img = Creat_Image(Btn2, MINDOWN_UNSEL_ICON);
    //lv_obj_set_pos(min_img, 18, 3);
    lv_obj_center(min_img);
    lv_obj_set_style_image_recolor_opa(min_img, 255, 0);
    lv_obj_set_style_image_recolor(min_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Min_src = min_img;
    lv_obj_add_event_cb(data->treat_data.Min_Btn, Treat_Widget_Event_cb, LV_EVENT_ALL, data);

    /*强度值显示（[我们] B5-A 对齐中文：动态绑定强度初值，去硬编码 "3"）*/
    lv_obj_t* Intens = Creat_Label(Obj2, "", &lv_font_montserrat_20);
    lv_obj_set_pos(Intens, 29, 50);
    lv_obj_set_style_text_color(Intens, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Intens = Intens;
    lv_label_set_text_fmt(Intens, "%d", (int)data->treat_data.Intens_Value);

    /*暂停按钮*/
    lv_obj_t* Btn3 = Create_Button(page_cont, 100, 40, 0xf6f6f6, 20, 0);
    lv_obj_set_pos(Btn3, 55, 220);
    lv_obj_set_style_shadow_opa(Btn3, 0, LV_PART_MAIN);
    data->treat_data.Pause_Btn = Btn3;
    /*暂停按钮图标*/
    lv_obj_t* pause_img = Creat_Image(Btn3, PAUSE_UNSEL_ICON);
    lv_obj_align(pause_img, LV_ALIGN_CENTER, -20, 0);
    lv_obj_set_style_image_recolor_opa(pause_img, 255, 0);
    lv_obj_set_style_image_recolor(pause_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Pause_src = pause_img;
    /*暂停标签*/
    lv_obj_t* pause = Creat_Label(Btn3, "Pause", &lv_font_montserrat_14);
    lv_obj_align(pause, LV_ALIGN_CENTER, 12, 0);
    lv_obj_set_style_text_color(pause, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Pause = pause;
    /*开始按钮*/
    lv_obj_t* Btn4 = Create_Button(page_cont, 100, 40, 0xf6f6f6, 20, 0);
    lv_obj_set_pos(Btn4, 180, 220);
    lv_obj_set_style_shadow_opa(Btn4, 0, LV_PART_MAIN);
    data->treat_data.Start_Btn = Btn4;
    /*开始按钮图标*/
    lv_obj_t* start_img = Creat_Image(Btn4, START_UNSEL_ICON);
    lv_obj_align(start_img, LV_ALIGN_CENTER, -16, 0);
    lv_obj_set_style_image_recolor_opa(start_img, 255, 0);
    lv_obj_set_style_image_recolor(start_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Start_src = start_img;
    /*开始标签*/
    lv_obj_t* start = Creat_Label(Btn4, "Start", &lv_font_montserrat_14);
    lv_obj_align(start, LV_ALIGN_CENTER, 12, 0);
    lv_obj_set_style_text_color(start, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Start = start;
    /*返回按钮*/
    lv_obj_t* Btn5 = Create_Button(page_cont, 100, 40, 0xf6f6f6, 20, 0);
    lv_obj_set_pos(Btn5, 305, 220);
    lv_obj_set_style_shadow_opa(Btn5, 0, LV_PART_MAIN);
    data->treat_data.Back_Btn = Btn5;
    /*返回按钮图标*/
    lv_obj_t* back_img = Creat_Image(Btn5, BACK_UNSEL_ICON);
    lv_obj_align(back_img, LV_ALIGN_CENTER, -16, 0);
    lv_obj_set_style_image_recolor_opa(back_img, 255, 0);
    lv_obj_set_style_image_recolor(back_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->treat_data.Back_src = back_img;
    /*返回标签*/
    lv_obj_t* back = Creat_Label(Btn5, "Back", &lv_font_montserrat_14);
    lv_obj_align(back, LV_ALIGN_CENTER, 12, 0);
    lv_obj_set_style_text_color(back, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_Back = back;
    
    /*电刺激阶段标签*/
    lv_obj_t* Label_stage = Creat_Label(page_cont, "Current Phase:1", &lv_font_montserrat_14);
    lv_obj_align(Label_stage, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_text_color(Label_stage, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->treat_data.Label_stage_stime = Label_stage;

    lv_obj_add_event_cb(data->treat_data.Pause_Btn, Treat_Widget_Event_cb, LV_EVENT_FOCUSED, data);
    lv_obj_add_event_cb(data->treat_data.Start_Btn, Treat_Widget_Event_cb, LV_EVENT_FOCUSED, data);
    lv_obj_add_event_cb(data->treat_data.Back_Btn, Treat_Widget_Event_cb, LV_EVENT_FOCUSED, data);
}

/**
 * @brief 电刺激参数设置界面 —— 英文版（脉宽/设置阶段/频率/设置按钮）
 */
void ParSet_En_Widget(lv_obj_t* page_cont, Param_Data_t* data)
{
    /*脉宽容器*/
    lv_obj_t* Obj = Create_Obj(page_cont, 140, 75, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(Obj, 2, -3);
    /*图标*/
    lv_obj_t* pulse_img = Creat_Image(Obj, PULSE_ICON);
    lv_obj_set_pos(pulse_img, 5, 8);
    lv_obj_set_style_image_recolor_opa(pulse_img, 255, 0);
    lv_obj_set_style_image_recolor(pulse_img, lv_color_hex(data->Slder_Color), 0);
    /*标签*/
    lv_obj_t* Label1 = Creat_Label(Obj, "Pulse width", &lv_font_montserrat_14);
    lv_obj_set_pos(Label1, 30, 14);
    lv_obj_set_width(Label1, 90);
    lv_obj_set_style_text_color(Label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*参数*/
    lv_obj_t* param = Creat_Label(Obj, "", &lv_font_montserrat_14);
    /* [我们] -Werror=format：uint32_t=unsigned long，%d → (int) */
    lv_label_set_text_fmt(param, "%duS", (int)data->Value_Pulse);
    lv_obj_set_width(param, 90);
    lv_obj_set_pos(param, 45, 40);
    lv_obj_set_style_text_color(param, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->Label_Pulse = param;
    /*设置容器*/
    lv_obj_t* Obj2 = Create_Obj(page_cont, 130, 75, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(Obj2, 165, -3);
    /*图标*/
    lv_obj_t* set_img = Creat_Image(Obj2, SET_STAGE_ICON);
    lv_obj_set_pos(set_img, 8, 8);
    lv_obj_set_style_image_recolor_opa(set_img, 255, 0);
    lv_obj_set_style_image_recolor(set_img, lv_color_hex(data->Slder_Color), 0);
    /*标签*/
    lv_obj_t* Label2 = Creat_Label(Obj2, "Phase", &lv_font_montserrat_14);
    lv_obj_set_pos(Label2, 35, 14);
    lv_obj_set_width(Label2, 80);
    lv_obj_set_style_text_color(Label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*参数*/
    //lv_obj_t* param2 = Creat_Label(Obj2, "1/1", &lv_font_montserrat_14);
    lv_obj_t* param2 = NULL;
    if(data->stage_stim == 2){
    param2 = Creat_Label(Obj2, "1/2", &lv_font_montserrat_14);
    }else{
    param2 = Creat_Label(Obj2, "1", &lv_font_montserrat_14);
    }
    lv_obj_set_pos(param2, 45, 40);
    lv_obj_set_size(param2, 50, 20);
    lv_obj_set_style_text_color(param2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->Label_Stage = param2;
    data->Value_Stage = 1;      //默认为第一阶段
    /*频率容器*/
    lv_obj_t* Obj3 = Create_Obj(page_cont, 135, 75, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(Obj3, 300, -3);
    /*图标*/
    lv_obj_t* pre_img = Creat_Image(Obj3, FREQUENCY_ICON);
    lv_obj_set_pos(pre_img, 5, 8);
    lv_obj_set_style_image_recolor_opa(pre_img, 255, 0);
    lv_obj_set_style_image_recolor(pre_img, lv_color_hex(data->Slder_Color), 0);
    /*标签*/
    lv_obj_t* Label3 = Creat_Label(Obj3, "Frequency", &lv_font_montserrat_14);
    lv_obj_set_pos(Label3, 35, 14);
    lv_obj_set_width(Label3, 100);
    lv_obj_set_style_text_color(Label3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*参数*/
    lv_obj_t* param3 = Creat_Label(Obj3, "", &lv_font_montserrat_14);
    /* [我们] -Werror=format：uint32_t=unsigned long，%d → (int) */
    lv_label_set_text_fmt(param3, "%dHz", (int)data->Value_Freq);
    lv_obj_set_pos(param3, 25, 36);
    lv_obj_set_width(param3, 80);
    lv_obj_align(param3, LV_ALIGN_TOP_MID, 35, 40);
    lv_obj_set_style_text_color(param3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->Label_Freq = param3;

    lv_obj_t* Obj4 = Create_Obj(page_cont, 330, 4, 0xebecf0, 1);
    //lv_obj_set_pos(Obj4, 58, 112);
    lv_obj_set_pos(Obj4, 75, 112);
    /*创建滑动条*/
    lv_obj_t* slider = Create_Sliders(page_cont, data->Slder_Color, 90);
    // lv_obj_set_pos(slider, 58, 107);
    lv_obj_set_pos(slider, 75, 107);
    data->Slider = slider;
    lv_obj_add_event_cb(data->Slider, Child_Slider_Event_cb, LV_EVENT_ALL, data);
    /* [我们] 滑条初值同步下移到 Label_Intens 赋值之后（见下），此处只建 slider */
    /*减号按钮*/
    lv_obj_t* Btn1 = Create_Button(page_cont, 85, 40, 0xE6D6DA, 30, 0);
    lv_obj_set_pos(Btn1, 75, 150);
    lv_obj_add_style(Btn1, &style_gradient, LV_PART_MAIN);
    lv_obj_add_style(Btn1, &style_shadow, LV_PART_MAIN);
    lv_obj_t* min_img = Creat_Image(Btn1, MIN_SEL_ICON);
    lv_obj_center(min_img);
    lv_obj_set_style_image_recolor_opa(min_img, 255, 0);
    lv_obj_set_style_image_recolor(min_img, lv_color_hex(FONT_GRAY_COLOR), 0);
    data->Min_Btn = Btn1;
    data->Min_src = min_img;
    lv_obj_add_event_cb(data->Min_Btn, Child_Slider_Event_cb, LV_EVENT_ALL, data);

    /*加号按钮*/
    lv_obj_t* Btn2 = Create_Button(page_cont, 85, 40, 0xE6D6DA, 30, 0);
    lv_obj_set_pos(Btn2, 320, 150);
    lv_obj_add_style(Btn2, &style_gradient, LV_PART_MAIN);
    lv_obj_add_style(Btn2, &style_shadow, LV_PART_MAIN);
    lv_obj_t* plu_img = Creat_Image(Btn2, PLU_SEL_ICON);
    lv_obj_center(plu_img);
    lv_obj_set_style_image_recolor_opa(plu_img, 255, 0);
    lv_obj_set_style_image_recolor(plu_img, lv_color_hex(data->Slder_Color), 0);
    data->Plu_Btn = Btn2;
    data->Plu_src = plu_img;
    lv_obj_add_event_cb(data->Plu_Btn, Child_Slider_Event_cb, LV_EVENT_ALL, data);
    /*强度值标签*/
    lv_obj_t* Label4 = Creat_Label(page_cont, "1", &lv_font_montserrat_20);
    //lv_obj_set_pos(Label4, 215, 165);
    lv_obj_set_size(Label4, 60, 30);
    lv_obj_set_pos(Label4, 210, 160);
    lv_obj_set_style_text_align( Label4,LV_TEXT_ALIGN_CENTER,LV_PART_MAIN);
    lv_obj_set_style_text_color(Label4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    data->Label_Intens = Label4;
    /* [我们] ⚠️ 滑条初值同步必须放在 Label_Intens 赋值之后：
     * LVGL9.5 lv_slider_set_value(ANIM_OFF) 不发 VALUE_CHANGED（lv_bar.c 确认），
     * 需手动同步强度标签；若提前，data->Label_Intens 为 NULL → lv_label_set_text_fmt 崩溃
     * （坑：英文产后页点 10 Sessions → 参数设置页进不去）。 */
    int32_t _init_intens = (data->Value_Stage == 2) ? data->Value_Intens2 : data->Value_Intens1;
    lv_slider_set_value(data->Slider, _init_intens, LV_ANIM_OFF);
    lv_label_set_text_fmt(data->Label_Intens, "%d", (int)_init_intens);
    /*设置按钮*/
    lv_obj_t* Btn3 = Create_Button(page_cont, 202, 40, data->Slder_Color, 30, 0);
   // lv_obj_set_pos(Btn3, 120, 210);
    lv_obj_align(Btn3, LV_ALIGN_TOP_MID, 0, 210);
     lv_obj_t* Labe5 = NULL;
    if(data->stage_stim == 2){
        Labe5 = Creat_Label(Btn3, data->Text_Set1, basic_widget.Chinise_Font_Btn);
    } else {
       Labe5 = Creat_Label(Btn3, data->Text_Set2, basic_widget.Chinise_Font_Btn);
    }
    lv_obj_center(Labe5);
    lv_obj_set_style_text_color(Labe5, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    data->Label_Set = Labe5;
    data->Set_Btn = Btn3;
    /* [我们] B5-A 对齐中文：Param_Data_t 已删 Page_Next，删除 user_data 跳转 */
    lv_obj_add_event_cb(data->Set_Btn, Child_Slider_Event_cb, LV_EVENT_FOCUSED, data);
}

/**
 * @brief 阶段疗程说明界面 —— 英文版（字体/坐标对齐客户定稿 TreIns_Widget）
 *        事件回调复用中文语言无关版（TreIns_Widget_event_cb / ThePage_*_scroll_cb）。
 */
void TreIns_En_Widget(lv_obj_t* page_cont, TreIns_Data_t* data)
{
    /*说明框*/
    lv_obj_t* Obj = Create_Obj(page_cont, data->Obj_Wid, data->Obj_Hig, 0xffffff, 0);
    lv_obj_set_pos(Obj, 25, -4);
    /*添加样式*/
    lv_obj_add_style(Obj, &style_gradient, LV_PART_MAIN);
    lv_obj_add_style(Obj, &style_shadow, LV_PART_MAIN);
    /*标题*/
    lv_obj_t* title1 = Creat_Label(Obj, data->Title, &lv_font_montserrat_16);
    lv_obj_set_pos(title1, 15, 1);
    lv_obj_set_style_text_color(title1, lv_color_hex(data->Title_Color), LV_PART_MAIN);
    lv_obj_t* title12 = Creat_Label(Obj, data->Title_time, &lv_font_montserrat_16);
    lv_obj_set_pos(title12, 82, 1);
    lv_obj_set_style_text_color(title12, lv_color_hex(data->Title_Color), LV_PART_MAIN);
    /*正文*/
    /*lv_obj_t* text = Creat_TextLabel(Obj, 360, 85, data->Text, &lv_font_montserrat_12);
    lv_obj_set_pos(text, 22, 32);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(text, 2, 0);*/
    lv_coord_t text_h = data->Obj_Hig - 40;
    lv_obj_t* text = Creat_TextLabel( Obj, 370,text_h,data->Text, &lv_font_montserrat_12);
    lv_obj_set_pos(text, 25, 22);
    lv_obj_set_style_text_color(text,lv_color_hex(FONT_GRAY_COLOR),LV_PART_MAIN);
    /* 英文内容比较长，行距不要再额外拉开 */
    lv_obj_set_style_text_line_space(text, 0, 0);
    lv_obj_t* dot1 = Creat_Label(Obj, "•", &lv_font_montserrat_12);
    lv_obj_set_pos(dot1, 18, 30);
    lv_obj_set_style_text_color(dot1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /*线材接线图*/
    lv_obj_t* Obj_img = Create_Obj(page_cont, 404, 300, 0xffffff, 0);
    lv_obj_align_to(Obj_img, Obj, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
    lv_obj_set_style_radius(Obj_img, 15, LV_PART_MAIN);
    lv_obj_set_style_border_width(Obj_img, 1, LV_PART_MAIN);
    lv_obj_set_style_border_opa(Obj_img, 255, LV_PART_MAIN);
    lv_obj_set_style_border_color(Obj_img, lv_color_hex(data->Title_Color), LV_PART_MAIN);
    lv_obj_clear_flag(Obj_img, LV_OBJ_FLAG_SCROLLABLE);
    data->Obj_Img = Obj_img;
    /*接线图*/
    lv_obj_t* Img = Creat_Image(Obj_img, data->src);
    lv_obj_set_pos(Img, 0, 0);

    /*底部模糊图层*/
    lv_obj_t* Obj2 = Create_Obj(page_cont, 480, 60, 0x2195f6, 0);
    lv_obj_set_pos(Obj2, -8, 225);
    lv_obj_set_style_border_width(Obj2, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Obj2, 120, LV_PART_MAIN);
    lv_obj_clear_flag(Obj2, LV_OBJ_FLAG_SCROLLABLE);
    ThePage_Obj_origin_y = 225;
    lv_obj_add_event_cb(page_cont, ThePage_Obj_scroll_cb, LV_EVENT_SCROLL, Obj2);

    /*查看历史按钮*/
    lv_obj_t* Btn1 = Create_Button(page_cont, 110, 40, 0xffffff, 30, 0);
    lv_obj_set_pos(Btn1, 188, 200);
    lv_obj_add_style(Btn1, &style_RedShadow, LV_PART_MAIN);
    data->History_Btn = Btn1;
    lv_obj_add_event_cb(Btn1, TreIns_Widget_event_cb, LV_EVENT_FOCUSED, data);

    lv_obj_t* Label = Creat_Label(Btn1, data->Label_Btn1, &lv_font_montserrat_14);
    lv_obj_center(Label);
    lv_obj_set_style_text_color(Label, lv_color_hex(data->Title_Color), LV_PART_MAIN);

    ThePage_HisBtn_origin_y = 200;
    lv_obj_add_event_cb(page_cont, ThePage_HisBtn_scroll_cb, LV_EVENT_SCROLL, Btn1);

    /*开始评估按钮*/
    lv_obj_t* Btn2 = Create_Button(page_cont, 110, 40, data->Title_Color, 30, 0);
    lv_obj_set_pos(Btn2, 316, 200);
    lv_obj_add_style(Btn2, &style_BlueShadow, LV_PART_MAIN);
    data->Start_Btn = Btn2;
    lv_obj_add_event_cb(Btn2, TreIns_Widget_event_cb, LV_EVENT_FOCUSED, data);
    lv_obj_t* Labe2 = Creat_Label(Btn2, data->Label_Btn2, &lv_font_montserrat_14);
    lv_obj_center(Labe2);
    lv_obj_set_style_text_color(Labe2, lv_color_hex(0xffffff), LV_PART_MAIN);

    ThePage_StrBtn_origin_y = 200;
    lv_obj_add_event_cb(page_cont, ThePage_StrBtn_scroll_cb, LV_EVENT_SCROLL, Btn2);
}

/* ============================================================================
 * [我们] B5-B 治疗页新控件英文版（对应中文 menu_ui.c 的
 *   draw_pressure_excel / Creat_Chart / Chart_widget / Select_Widget2 /
 *   Stage_Widget2 / Instr_Widget，文字/字体/坐标对齐客户定稿 EDA_EMG_LVGL_PC_EN）
 * ==========================================================================*/

/**
 * @brief 创建曲线表格（刻度虚线网格）—— 英文版（数字刻度，montserrat_12）
 */
static void draw_pressure_excel_En(lv_obj_t* cont)
{
    static lv_point_precise_t line_1[] = { {0, 0},{400, 0} };
    static lv_point_precise_t line_2[] = { {0, 15},{0, 164} };
    (void)line_2;
    for (uint8_t i = 0; i < 5; i++) {
        lv_obj_t* line1 = creat_DottedLine(cont, 370, 6, line_1, 2);
        lv_obj_set_pos(line1, 45, 249 - (i * 29));
        lv_obj_clear_flag(line1, LV_OBJ_FLAG_SCROLLABLE);
    }
    lv_obj_t* text1 = Creat_Label(cont, "0", &lv_font_montserrat_12);
    lv_obj_set_pos(text1, 25, 238);
    lv_obj_set_style_text_color(text1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text2 = Creat_Label(cont, "25", &lv_font_montserrat_12);
    lv_obj_set_pos(text2, 20, 211);
    lv_obj_set_style_text_color(text2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text3 = Creat_Label(cont, "50", &lv_font_montserrat_12);
    lv_obj_set_pos(text3, 20, 184);
    lv_obj_set_style_text_color(text3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text4 = Creat_Label(cont, "75", &lv_font_montserrat_12);
    lv_obj_set_pos(text4, 20, 157);
    lv_obj_set_style_text_color(text4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text5 = Creat_Label(cont, "100", &lv_font_montserrat_12);
    lv_obj_set_pos(text5, 13, 125);
    lv_obj_set_style_text_color(text5, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
}

/**
 * @brief 创建曲线图（透明背景 lv_chart + 预填充中间值）—— 英文版
 */
static void Creat_En_Chart(Chart_Data_t* widget, lv_obj_t* page_cont)
{
    static lv_style_t style;
    static bool style_inited = false;
    if (!style_inited) {
        style_inited = true;
        lv_style_init(&style);
        lv_style_set_radius(&style, 1);
        lv_style_set_bg_opa(&style, 0);
        lv_style_set_border_width(&style, 0);
    }

    draw_pressure_excel_En(page_cont);

    lv_obj_t* chart = lv_chart_create(page_cont);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_obj_set_pos(chart, 45, 130);
    lv_obj_set_size(chart, 370, 120);
    lv_obj_add_style(chart, &style, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_set_style_radius(chart, 0, 0);
    lv_obj_set_style_pad_bottom(chart, 0, 0);
    lv_chart_set_div_line_count(chart, 0, 0);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
    lv_chart_set_point_count(chart, 100);

    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_rounded(chart, true, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_INDICATOR);

    lv_chart_series_t* ser_green = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_all_value(chart, ser_green, LV_CHART_POINT_NONE);  /* 所有点标记为无效，不画线 */
    // for (uint16_t i = 0; i < 100; i++) {
    //     lv_chart_set_next_value(chart, ser_green, 50);
    // }

    lv_obj_add_flag(chart, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

    widget->Chart = chart;
}

/**
 * @brief 创建曲线图页面（左上盒/右上盒/腹部监控/完成标签 + 曲线表）—— 英文版
 */
void Chart_En_widget(lv_obj_t* page_cont, Chart_Data_t* widget)
{
    /*左上框*/
    lv_obj_t* Obj_left = Create_Obj(page_cont, 260, 90, 0xffffff, 0);
    lv_obj_set_pos(Obj_left, 15, 5);
    lv_obj_add_style(Obj_left, &style_shadow, LV_PART_MAIN);
    lv_obj_add_style(Obj_left, &style_gradient, LV_PART_MAIN);
    lv_obj_t* Img_left = Creat_Image(Obj_left, VAGINA_ICON);
    lv_obj_set_pos(Img_left, 10, 10);
    lv_obj_t* title1 = Creat_Label(Obj_left, "Vagina", &lv_font_montserrat_14);
    lv_obj_set_pos(title1, 35, 15);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Title_Vagina = title1;

    lv_obj_t* title3 = Creat_Label(Obj_left, "Max EMG\nPotential", &lv_font_montserrat_10);
    lv_obj_set_pos(title3, 20, 36);
    lv_obj_set_width(title3, 70);
    lv_obj_set_style_text_color(title3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* label = Creat_Label(Obj_left, "", &lv_font_montserrat_12);
    lv_label_set_text_fmt(label, "%.1fuV", widget->Value_MaxEMG);
    lv_obj_set_pos(label, 20, 60);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_RED_COLOR), LV_PART_MAIN);
    widget->Label_MaxEMG = label;

    lv_obj_t* title4 = Creat_Label(Obj_left, "Instant EMG\nPotential", &lv_font_montserrat_10);
    lv_obj_set_pos(title4, 92, 36);
    lv_obj_set_width(title4, 78);
    lv_obj_set_style_text_color(title4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* label2 = Creat_Label(Obj_left, "", &lv_font_montserrat_12);
    lv_label_set_text_fmt(label2, "%.1fuV", widget->Value_InsEMG);
    lv_obj_set_pos(label2, 95, 60);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_RED_COLOR), LV_PART_MAIN);
    widget->Label_InsEMG = label2;

    lv_obj_t* title5 = Creat_Label(Obj_left, "Time Left", &lv_font_montserrat_10);
    lv_obj_set_pos(title5, 163, 36);
    lv_obj_set_width(title5, 65);
    lv_obj_set_style_text_color(title5, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    lv_obj_t* label3 = Creat_Label(Obj_left, "", &lv_font_montserrat_12);
    lv_label_set_text_fmt(label3, "%d:%02d", (int)(widget->Value_TimeLeft / 60), (int)(widget->Value_TimeLeft % 60));
    lv_obj_set_pos(label3, 158, 60);
    lv_obj_set_style_text_color(label3, lv_color_hex(FONT_RED_COLOR), LV_PART_MAIN);
    widget->Label_TimeLeft = label3;

    /*右上框*/
    lv_obj_t* Obj_rigth = Create_Obj(page_cont, 160, 90, 0xffffff, 0);
    lv_obj_set_pos(Obj_rigth, 280, 5);
    lv_obj_add_style(Obj_rigth, &style_shadow, LV_PART_MAIN);
    lv_obj_add_style(Obj_rigth, &style_gradient, LV_PART_MAIN);
    lv_obj_t* Img_rigth = Creat_Image(Obj_rigth, NEXTPRE_ICON);
    lv_obj_set_pos(Img_rigth, 10, 10);

    lv_obj_t* title2 = Creat_Label(Obj_rigth, "Next preview", &lv_font_montserrat_14);
    lv_obj_set_pos(title2, 35, 15);
    lv_obj_set_style_text_color(title2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* Obj2 = Create_Obj(Obj_rigth, 110, 3, FONT_GREEN_COLOR, 0);
    lv_obj_set_pos(Obj2, 30, 53);
    lv_obj_set_style_bg_color(Obj2, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    lv_obj_t* title6 = Creat_Label(page_cont, "Abdominal Muscle\nMonitoring", &lv_font_montserrat_10);
    lv_obj_set_width(title6, 120);
    lv_obj_align(title6, LV_ALIGN_TOP_RIGHT, -15, 93);
    lv_obj_set_style_text_align(title6, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_color(title6, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    lv_obj_t* label4 = Creat_Label(page_cont, "", &lv_font_montserrat_12);
    lv_obj_align(label4, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_text_color(label4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Label_Finish = label4;

    Creat_En_Chart(widget, page_cont);
}

/**
 * @brief 选择疗程界面（Flex 换行，自动填充全部疗程格）—— 英文版
 *        [我们] 用本文件独立计数器（不依赖中文 g_Treat_Point_cnt）
 */
static uint8_t s_en_treat_point_cnt = 0;
static void Stage_En_Widget2(lv_obj_t* page_cont, const char* s, uint8_t t, Stage_Data_t* data);

void Select_En_Widget2(lv_obj_t* page_cont, Select_Data_t* data)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, data->High_Obj, FONT_RED_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_pos(cont, 10, -8);

    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 3, 0);
    lv_obj_set_style_pad_column(cont, 25, 0);

    s_en_treat_point_cnt = 0;

    for (uint8_t s = 0; s < data->Stage_Num; s++) {
        for (uint8_t t = 1; t <= data->Sta_Table[s].total_times; t++) {
            Stage_En_Widget2(cont, data->Sta_Table[s].name, t, &data->sta[s_en_treat_point_cnt]);
            data->Treat_Point[s_en_treat_point_cnt++] = (Treat_Point_t){ s, t };
        }
    }
    data->Btn_MaxSum = s_en_treat_point_cnt;
    lv_obj_scroll_to_y(cont, 0, LV_ANIM_OFF);
}

/**
 * @brief 阶段疗程控件（单格：按钮 + 图标 + 标签）—— 英文版
 */
static void Stage_En_Widget2(lv_obj_t* page_cont, const char* s, uint8_t t, Stage_Data_t* data)
{
    lv_obj_t* Btn = Create_Button(page_cont, 128, 92, FONT_WHITE_COLOR, 10, 1);
    lv_obj_t* Obj = Create_Obj(Btn, 128, 92, FONT_BLUE_COLOR, 0);
    data->obj = Obj;
    lv_obj_align(Obj, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t* Img = Creat_Image(Btn, Stage_unSel_ICON);
    lv_obj_center(Img);
    lv_obj_align(Img, LV_ALIGN_CENTER, 0, -17);
    lv_obj_set_style_image_recolor_opa(Img, 255, 0);
    lv_obj_set_style_image_recolor(Img, lv_color_hex(FONT_GRAY_COLOR), 0);

    data->btn = Btn;
    data->img = Img;

    lv_obj_t* Label1 = Creat_Label(Btn, (char*)s, &lv_font_montserrat_12);
    lv_obj_set_width(Label1, 120);
    lv_obj_set_style_text_align(Label1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    //lv_obj_set_pos(Label1, 4, 48);
    lv_obj_align_to(Label1, Btn, LV_ALIGN_OUT_BOTTOM_MID, 0, -38);
    lv_obj_set_style_text_color(Label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    lv_obj_t* Label2 = Creat_Label(Btn, "", &lv_font_montserrat_12);
    lv_obj_set_width(Label2, 120);
    lv_label_set_text_fmt(Label2, "Treatment %d", t);
    lv_obj_set_style_text_align(Label2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    //lv_obj_set_pos(Label2, 4, 63);
    lv_obj_align_to(Label2, Btn, LV_ALIGN_OUT_BOTTOM_MID, 0, -24);
    lv_obj_set_style_text_color(Label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

}

/**
 * @brief 疗程说明界面（标题 + 正文 + 图片 + Select 按钮）—— 英文版
 *        [我们] ESP32 Instr_Data_t 无 Text_Wid/Text_Hig，正文用固定 200x180（对齐中文版） */
void Instr_En_Widget(lv_obj_t* page_cont, Instr_Data_t* data)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_RED_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_pos(cont, -8, -8);

    lv_obj_t* Obj = Create_Obj(cont, data->Obj_Wid, data->Obj_Hig, 0xffffff, 0);
    lv_obj_set_pos(Obj, 18, -4);
    lv_obj_add_style(Obj, &style_gradient, LV_PART_MAIN);
    lv_obj_add_style(Obj, &style_shadow, LV_PART_MAIN);
    lv_obj_t* title = Creat_Label(Obj, data->Title, &lv_font_montserrat_16);
    lv_obj_set_pos(title, 10, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(data->Title_Color), LV_PART_MAIN);
    lv_obj_t* label = Creat_Label(Obj, "•", &lv_font_montserrat_12);
    lv_obj_set_pos(label, 10, 32);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text = Creat_TextLabel(Obj, 238, 180, data->Text, &lv_font_montserrat_12);
    lv_obj_set_pos(text, 18, 24);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(text, 2, 0);

    lv_obj_t* Img = Creat_Image(cont, POS_ICON);
    lv_obj_set_pos(Img, 291, 12);

    lv_obj_t* btn = Create_Button(cont, 110, 40, data->Title_Color, 30, 0);
    lv_obj_set_pos(btn, 305, 200);
    lv_obj_t* label2 = Creat_Label(btn, "Select", &lv_font_montserrat_14);
    lv_obj_center(label2);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    data->Btn = btn;
}
