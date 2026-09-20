/**
  ******************************************************************************
  * @文件名称   Ass_En_ui.c
  * @文件描述   英文盆底评估页实现（对应中文 PelFlo_AssPage/PelFlo_Ass_ui.c）。
  *            复用中文全局状态（Ass_Widget / g_write_idx / ser_green/red /
  *            滚动偏移 / ecg 采样数组）。
  ******************************************************************************
  */

#include "Ass_En_ui.h"
#include "Ass_En_ui_event.h"
#include "PelFlo_Ass_ui.h"
#include "PelFlo_Ass_ui_event.h"   /* 复用中文语言无关回调：BackToMenu / Ass_Page_Esc / scroll / Draw_EcgTable / Mydraw */
#include "basic.h"
#include "app_keypad.h"   /* keypad group */
#include "therapy_eval.h"
#include <math.h>

static void Ass_En_Page1_widget(Ass_Widget_t* widget);
static void Ass_En_Page2_widget(Ass_Widget_t* widget);
static void Ass_En_Page3_widget(Ass_Widget_t* widget);
static void Ass_En_Page4_widget(Ass_Widget_t* widget);
static void Ass_En_Page5_widget(Ass_Widget_t* widget);

static void Ass_En_Page2_Chart(Ass_Widget_t* widget, lv_obj_t* page_cont);
static void Ass_En_Page2_Chart2(Ass_Widget_t* widget, lv_obj_t* page_cont);
static void Ass_En_draw_pressure_excel(lv_obj_t* cont);
static void Record_En_EcgTable_wdiget(Ass_Widget_t* widget, lv_obj_t* page_cont);
static void Record_En_DataMenu_widget(Ass_Widget_t* widget, lv_obj_t* page_cont, uint8_t amount);

/* 英文版 ESC 承接对象记录（中文 s_page_esc_obj 是 static 不可见，英文各自记录并清理） */
static lv_obj_t* s_en_page_esc_obj = NULL;
static lv_obj_t* s_en_guide_cont = NULL;
static uint8_t s_en_guide_phase = 0;
static Scroll_Data_t Ass_En_Scroll;

static void ass_en_guide_draw_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);

    if (s_en_guide_phase >= g_eval.phase_cnt) return;
    const rx_eval_phase_t *ph = &g_eval.phases[s_en_guide_phase];

    uint32_t total = 0;
    for (uint8_t i = 0; i < ph->step_cnt; i++){
        total += (uint32_t)ph->steps[i].dur_s * 20u;
    } 
    total *= ph->repeat;
    if (!total) return;

    lv_area_t area;
    lv_obj_get_coords(obj, &area);
    lv_draw_line_dsc_t d; 
    lv_draw_line_dsc_init(&d);
    d.base.layer = layer;
    d.color = lv_color_hex(FONT_RED_COLOR); 
    d.width = 2;
    d.p1.x = area.x1; 
    d.p1.y = area.y1 + 90;
    d.p2.x = area.x1 + 370; 
    d.p2.y = area.y1 + 90;
    lv_draw_line(layer, &d);

    uint32_t tick = 0;
    for (uint8_t r = 0; r < ph->repeat; r++) {
        for (uint8_t i = 0; i < ph->step_cnt; i++) {
            const rx_step_t *st = &ph->steps[i]; 
            uint32_t span = (uint32_t)st->dur_s * 20u;
            if (st->type == RX_STEP_TRAIN && span) {
                int32_t x0 = area.x1 + (int32_t)((uint64_t)tick * 370u / total);
                int32_t x1 = area.x1 + (int32_t)((uint64_t)(tick + span) * 370u / total);
                int32_t prev_x = x0, prev_y = area.y1 + 90;
                for (int32_t n = 1; n <= x1 - x0; n++) {
                    int32_t x = x0 + n; 
                    float q = (float)n / (float)(x1 - x0);
                    int32_t y = area.y1 + 90 - (int32_t)(58.0f * sinf(q * 3.1415926f));
                    d.p1.x = prev_x; 
                    d.p1.y = prev_y; 
                    d.p2.x = x; 
                    d.p2.y = y; 
                    lv_draw_line(layer, &d);
                    prev_x = x; 
                    prev_y = y;
                }
            }
            tick += span;
        }
    }
}

static void ass_en_guide_init(lv_obj_t *parent)
{
    s_en_guide_cont = lv_obj_create(parent);
    lv_obj_set_pos(s_en_guide_cont, 45, 130); lv_obj_set_size(s_en_guide_cont, 370, 120);
    lv_obj_set_style_bg_opa(s_en_guide_cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_en_guide_cont, 0, 0); lv_obj_set_style_pad_all(s_en_guide_cont, 0, 0);
    lv_obj_set_scrollbar_mode(s_en_guide_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(s_en_guide_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_en_guide_cont, ass_en_guide_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
}

void ass_en_guide_update(uint8_t phase)
{
    s_en_guide_phase = phase;
    if (s_en_guide_cont) lv_obj_invalidate(s_en_guide_cont);
}

/**
 * @brief 英文评估页子页面创建入口（Page_Clean 后按页重建英文 UI）
 */
void Ass_En_Page_Load(Ass_PageID_t page_id)
{
    Ass_LeaveGroup();           /*复用中文：先移出 group，防 Page_Clean 时 refocus FOCUSED 悬垂（坑 6.8） */
    /*清理英文 ESC 承接对象（Page2/3 注册的 cont），防 group 悬垂 */
    if (s_en_page_esc_obj && lv_obj_is_valid(s_en_page_esc_obj)) lv_group_remove_obj(s_en_page_esc_obj);
    s_en_page_esc_obj = NULL;
    s_en_guide_cont = NULL;
    Page_Clean();
    switch (page_id) {
    case AssPage1:
        Ass_En_Page1_widget(&Ass_Widget);
        break;
    case AssPage2:
        Ass_En_Page2_widget(&Ass_Widget);
        break;
    case AssPage3:
        Ass_En_Page3_widget(&Ass_Widget);
        break;
    case AssPage4:
        Ass_En_Page4_widget(&Ass_Widget);
        break;
    case AssPage5:
        Ass_En_Page5_widget(&Ass_Widget);
        break;
    default:
        break;
    }
}

static void page_scroll_key_cb(lv_event_t* e)
{
    Scroll_Data_t* scroll = (Scroll_Data_t*)lv_event_get_user_data(e);
    uint32_t key = lv_event_get_key(e);
    int16_t step = 40;

    if (key == LV_KEY_DOWN) {
        if (lv_obj_get_scroll_y(scroll->Obj) >= scroll->Origin_Y) return;
        lv_obj_scroll_by(scroll->Obj, 0, -step, LV_ANIM_ON);
        lv_event_stop_processing(e);
    }
    else if (key == LV_KEY_UP) {
        if (lv_obj_get_scroll_y(scroll->Obj) <= scroll->END_Y) return;
        lv_obj_scroll_by(scroll->Obj, 0, step, LV_ANIM_ON);
        lv_event_stop_processing(e);
    }
}

/**
 * @brief 英文盆底评估第1页（说明页）
 */
static void Ass_En_Page1_widget(Ass_Widget_t* widget)
{
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    /* 页面滚动设置 */
    Ass_En_Scroll.Obj = cont;
    Ass_En_Scroll.Origin_Y = 210;
    Ass_En_Scroll.END_Y = 0;
    widget->Scroll = &Ass_En_Scroll;
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /*说明框*/
    lv_obj_t* Obj = Create_Obj(cont, 404, 160, 0xffffff, 0);
    lv_obj_set_pos(Obj, 25, 5);
    lv_obj_add_style(Obj, &style_shadow, LV_PART_MAIN);
    lv_obj_add_style(Obj, &style_gradient, LV_PART_MAIN);
    /*标题*/
    lv_obj_t* title = Creat_Label(Obj, "Description of Pelvic floor assessment", &lv_font_montserrat_18);
    lv_obj_set_pos(title, 20, 8);
    lv_obj_set_style_text_color(title, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    /*正文*/
    lv_obj_t* label = Creat_Label(Obj, "•", &lv_font_montserrat_12);
    lv_obj_set_pos(label, 20, 43);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text = Creat_TextLabel(Obj, 350, 100,
        "5-minute assessment: Analyze pelvic floor muscle "
        "health through EMG signals and intelligent algorithms. "
        "Follow voice & graphics to contract and relax pelvic floor "
        "muscles. Avoid abdominal and thigh tension for precise results.",
        &lv_font_montserrat_12);
    lv_obj_set_pos(text, 30, 35);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(text, 4, 0);

    /*线材接线图*/
    lv_obj_t* Obj_img = Create_Obj(cont, 404, 300, 0xffffff, 0);
    lv_obj_set_pos(Obj_img, 30, 200);
    lv_obj_set_style_radius(Obj_img, 15, LV_PART_MAIN);
    lv_obj_set_style_border_width(Obj_img, 1, LV_PART_MAIN);
    lv_obj_set_style_border_opa(Obj_img, 255, LV_PART_MAIN);
    lv_obj_set_style_border_color(Obj_img, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_clear_flag(Obj_img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* Img = Creat_Image(Obj_img, "C:/Users/zqt/Desktop/PDJ_LVGL/png/page2/2_4_2.bin");
    lv_obj_set_pos(Img, 0, 0);

    /*底部模糊图层*/
    lv_obj_t* Obj2 = Create_Obj(cont, 480, 60, 0x2195f6, 0);
    lv_obj_set_pos(Obj2, -8, 225);
    lv_obj_set_style_border_width(Obj2, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Obj2, 120, LV_PART_MAIN);
    lv_obj_clear_flag(Obj2, LV_OBJ_FLAG_SCROLLABLE);
    Obj_origin_y = 225;
    lv_obj_add_event_cb(cont, Obj_scroll_cb, LV_EVENT_SCROLL, Obj2);

    /*查看历史按钮*/
    widget->History_Btn = Create_Button(cont, 110, 40, 0xffffff, 30, 0);
    lv_obj_set_pos(widget->History_Btn, 195, 200);
    lv_obj_add_style(widget->History_Btn, &style_BlueShadow, LV_PART_MAIN);
    widget->History_Btn_Label = Creat_Label(widget->History_Btn, "History", &lv_font_montserrat_14);
    lv_obj_center(widget->History_Btn_Label);
    lv_obj_set_style_text_color(widget->History_Btn_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    HisBtn_origin_y = 200;
    lv_obj_add_event_cb(cont, HisBtn_scroll_cb, LV_EVENT_SCROLL, widget->History_Btn);
    lv_obj_add_event_cb(widget->History_Btn, Ass_En_Page1_Btn_FocEvent_cb, LV_EVENT_ALL, widget->History_Btn_Label);
    lv_obj_add_event_cb(widget->History_Btn, page_scroll_key_cb, LV_EVENT_KEY, widget->Scroll);

    /*开始评估按钮*/
    widget->Ass_Btn = Create_Button(cont, 110, 40, 0xffffff, 30, 0);
    lv_obj_set_pos(widget->Ass_Btn, 324, 200);
    lv_obj_add_style(widget->Ass_Btn, &style_BlueShadow, LV_PART_MAIN);
    widget->Ass_Btn_Label = Creat_Label(widget->Ass_Btn, "Start", &lv_font_montserrat_14);
    lv_obj_center(widget->Ass_Btn_Label);
    lv_obj_set_style_text_color(widget->Ass_Btn_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    AssBtn_origin_y = 200;
    lv_obj_add_event_cb(cont, AssBtn_scroll_cb, LV_EVENT_SCROLL, widget->Ass_Btn);
    lv_obj_add_event_cb(widget->Ass_Btn, Ass_En_Page1_Btn_FocEvent_cb, LV_EVENT_ALL, widget->Ass_Btn_Label);
    lv_obj_add_event_cb(widget->Ass_Btn, page_scroll_key_cb, LV_EVENT_KEY, widget->Scroll);

    /* ===== [我们] 注册按钮到 keypad group（左右切换 + 确认/ESC） ===== */
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        lv_group_add_obj(g, widget->History_Btn);
        lv_group_add_obj(g, widget->Ass_Btn);
        lv_group_focus_obj(widget->Ass_Btn);
    }
}

/**
 * @brief 英文盆底评估第2页（评估过程）
 */
static void Ass_En_Page2_widget(Ass_Widget_t* widget)
{
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    /*左上框*/
    lv_obj_t* Obj_left = Create_Obj(cont, 250, 90, 0xffffff, 0);
    lv_obj_set_pos(Obj_left, 15, 5);
    lv_obj_add_style(Obj_left, &style_shadow, LV_PART_MAIN);
    lv_obj_add_style(Obj_left, &style_gradient, LV_PART_MAIN);
    lv_obj_t* Img_left = Creat_Image(Obj_left, "C:/Users/zqt/Desktop/PDJ_LVGL/png/AssPage/2_1.bin");
    lv_obj_set_pos(Img_left, 10, 10);
    lv_obj_t* title1 = Creat_Label(Obj_left, "Vagina", &lv_font_montserrat_14);
    lv_obj_set_pos(title1, 35, 15);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Phase_Label = title1;

    lv_obj_t* title3 = Creat_Label(Obj_left, "Max EMG\nPotential", &lv_font_montserrat_10);
    lv_obj_set_pos(title3, 20, 36);
    lv_obj_set_width(title3, 70);
    lv_obj_set_style_text_color(title3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Max_Label = Creat_Label(Obj_left, "50.8uV", &lv_font_montserrat_14);
    lv_obj_set_pos(widget->Max_Label, 20, 60);
    lv_obj_set_width(widget->Max_Label, 70);
    lv_obj_set_style_text_color(widget->Max_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    lv_obj_t* title4 = Creat_Label(Obj_left, "Instant EMG\nPotential", &lv_font_montserrat_10);
    lv_obj_set_pos(title4, 92, 36);
    lv_obj_set_width(title4, 78);
    lv_obj_set_style_text_color(title4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Instan_Label = Creat_Label(Obj_left, "50.8uV", &lv_font_montserrat_14);
    lv_obj_set_pos(widget->Instan_Label, 95, 60);
    lv_obj_set_style_text_color(widget->Instan_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    lv_obj_t* title5 = Creat_Label(Obj_left, "Time Left", &lv_font_montserrat_10);
    lv_obj_set_pos(title5, 175, 36);
    lv_obj_set_width(title5, 65);
    lv_obj_set_style_text_color(title5, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->RemTime_Label = Creat_Label(Obj_left, "4:30", &lv_font_montserrat_14);
    lv_obj_set_pos(widget->RemTime_Label, 183, 60);
    lv_obj_set_style_text_color(widget->RemTime_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    /*右上框*/
    lv_obj_t* Obj_rigth = Create_Obj(cont, 160, 90, 0xffffff, 0);
    lv_obj_set_pos(Obj_rigth, 280, 5);
    lv_obj_add_style(Obj_rigth, &style_shadow, LV_PART_MAIN);
    lv_obj_add_style(Obj_rigth, &style_gradient, LV_PART_MAIN);
    lv_obj_t* Img_rigth = Creat_Image(Obj_rigth, "C:/Users/zqt/Desktop/PDJ_LVGL/png/AssPage/2_2.bin");
    lv_obj_set_pos(Img_rigth, 10, 10);
    lv_obj_t* title2 = Creat_Label(Obj_rigth, "Next preview", &lv_font_montserrat_14);
    lv_obj_set_pos(title2, 35, 15);
    lv_obj_set_style_text_color(title2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Next_Action_Label = Creat_Label(Obj_rigth, "Relax", &lv_font_montserrat_12);
    lv_obj_set_pos(widget->Next_Action_Label, 35, 42);
    lv_obj_set_style_text_color(widget->Next_Action_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_t* Obj2 = Create_Obj(Obj_rigth, 110, 3, FONT_GREEN_COLOR, 0);
    lv_obj_set_pos(Obj2, 30, 53);
    lv_obj_set_style_bg_color(Obj2, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    /*曲线表*/
    ass_en_guide_init(cont);
    // Ass_En_Page2_Chart2(widget, cont);
    Ass_En_Page2_Chart(widget, cont);
    ass_en_guide_update(0);

    lv_obj_t* title6 = Creat_Label(cont, "Abdominal Muscle\nMonitoring", &lv_font_montserrat_10);
    lv_obj_set_width(title6, 120);
    lv_obj_align(title6, LV_ALIGN_TOP_RIGHT, -15, 93);
    //lv_obj_set_pos(title6, 390, 105);
    lv_obj_set_style_text_align(title6, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_text_color(title6, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t *action_box = Create_Obj(cont, 150, 28, 0xffffff, 4);
    lv_obj_set_pos(action_box, 42, 96);
    lv_obj_set_style_bg_opa(action_box, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(action_box, 0, LV_PART_MAIN);
    widget->Action_Label = Creat_Label(action_box, "Relax", &lv_font_montserrat_12);
    lv_obj_center(widget->Action_Label);
    lv_obj_set_style_text_color(widget->Action_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    /* =====  评估中：注册 cont 承接 ESC（中止评估回菜单） ===== */
    lv_group_t *g2 = app_keypad_get_group();
    if (g2) { lv_group_add_obj(g2, cont); lv_group_focus_obj(cont); s_en_page_esc_obj = cont; }
    lv_obj_add_event_cb(cont, Ass_Page_Esc_cb, LV_EVENT_KEY, NULL);
}

/**
 * @brief 英文盆底评估第3页（生成报告中）
 */
static void Ass_En_Page3_widget(Ass_Widget_t* widget)
{
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    lv_obj_t* Img = Creat_Image(cont, "C:/Users/zqt/Desktop/PDJ_LVGL/png/AssPage/3_1.bin");
    lv_obj_set_pos(Img, 172, 38);
    lv_obj_t* title1 = Creat_Label(cont, "Evaluation completed, generating report.", &lv_font_montserrat_20);
    lv_obj_set_width(title1, 440);
    lv_obj_set_style_text_align(title1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title1, LV_ALIGN_CENTER, 0, 72);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* title2 = Creat_Label(cont, "Please wait ..", &lv_font_montserrat_20);
    lv_obj_set_width(title2, 220);
    lv_obj_set_style_text_align(title2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title2, LV_ALIGN_CENTER, 0, 102);
    lv_obj_set_style_text_color(title2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_timer_t* report_timer = lv_timer_create(Ass_En_report_timer_cb, 100, cont);
    lv_timer_pause(report_timer);
    widget->Report_Timer = report_timer;

    /* ===== [我们] 生成报告中：注册 cont 承接 ESC（返回菜单） ===== */
    lv_group_t *g3 = app_keypad_get_group();
    if (g3) { lv_group_add_obj(g3, cont); lv_group_focus_obj(cont); s_en_page_esc_obj = cont; }
    lv_obj_add_event_cb(cont, Ass_Page_Esc_cb, LV_EVENT_KEY, NULL);
}

/**
 * @brief 英文盆底评估第4页（检测报告）
 */
static void Ass_En_Page4_widget(Ass_Widget_t* widget)
{
    static lv_point_precise_t line_1[] = { {98, 0},{0, 0} };
    static lv_point_precise_t line_2[] = { {0, 0},{0, 33} };
    static lv_point_precise_t line_3[] = { {0, 0},{0, 331} };

    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    Record_En_wdiget(widget, cont);        //数据表格（英文）

    /*标题：参考目标图，顶部居中*/
    lv_obj_t* title1 = Creat_Label(cont, "Assessment Report", &lv_font_montserrat_18);
    lv_obj_set_width(title1, 300);
    lv_obj_set_style_text_align(title1, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title1, LV_ALIGN_TOP_MID, 0, 2);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /*返回按钮：沿用中文版最终位置和大小*/
    lv_obj_t* btn = Create_Button(cont, 64, 24, FONT_WHITE_COLOR, 20, 0);
    lv_obj_set_pos(btn, 392, 4);
    lv_obj_add_style(btn, &style_BlueShadow, LV_PART_MAIN);
    lv_obj_t* label2 = Creat_Label(btn, "Back", &lv_font_montserrat_12);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_center(label2);
    basic_widget.Report_BackBtn = btn;

    /* [我们] LV_EVENT_ALL：复用中文 BackToMenu_Event_cb（ESC/确认都能触发） */
    lv_obj_add_event_cb(btn, BackToMenu_Event_cb, LV_EVENT_ALL, NULL);

    lv_group_t *g4 = app_keypad_get_group();
    if (g4) { lv_group_add_obj(g4, btn); lv_group_focus_obj(btn); }
}

/**
 * @brief 英文盆底评估第5页（健康档案）
 */
static void Ass_En_Page5_widget(Ass_Widget_t* widget)
{
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    Record_En_DataMenu_widget(widget, cont, 30);
}

/**
 * @brief 英文记录界面菜单栏设计（记录总览）
 */
static void Record_En_DataMenu_widget(Ass_Widget_t* widget, lv_obj_t* page_cont, uint8_t amount)
{
    lv_obj_t* cont;
    lv_obj_t* label;
    lv_obj_t* sub_page;
    lv_obj_t* menu = lv_menu_create(page_cont);
    lv_obj_set_size(menu, 480, 290);
    lv_obj_center(menu);
    lv_obj_set_style_bg_opa(menu, 255, LV_PART_MAIN);
    lv_obj_set_style_bg_color(menu, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_clear_flag(menu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(menu, LV_SCROLLBAR_MODE_OFF);
    lv_menu_set_mode_header(menu, LV_MENU_HEADER_TOP_UNFIXED);

    widget->Datamenu = menu;

    lv_obj_t* back_btn = lv_menu_get_main_header_back_button(menu);
    (void)back_btn;

    lv_obj_t* hdr = lv_menu_get_main_header(menu);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1E88E5), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(hdr, 255, LV_PART_MAIN);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(hdr, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0x1565C0), LV_PART_MAIN);
    lv_obj_set_style_pad_ver(hdr, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(hdr, 12, LV_PART_MAIN);
    lv_obj_t* title = lv_obj_get_child(hdr, 1);
    lv_obj_set_flex_grow(title, 0);
    lv_obj_set_style_margin_left(title, 160, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(FONT_GREEN_COLOR), 0);

    lv_obj_t* main_page = lv_menu_page_create(menu, "Health Records");
    lv_obj_set_style_bg_color(main_page, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_size(main_page, 480, 280);
    lv_obj_clear_flag(main_page, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* obj = lv_menu_section_create(main_page);
    lv_obj_set_size(obj, 480, 170);
    lv_obj_set_style_bg_color(obj, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    for (uint8_t i = 0; i < amount; i++) {
        char arr[50], arr2[5];

        lv_obj_t* sub_page = lv_menu_page_create(menu, "Assessment Report");
        lv_obj_add_event_cb(sub_page, Ass_En_Record_Menu_event_cb, LV_EVENT_STYLE_CHANGED, NULL);
        cont = lv_menu_cont_create(obj);
        lv_obj_set_style_height(cont, 40, LV_PART_MAIN);
        label = lv_label_create(cont);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
        lv_label_set_text_fmt(label, "          Pelvic Floor Assessment                     "
            "                          2027.%d.%d", (int)lv_rand(1, 12), (int)lv_rand(1, 31));
        lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
        lv_menu_set_load_page_event(menu, cont, sub_page);
    }
    lv_obj_t* section = lv_menu_cont_create(main_page);
    lv_obj_t* btn = Create_Button(section, 88, 33, FONT_WHITE_COLOR, 20, 0);
    lv_obj_set_flex_grow(btn, 0);
    lv_obj_set_style_margin_left(btn, 350, 0);
    lv_obj_set_style_margin_top(btn, 30, 0);
    lv_obj_add_style(btn, &style_BlueShadow, LV_PART_MAIN);
    lv_obj_t* label2 = Creat_Label(btn, "Back", &lv_font_montserrat_12);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_center(label2);
    basic_widget.Record_BackBtn = btn;

    /* [我们] LV_EVENT_ALL：复用中文 BackToMenu_Event_cb（ESC/确认都能触发） */
    lv_obj_add_event_cb(btn, BackToMenu_Event_cb, LV_EVENT_ALL, menu);

    lv_group_t *g5 = app_keypad_get_group();
    if (g5) { lv_group_add_obj(g5, btn); lv_group_focus_obj(btn); }
    lv_menu_set_page(menu, main_page);
}

/**
 * @brief 英文评估报告界面
 */
void Record_En_wdiget(Ass_Widget_t* widget, lv_obj_t* page_cont)
{
    /*
     * 英文列宽按目标图重新分配，总宽度保持 440：
     * Stage 96 / Parameter 98 / Range 82 / Results 98 / Score 66
     */
    static lv_point_precise_t line_stage[] = { {96, 0},{0, 0} };

    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 0);
    lv_obj_set_style_radius(cont, 18, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(cont, 255, LV_PART_MAIN);
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cont, 0, 0);

    /*表格主体：沿用中文版最终稳定位置*/
    lv_obj_t* Obj_table = Create_Obj(cont, 480, 370, FONT_YELLOW_COLOR, 0);
    lv_obj_set_pos(Obj_table, -4, 77);
    lv_obj_set_style_pad_all(Obj_table, 0, 0);

    /*创建右侧数据表格*/
    Record_En_EcgTable_wdiget(widget, Obj_table);

    /*******************英文表格头*******************/
    lv_obj_t* Obj_header = Create_Obj(cont, 480, 64, FONT_GRAY_COLOR, 0);
    lv_obj_set_pos(Obj_header, 0, 26);
    lv_obj_set_style_pad_all(Obj_header, 0, 0);
    lv_obj_set_style_bg_opa(Obj_header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(Obj_header, 0, LV_PART_MAIN);

    lv_obj_t* Obj2 = Create_Obj(Obj_header, 440, 32, FONT_GREEN_COLOR, 1);
    lv_obj_align(Obj2, LV_ALIGN_BOTTOM_MID, -4, -1);
    lv_obj_set_style_pad_all(Obj2, 0, 0);

    /*Stage*/
    lv_obj_t* title2 = Creat_Label(Obj2, "Stage", &lv_font_montserrat_12);
    lv_obj_set_width(title2, 96);
    lv_obj_set_pos(title2, 0, 8);
    lv_obj_set_style_text_align(title2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title2, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*Parameter*/
    lv_obj_t* title8 = Creat_Label(Obj2, "Parameter", &lv_font_montserrat_12);
    lv_obj_set_width(title8, 98);
    lv_obj_set_pos(title8, 96, 8);
    lv_obj_set_style_text_align(title8, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title8, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*Range*/
    lv_obj_t* title9 = Creat_Label(Obj2, "Range", &lv_font_montserrat_12);
    lv_obj_set_width(title9, 82);
    lv_obj_set_pos(title9, 194, 8);
    lv_obj_set_style_text_align(title9, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title9, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*Results*/
    lv_obj_t* title10 = Creat_Label(Obj2, "Results", &lv_font_montserrat_12);
    lv_obj_set_width(title10, 98);
    lv_obj_set_pos(title10, 276, 8);
    lv_obj_set_style_text_align(title10, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title10, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*Score*/
    lv_obj_t* title11 = Creat_Label(Obj2, "Score", &lv_font_montserrat_12);
    lv_obj_set_width(title11, 66);
    lv_obj_set_pos(title11, 374, 8);
    lv_obj_set_style_text_align(title11, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title11, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*表头底部左侧横线*/
    lv_obj_t* line6 = creat_line(Obj2, 96, 1, line_stage, 1);
    lv_obj_set_pos(line6, 0, 31);
    lv_obj_set_style_line_color(line6, lv_color_hex(0xffffff), LV_PART_MAIN);

    /*表头竖分隔线：1px*/
    lv_obj_t* line7 = lv_obj_create(Obj2);
    lv_obj_set_size(line7, 1, 32);
    lv_obj_set_pos(line7, 96, 0);
    lv_obj_set_style_bg_color(line7, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line7, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line7, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line7, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line7, 0, 0);
    lv_obj_clear_flag(line7, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* line_header8 = lv_obj_create(Obj2);
    lv_obj_set_size(line_header8, 1, 32);
    lv_obj_set_pos(line_header8, 194, 0);
    lv_obj_set_style_bg_color(line_header8, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line_header8, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line_header8, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line_header8, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line_header8, 0, 0);
    lv_obj_clear_flag(line_header8, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* line_header9 = lv_obj_create(Obj2);
    lv_obj_set_size(line_header9, 1, 32);
    lv_obj_set_pos(line_header9, 276, 0);
    lv_obj_set_style_bg_color(line_header9, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line_header9, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line_header9, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line_header9, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line_header9, 0, 0);
    lv_obj_clear_flag(line_header9, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* line_header10 = lv_obj_create(Obj2);
    lv_obj_set_size(line_header10, 1, 32);
    lv_obj_set_pos(line_header10, 374, 0);
    lv_obj_set_style_bg_color(line_header10, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line_header10, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line_header10, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line_header10, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line_header10, 0, 0);
    lv_obj_clear_flag(line_header10, LV_OBJ_FLAG_SCROLLABLE);
    /*******************英文表格头*******************/

    /*底部 Total Score 区域*/
    lv_obj_t* Obj3 = Create_Obj(Obj_table, 278, 34, FONT_GREEN_COLOR, 1);
    lv_obj_set_pos(Obj3, 116, 309);

    lv_obj_t* Obj4 = Create_Obj(Obj_table, 66, 35, 0xF2F4FA, 1);
    lv_obj_set_pos(Obj4, 394, 309);
    lv_obj_set_style_radius(Obj4, 0, LV_PART_MAIN);

    /*左侧阶段蓝色区域：方角、无边框*/
    lv_obj_t* Obj1 = lv_obj_create(Obj_table);
    lv_obj_set_size(Obj1, 96, 298);
    lv_obj_set_pos(Obj1, 20, 11);
    lv_obj_set_style_bg_color(Obj1, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Obj1, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(Obj1, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(Obj1, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(Obj1, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_outline_width(Obj1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(Obj1, 0, 0);
    lv_obj_clear_flag(Obj1, LV_OBJ_FLAG_SCROLLABLE);

    /*Pre-rest Phase：对应右侧2行*/
    lv_obj_t* phase1 = lv_obj_create(Obj1);
    lv_obj_set_size(phase1, 96, 67);
    lv_obj_set_pos(phase1, 0, 0);
    lv_obj_set_style_bg_opa(phase1, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(phase1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(phase1, 0, 0);
    lv_obj_clear_flag(phase1, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title3 = Creat_Label(phase1, "Pre-rest Phase", &lv_font_montserrat_12);
    lv_obj_set_width(title3, 94);
    lv_obj_set_style_text_align(title3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title3, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title3, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*Fast Contraction Phase：对应右侧3行，长文字换行*/
    lv_obj_t* phase2 = lv_obj_create(Obj1);
    lv_obj_set_size(phase2, 96, 98);
    lv_obj_set_pos(phase2, 0, 67);
    lv_obj_set_style_bg_opa(phase2, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(phase2, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(phase2, 0, 0);
    lv_obj_clear_flag(phase2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title4 = Creat_Label(phase2, "Fast Contraction\nPhase", &lv_font_montserrat_12);
    lv_obj_set_width(title4, 94);
    lv_obj_set_style_text_align(title4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(title4, 1, LV_PART_MAIN);
    lv_obj_align(title4, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title4, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*Sustained Contraction：对应右侧2行*/
    lv_obj_t* phase3 = lv_obj_create(Obj1);
    lv_obj_set_size(phase3, 96, 66);
    lv_obj_set_pos(phase3, 0, 165);
    lv_obj_set_style_bg_opa(phase3, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(phase3, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(phase3, 0, 0);
    lv_obj_clear_flag(phase3, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title5 = Creat_Label(phase3, "Sustained\nContraction", &lv_font_montserrat_12);
    lv_obj_set_width(title5, 94);
    lv_obj_set_style_text_align(title5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_line_space(title5, 1, LV_PART_MAIN);
    lv_obj_align(title5, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title5, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*Post-rest Phase：对应右侧2行*/
    lv_obj_t* phase4 = lv_obj_create(Obj1);
    lv_obj_set_size(phase4, 96, 67);
    lv_obj_set_pos(phase4, 0, 231);
    lv_obj_set_style_bg_opa(phase4, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(phase4, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(phase4, 0, 0);
    lv_obj_clear_flag(phase4, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title6 = Creat_Label(phase4, "Post-rest Phase", &lv_font_montserrat_12);
    lv_obj_set_width(title6, 94);
    lv_obj_set_style_text_align(title6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title6, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title6, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*阶段分界横线：1px，铺满96px*/
    const int phase_line_y[4] = {78, 176, 242, 309};
    for (uint8_t i = 0; i < 4; i++) {
        lv_obj_t* line = lv_obj_create(Obj_table);
        lv_obj_set_size(line, 96, 1);
        lv_obj_set_pos(line, 20, phase_line_y[i]);
        lv_obj_set_style_bg_color(line, lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(line, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(line, 0, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    }

    /*正文竖分隔线：Stage / Parameter / Range / Results / Score*/
    const int body_line_x[4] = {116, 214, 296, 394};
    for (uint8_t i = 0; i < 4; i++) {
        lv_obj_t* line = lv_obj_create(Obj_table);
        lv_obj_set_size(line, 1, 298);
        lv_obj_set_pos(line, body_line_x[i], 11);
        lv_obj_set_style_bg_color(line, lv_color_hex(0xffffff), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(line, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(line, 0, 0);
        lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_t* title7 = Creat_Label(Obj3, "Total Score", &lv_font_montserrat_12);
    lv_obj_set_width(title7, 278);
    lv_obj_set_style_text_align(title7, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_center(title7);
    lv_obj_set_style_text_color(title7, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
}

/**
 * @brief 英文评估报告表格
 */
static void Record_En_EcgTable_wdiget(Ass_Widget_t* widget, lv_obj_t* page_cont)
{
    /*文案参考目标图*/
    char* text[] = {
        "Average",
        "Variability",
        "Maximum",
        "Rise time",
        "Recovery time",
        "Average",
        "Variability",
        "Average",
        "Variability"
    };

    static lv_style_t style_table_bg;
    lv_style_init(&style_table_bg);
    lv_style_set_bg_opa(&style_table_bg, LV_OPA_COVER);
    lv_style_set_bg_color(&style_table_bg, lv_color_hex(0xffffff));
    lv_style_set_text_color(&style_table_bg, lv_color_white());
    lv_style_set_border_opa(&style_table_bg, 0);

    /*
     * 右侧四列宽度：
     * Parameter 98 / Range 82 / Results 98 / Score 66 = 344
     * 位置与上方英文表头严格对应。
     */
    lv_obj_t* obj = Create_Obj(page_cont, 348, 335, 0x6483CD, 1);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN);
    lv_obj_set_pos(obj, 112, -29);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* table = lv_table_create(obj);
    lv_obj_set_size(table, 344, 303);
    lv_obj_add_style(table, &style_table_bg, LV_PART_ITEMS);
    lv_obj_add_style(table, &style_table_bg, LV_PART_MAIN);
    lv_obj_set_pos(table, -8, 22);

    /*英文正文使用 Montserrat 12，行高沿用中文版最后效果*/
    lv_obj_set_style_pad_ver(table, 9, LV_PART_ITEMS);
    lv_obj_set_style_pad_hor(table, 0, LV_PART_ITEMS);
    lv_obj_set_style_border_width(table, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(table, 0, LV_PART_ITEMS);
    lv_obj_set_scrollbar_mode(table, LV_SCROLLBAR_MODE_ON);
    lv_obj_set_scroll_dir(table, LV_DIR_VER);

    for (uint8_t i = 0; i < 9; i++) {
        lv_table_set_cell_value_fmt(table, i, 0, "%s", text[i]);
    }

    for (uint8_t i = 0; i < 10; i++) {
        lv_table_set_cell_value_fmt(table, i, 1, "%d", (int)lv_rand(200, 1000));
        lv_table_set_cell_value_fmt(table, i, 2, "%d", (int)lv_rand(200, 1000));
        lv_table_set_cell_value_fmt(table, i, 3, "%d", (int)lv_rand(200, 1000));
    }

    lv_table_set_column_count(table, 4);
    lv_table_set_row_count(table, 9);

    lv_table_set_column_width(table, 0, 98);   // Parameter
    lv_table_set_column_width(table, 1, 82);   // Range
    lv_table_set_column_width(table, 2, 98);   // Results
    lv_table_set_column_width(table, 3, 66);   // Score

    Set_Chinese_Font(table, &lv_font_montserrat_12);
    lv_obj_set_style_text_align(table, LV_TEXT_ALIGN_CENTER, LV_PART_ITEMS);
    lv_obj_set_style_text_color(table, lv_color_hex(FONT_GRAY_COLOR), LV_PART_ITEMS);

    lv_obj_add_event_cb(table, Draw_EcgTable_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    lv_obj_add_flag(table, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
}

/**
 * @brief 英文盆底评估第2页的绿色曲线图
 */
static void Ass_En_Page2_Chart(Ass_Widget_t* widget, lv_obj_t* page_cont)
{
    uint16_t i;
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 1);
    lv_style_set_bg_opa(&style, 0);
    lv_style_set_border_width(&style, 0);

    Ass_En_draw_pressure_excel(page_cont);

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
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 3000);
    lv_chart_set_point_count(chart, 400);

    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_rounded(chart, true, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_INDICATOR);

    ser_green = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_LIGHT_GREEN), LV_CHART_AXIS_PRIMARY_Y);

    lv_obj_add_flag(chart, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(chart, Mydraw_event_cb, LV_EVENT_DRAW_TASK_ADDED, ser_green);
    widget->Chart = chart;

    lv_timer_t* timer = lv_timer_create(Ass_En_Aemg_add_data_real, 50, chart);
    widget->Chart_Timer = timer;
    lv_timer_pause(widget->Chart_Timer);
}

/**
 * @brief 英文盆底评估第2页的红色曲线图
 */
static void Ass_En_Page2_Chart2(Ass_Widget_t* widget, lv_obj_t* page_cont)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 1);
    lv_style_set_bg_opa(&style, 0);
    lv_style_set_border_width(&style, 0);
    (void)widget;
    Ass_En_draw_pressure_excel(page_cont);
    /* Static red baseline, matching the reference line shown on the Chinese page. */
    lv_obj_t* red_chart = lv_chart_create(page_cont);
    lv_chart_set_type(red_chart, LV_CHART_TYPE_LINE);
    lv_obj_set_pos(red_chart, 45, 130);
    lv_obj_set_size(red_chart, 370, 120);
    lv_obj_set_style_bg_opa(red_chart, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(red_chart, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(red_chart, 0, 0);
    lv_chart_set_div_line_count(red_chart, 0, 0);
    lv_chart_set_update_mode(red_chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_chart_set_range(red_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 3000);
    lv_chart_set_point_count(red_chart, 400);
    lv_obj_set_style_line_width(red_chart, 2, LV_PART_ITEMS);
    lv_obj_set_style_line_rounded(red_chart, true, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(red_chart, 0, LV_PART_INDICATOR);
    ser_red = lv_chart_add_series(red_chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    for (uint16_t i = 0; i < 400; i++) lv_chart_set_next_value(red_chart, ser_red, 45);
    return;

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
    lv_chart_set_point_count(chart, 1000);

    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);
    lv_obj_set_style_line_rounded(chart, true, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_INDICATOR);

    ser_red = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    for (uint16_t i = 0; i < 1000; i++) {
        lv_chart_set_next_value(chart, ser_red, ecg_sample[i]);
    }
}

/**
 * @brief 英文创建曲线表格背景虚线
 */
static void Ass_En_draw_pressure_excel(lv_obj_t* cont)
{
    // static lv_point_precise_t line_1[] = { {0, 0},{400, 0} };
    // static lv_point_precise_t line_2[] = { {0, 15},{0, 164} };
    // /*横线*/
    // lv_obj_t* line1 = creat_line(cont, 400, 1, line_1, 2);
    // lv_obj_set_pos(line1, 45, 155);
    // lv_obj_set_style_line_color(line1, lv_color_hex(0x736f6f), LV_PART_MAIN);
    // lv_obj_set_style_line_opa(line1, 100, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_width(line1, 3, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_gap(line1, 3, LV_PART_MAIN);
    // lv_obj_t* line2 = creat_line(cont, 400, 1, line_1, 2);
    // lv_obj_set_pos(line2, 45, 137);
    // lv_obj_set_style_line_color(line2, lv_color_hex(0x736f6f), LV_PART_MAIN);
    // lv_obj_set_style_line_opa(line2, 100, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_width(line2, 3, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_gap(line2, 3, LV_PART_MAIN);
    // lv_obj_t* line3 = creat_line(cont, 400, 1, line_1, 2);
    // lv_obj_set_pos(line3, 45, 115);
    // lv_obj_set_style_line_color(line3, lv_color_hex(0x736f6f), LV_PART_MAIN);
    // lv_obj_set_style_line_opa(line3, 100, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_width(line3, 3, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_gap(line3, 3, LV_PART_MAIN);
    // /*竖线*/
    // lv_obj_t* line4 = creat_line(cont, 1, 164, line_2, 2);
    // lv_obj_set_pos(line4, 105, 107);
    // lv_obj_set_style_line_color(line4, lv_color_hex(0x736f6f), LV_PART_MAIN);
    // lv_obj_set_style_line_opa(line4, 100, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_width(line4, 3, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_gap(line4, 3, LV_PART_MAIN);
    // lv_obj_t* line5 = creat_line(cont, 1, 164, line_2, 2);
    // lv_obj_set_pos(line5, 175, 107);
    // lv_obj_set_style_line_color(line5, lv_color_hex(0x736f6f), LV_PART_MAIN);
    // lv_obj_set_style_line_opa(line5, 100, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_width(line5, 3, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_gap(line5, 3, LV_PART_MAIN);
    // lv_obj_t* line6 = creat_line(cont, 1, 164, line_2, 2);
    // lv_obj_set_pos(line6, 245, 107);
    // lv_obj_set_style_line_color(line6, lv_color_hex(0x736f6f), LV_PART_MAIN);
    // lv_obj_set_style_line_opa(line6, 100, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_width(line6, 3, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_gap(line6, 3, LV_PART_MAIN);
    // lv_obj_t* line7 = creat_line(cont, 1, 164, line_2, 2);
    // lv_obj_set_pos(line7, 315, 107);
    // lv_obj_set_style_line_color(line7, lv_color_hex(0x736f6f), LV_PART_MAIN);
    // lv_obj_set_style_line_opa(line7, 100, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_width(line7, 3, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_gap(line7, 3, LV_PART_MAIN);
    // lv_obj_t* line8 = creat_line(cont, 1, 164, line_2, 2);
    // lv_obj_set_pos(line8, 385, 107);
    // lv_obj_set_style_line_color(line8, lv_color_hex(0x736f6f), LV_PART_MAIN);
    // lv_obj_set_style_line_opa(line8, 100, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_width(line8, 3, LV_PART_MAIN);
    // lv_obj_set_style_line_dash_gap(line8, 3, LV_PART_MAIN);
    static lv_point_precise_t line_1[] = { {0, 0},{400, 0} };
    static lv_point_precise_t line_2[] = { {0, 15},{0, 164} };
    /*横线*/
    for (uint8_t i = 0; i < 5; i++) {
        lv_obj_t* line1 = creat_DottedLine(cont, 370, 6, line_1, 2);
        lv_obj_set_pos(line1, 45, 249 - (i * 29));
        lv_obj_clear_flag(line1, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    }
    lv_obj_t* text1 = Creat_Label(cont, "0", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text1, 25, 238);
    lv_obj_set_style_text_color(text1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text2 = Creat_Label(cont, "25", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text2, 20, 211);
    lv_obj_set_style_text_color(text2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text3 = Creat_Label(cont, "50", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text3, 20, 184);
    lv_obj_set_style_text_color(text3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text4 = Creat_Label(cont, "75", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text4, 20, 157);
    lv_obj_set_style_text_color(text4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text5 = Creat_Label(cont, "100", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text5, 13, 125);
    lv_obj_set_style_text_color(text5, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
}
