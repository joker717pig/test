#include "PelFlo_Ass_ui.h"
#include "PelFlo_Ass_ui_event.h"
#include "menu_ui_event_cb.h"
#include "app_keypad.h"   /* [我们] keypad group */
#include "therapy_eval.h" /* [我们] 评估结果（报告页指标/得分） */
#include "emg_force.h"    /* [我们] 评估中停止采集 */
#include "esp_log.h"         /* [我们] stub 日志 */
#include <math.h>   // 需要 sinf()
static const char *TAG = "Ass_ui";

static void PelFloAss_Page1_widget(Ass_Widget_t* widget);
static void PelFloAss_Page2_widget(Ass_Widget_t* widget);
static void PelFloAss_Page3_widget(Ass_Widget_t* widget);
static void PelFloAss_Page4_widget(Ass_Widget_t* widget);
static void PelFloAss_Page5_widget(Ass_Widget_t* widget);

static void PelFloAss_Page2_Chart(Ass_Widget_t* widget, lv_obj_t* page_cont);
static void PelFloAss_Page2_Chart2(Ass_Widget_t* widget, lv_obj_t* page_cont);
static void lv_example_chart_5(Ass_Widget_t* widget, lv_obj_t* page_cont);
static void draw_pressure_excel(lv_obj_t* cont);

static void Record_DataMenu_widget(Ass_Widget_t* widget, lv_obj_t* page_cont, uint8_t amount);
static void Record_DataMenu_widget2(Ass_Widget_t* widget, lv_obj_t* page_cont, uint8_t amount);


Ass_Widget_t Ass_Widget;

lv_coord_t HisBtn_origin_y;     //查看历史按钮起始位置
lv_coord_t AssBtn_origin_y;     //开始评估历史按钮起始位置
lv_coord_t Obj_origin_y;        //模糊遮罩起始位置

static lv_obj_t *s_page_esc_obj = NULL;   /* [我们] 当前无按钮页面的 ESC 焦点对象 */

/* [我们] 评估阶段引导波形（示意底图） */
static lv_obj_t *s_guide_cont = NULL;     /* 引导波形容器（波形图底层） */
static uint8_t   s_guide_phase = 0;       /* 当前阶段序号（0~3），绘制回调据此画三角峰 */
#define ASS_GUIDE_HIGH_Y   32             /* 用力峰顶 y（容器内，对齐刻度 75） */
#define ASS_GUIDE_LOW_Y    90             /* 静息基线 y（容器内，对齐刻度 25） */
#define ASS_GUIDE_BAR_H    3              /* 基线横线高度（px） */
#define ASS_GUIDE_COLOR    0xFFA500       /* 橙色 */

Scroll_Data_t Record_Scroll;        //报告滚动页面数据
Scroll_Data_t Menu_Record_Scroll;   //记录菜单滚动页面数据
Scroll_Data_t Ass_Scroll;           //评估页滚动页面数据
static lv_obj_t *s_record_back_btn = NULL;

/* ===== [我们] 评估页资源清理（切页/返回前调用，防悬垂崩溃，坑 6.8 同族） ===== */
void Ass_Stop_Timers(void)
{
    if (Ass_Widget.Chart_Timer)  { lv_timer_del(Ass_Widget.Chart_Timer);  Ass_Widget.Chart_Timer = NULL; }
    if (Ass_Widget.Report_Timer) { lv_timer_del(Ass_Widget.Report_Timer); Ass_Widget.Report_Timer = NULL; }
    s_guide_cont = NULL;   /* [我们] 引导层随 Page_Clean 一起删除，这里只清指针防悬垂 */
}

void Ass_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g == NULL) return;
    /* [我们] lv_obj_is_valid 防悬垂：History/Ass 等按钮在 Page_Clean 后指针已失效（Page4/Page5 时），
     * 直接 lv_group_remove_obj 悬垂指针 → use-after-free 崩溃 */
    if (lv_obj_is_valid(Ass_Widget.History_Btn))      lv_group_remove_obj(Ass_Widget.History_Btn);
    if (lv_obj_is_valid(Ass_Widget.Ass_Btn))          lv_group_remove_obj(Ass_Widget.Ass_Btn);
    if (lv_obj_is_valid(basic_widget.Record_BackBtn)) lv_group_remove_obj(basic_widget.Record_BackBtn);
    if (lv_obj_is_valid(basic_widget.Report_BackBtn)) lv_group_remove_obj(basic_widget.Report_BackBtn);
    if (s_page_esc_obj)                               { lv_group_remove_obj(s_page_esc_obj); s_page_esc_obj = NULL; }
}

void Ass_BackToMenu(void)
{
    Ass_Stop_Timers();
    Ass_LeaveGroup();
    Goto_MenuPage();
}

/* 前向声明 */
static void Ass_BackConfirm_Click_cb(lv_event_t* e);
static void Ass_BackConfirm_Key_cb(lv_event_t* e);

/* [我们] 返回确认弹窗点击回调 */
static void Ass_BackConfirm_Click_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    
    // 先把是/否移出 group，避免后续页面清理时 refocus 悬垂
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        if (lv_obj_is_valid(Ass_Widget.Window.Yes_Btn)) lv_group_remove_obj(Ass_Widget.Window.Yes_Btn);
        if (lv_obj_is_valid(Ass_Widget.Window.No_Btn))  lv_group_remove_obj(Ass_Widget.Window.No_Btn);
    }
    
    // 如果是"是"按钮，执行返回
    if (obj == Ass_Widget.Window.Yes_Btn) {
        // 关闭弹窗
        if (Ass_Widget.Window.Msgbox && lv_obj_is_valid(Ass_Widget.Window.Msgbox)) {
            lv_msgbox_close_async(Ass_Widget.Window.Msgbox);
        }
        Ass_BackToMenu();
    }
    // 如果是"否"按钮，恢复评估（如果之前暂停了图表定时器）
    else if (obj == Ass_Widget.Window.No_Btn) {
        // 关闭弹窗
        if (Ass_Widget.Window.Msgbox && lv_obj_is_valid(Ass_Widget.Window.Msgbox)) {
            lv_msgbox_close_async(Ass_Widget.Window.Msgbox);
        }
        if (Ass_Widget.Chart_Timer) {
            lv_timer_resume(Ass_Widget.Chart_Timer);
        }
    }
}

/* [我们] 返回确认弹窗按键回调（ESC=否，左右切换焦点） */
static void Ass_BackConfirm_Key_cb(lv_event_t* e)
{
    uint32_t key = lv_event_get_key(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    
    if (code == LV_EVENT_KEY) {
        if (key == LV_KEY_ESC) {
            // ESC 视为"否"，关闭弹窗
            if (Ass_Widget.Window.Msgbox && lv_obj_is_valid(Ass_Widget.Window.Msgbox)) {
                lv_msgbox_close_async(Ass_Widget.Window.Msgbox);
            }
        } else if (key == LV_KEY_LEFT) {
            if (obj == Ass_Widget.Window.No_Btn)  lv_group_focus_obj(Ass_Widget.Window.Yes_Btn);
        } else if (key == LV_KEY_RIGHT) {
            if (obj == Ass_Widget.Window.Yes_Btn)  lv_group_focus_obj(Ass_Widget.Window.No_Btn);
        }
    }
}

/* [我们] 显示返回确认弹窗 */
void Ass_ShowBackConfirm(void)
{
    LV_LOG_USER("[Ass] ShowBackConfirm called");
    // 1. 显示弹窗 (先显示，防止 eval_stop 触发 UI 刷新导致弹窗被清理或遮挡)
    LV_LOG_USER("[Ass] Calling Back_Window");
    Back_Window(&Ass_Widget.Window, "是否确认返回？");
    LV_LOG_USER("[Ass] Back_Window returned, Msgbox: %p, Yes: %p, No: %p", 
                (void*)Ass_Widget.Window.Msgbox, 
                (void*)Ass_Widget.Window.Yes_Btn, 
                (void*)Ass_Widget.Window.No_Btn);

    // 2. 停止评估相关定时器/采集
    // 注意：这里不直接调用 Ass_BackToMenu，因为 Ass_BackToMenu 会清理整个页面
    // 我们只是暂停/停止当前的评估活动，等待用户确认
    eval_stop();
    emg_force_stop_acq();
    // 停止图表定时器，防止在弹窗期间继续更新图表
    if (Ass_Widget.Chart_Timer) {
        lv_timer_pause(Ass_Widget.Chart_Timer);
    }

    // 3. 键盘导航绑定
    lv_group_t *g = app_keypad_get_group();
    if (g && Ass_Widget.Window.Yes_Btn && lv_obj_is_valid(Ass_Widget.Window.Yes_Btn)) {
        LV_LOG_USER("[Ass] Binding confirm buttons to group");
        lv_group_add_obj(g, Ass_Widget.Window.Yes_Btn);
        lv_group_add_obj(g, Ass_Widget.Window.No_Btn);
        lv_group_focus_obj(Ass_Widget.Window.Yes_Btn);
        
        // 绑定点击事件
        lv_obj_add_event_cb(Ass_Widget.Window.Yes_Btn, Ass_BackConfirm_Click_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(Ass_Widget.Window.No_Btn,  Ass_BackConfirm_Click_cb, LV_EVENT_CLICKED, NULL);
        
        // 绑定按键事件
        lv_obj_add_event_cb(Ass_Widget.Window.Yes_Btn, Ass_BackConfirm_Key_cb, LV_EVENT_KEY, NULL);
        lv_obj_add_event_cb(Ass_Widget.Window.No_Btn,  Ass_BackConfirm_Key_cb, LV_EVENT_KEY, NULL);
    } else {
        LV_LOG_WARN("[Ass] Failed to bind confirm buttons: g=%p, Yes=%p, Valid=%d", 
                    (void*)g, 
                    (void*)Ass_Widget.Window.Yes_Btn, 
                    Ass_Widget.Window.Yes_Btn ? lv_obj_is_valid(Ass_Widget.Window.Yes_Btn) : 0);
    }
}

/* 前向声明 */
static void Ass_BackConfirm_Click_cb(lv_event_t* e);
static void Ass_BackConfirm_Key_cb(lv_event_t* e);

/**
 * @brief 引导波形绘制回调：画静息基线 + 每个"用力"步骤一个三角峰
 */
// static void ass_guide_draw_cb(lv_event_t *e)
// {
//     lv_obj_t *obj = lv_event_get_target(e);
//     lv_layer_t *layer = lv_event_get_layer(e);

//     if (s_guide_phase >= g_eval.phase_cnt) return;
//     const rx_eval_phase_t *ph = &g_eval.phases[s_guide_phase];

//     /* 阶段总 tick（50ms 一点 = 20 点/秒） */
//     uint32_t total_ticks = 0;
//     for (uint8_t i = 0; i < ph->step_cnt; i++) {
//         total_ticks += (uint32_t)ph->steps[i].dur_s * 20u;
//     }
//     total_ticks *= ph->repeat;
//     if (total_ticks == 0) return;

//     lv_area_t coords;
//     lv_obj_get_coords(obj, &coords);
//     int32_t cx = coords.x1;
//     int32_t cy = coords.y1;

//     /* 静息基线：全宽横线（25% 刻度处） */
//     lv_draw_rect_dsc_t base_dsc;
//     lv_draw_rect_dsc_init(&base_dsc);
//     base_dsc.base.layer = layer;
//     base_dsc.radius = 0;
//     base_dsc.border_width = 0;
//     base_dsc.bg_color = lv_color_hex(ASS_GUIDE_COLOR);
//     base_dsc.bg_opa = LV_OPA_COVER;
//     lv_area_t base_area;
//     base_area.x1 = cx;
//     base_area.x2 = cx + 369;
//     base_area.y1 = cy + ASS_GUIDE_LOW_Y;
//     base_area.y2 = cy + ASS_GUIDE_LOW_Y + ASS_GUIDE_BAR_H - 1;
//     lv_draw_rect(layer, &base_dsc, &base_area);

//     /* 每个 TRAIN 步骤画一个三角峰：基线 → 峰顶(75%) → 基线 */
//     uint32_t tick_x = 0;
//     for (uint8_t r = 0; r < ph->repeat; r++) {
//         for (uint8_t i = 0; i < ph->step_cnt; i++) {
//             const rx_step_t *st = &ph->steps[i];
//             uint32_t step_ticks = (uint32_t)st->dur_s * 20u;
//             if (st->type == RX_STEP_TRAIN) {
//                 int32_t x0 = cx + (int32_t)((uint64_t)tick_x * 370u / total_ticks);
//                 int32_t x1 = cx + (int32_t)((uint64_t)(tick_x + step_ticks) * 370u / total_ticks);
//                 int32_t xmid = (x0 + x1) / 2;

//                 lv_draw_triangle_dsc_t tri_dsc;
//                 lv_draw_triangle_dsc_init(&tri_dsc);
//                 tri_dsc.base.layer = layer;
//                 tri_dsc.p[0].x = x0;   tri_dsc.p[0].y = cy + ASS_GUIDE_LOW_Y;   /* 基线左 */
//                 tri_dsc.p[1].x = xmid; tri_dsc.p[1].y = cy + ASS_GUIDE_HIGH_Y;  /* 峰顶 */
//                 tri_dsc.p[2].x = x1;   tri_dsc.p[2].y = cy + ASS_GUIDE_LOW_Y;   /* 基线右 */
//                 tri_dsc.color = lv_color_hex(ASS_GUIDE_COLOR);
//                 tri_dsc.opa = LV_OPA_COVER;
//                 lv_draw_triangle(layer, &tri_dsc);
//             }
//             tick_x += step_ticks;
//         }
//     }
// }


/**
 * @brief 引导波形绘制回调：画静息基线 + 每个"用力"步骤一个正弦半波
 */
static void ass_guide_draw_cb(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    lv_layer_t *layer = lv_event_get_layer(e);

    if (s_guide_phase >= g_eval.phase_cnt) return;
    const rx_eval_phase_t *ph = &g_eval.phases[s_guide_phase];

    /* 阶段总 tick */
    uint32_t total_ticks = 0;
    for (uint8_t i = 0; i < ph->step_cnt; i++) {
        total_ticks += (uint32_t)ph->steps[i].dur_s * 20u;
    }
    total_ticks *= ph->repeat;
    if (total_ticks == 0) return;

    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    int32_t cx = coords.x1;
    int32_t cy = coords.y1;

    /* 静息基线 */
    lv_draw_rect_dsc_t base_dsc;
    lv_draw_rect_dsc_init(&base_dsc);
    base_dsc.radius = 0;
    base_dsc.border_width = 0;
    base_dsc.bg_color = lv_color_hex(FONT_RED_COLOR);
    base_dsc.bg_opa = LV_OPA_COVER;
    lv_area_t base_area;
    base_area.x1 = cx;
    base_area.x2 = cx + 369;
    base_area.y1 = cy + ASS_GUIDE_LOW_Y;
    base_area.y2 = cy + ASS_GUIDE_LOW_Y + ASS_GUIDE_BAR_H - 1;
    lv_draw_rect(layer, &base_dsc, &base_area);

    /* 线段绘制描述符（复用，避免循环内反复初始化） */
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.base.layer = layer;
    line_dsc.color = lv_color_hex(FONT_RED_COLOR);
    line_dsc.opa = LV_OPA_COVER;
    line_dsc.width = 2;               // 线宽，可按需调整
    line_dsc.round_start = 0;
    line_dsc.round_end = 0;

    uint32_t tick_x = 0;
    for (uint8_t r = 0; r < ph->repeat; r++) {
        for (uint8_t i = 0; i < ph->step_cnt; i++) {
            const rx_step_t *st = &ph->steps[i];
            uint32_t step_ticks = (uint32_t)st->dur_s * 20u;

            if (st->type == RX_STEP_TRAIN && step_ticks > 0) {
                int32_t x0 = cx + (int32_t)((uint64_t)tick_x * 370u / total_ticks);
                int32_t x1 = cx + (int32_t)((uint64_t)(tick_x + step_ticks) * 370u / total_ticks);

                int32_t amp = ASS_GUIDE_HIGH_Y - ASS_GUIDE_LOW_Y;  // 振幅
                int32_t seg_count = x1 - x0;                       // 每像素一个采样点
                if (seg_count < 2) seg_count = 2;

                int32_t x_prev = x0;
                int32_t y_prev = cy + ASS_GUIDE_LOW_Y;             // 从基线开始

                for (int32_t s = 1; s <= seg_count; s++) {
                    int32_t x_cur = x0 + s;
                    if (x_cur > x1) x_cur = x1;

                    /* 相位从 0 到 π（半周期正弦，基线→峰顶→基线） */
                    float phase = (float)s / seg_count * 3.1415926f;
                    int32_t y_cur = cy + ASS_GUIDE_LOW_Y + (int32_t)(amp * sinf(phase));

                    line_dsc.p1.x = x_prev; line_dsc.p1.y = y_prev;
                    line_dsc.p2.x = x_cur;  line_dsc.p2.y = y_cur;
                    lv_draw_line(layer, &line_dsc);

                    x_prev = x_cur;
                    y_prev = y_cur;
                }
            }
            tick_x += step_ticks;
        }
    }
}
/**
 * @brief 创建评估引导波形容器（必须在曲线图之前调用，使其位于波形底层）
 * @param parent 页面容器（与曲线图同父）
 */
static void Ass_Guide_Init(lv_obj_t *parent)
{
    s_guide_cont = lv_obj_create(parent);
    lv_obj_set_pos(s_guide_cont, 45, 130);
    lv_obj_set_size(s_guide_cont, 370, 120);
    lv_obj_set_style_bg_opa(s_guide_cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_guide_cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_guide_cont, 0, 0);
    lv_obj_set_scrollbar_mode(s_guide_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(s_guide_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_add_event_cb(s_guide_cont, ass_guide_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
}

/**
 * @brief 切换评估阶段引导波形（触发重绘为对应阶段的三角峰）
 * @param phase_idx 阶段序号（0=前静息 / 1=快速收缩 / 2=持续收缩 / 3=后静息）
 */
void Ass_Guide_Update(uint8_t phase_idx)
{
    s_guide_phase = phase_idx;
    if (s_guide_cont != NULL) {
        lv_obj_invalidate(s_guide_cont);
    }
}


/**
 * @brief 评估页子页面创建入口
 * @param page_id
 */
void Ass_Page_Load(Ass_PageID_t page_id)
{
    Ass_LeaveGroup();           /* [我们] 先移出 group，防 Page_Clean 时 refocus FOCUSED 悬垂（坑 6.8） */
    Page_Clean();
    switch (page_id) {
    case AssPage1:
        PelFloAss_Page1_widget(&Ass_Widget);
        break;
    case AssPage2:
        PelFloAss_Page2_widget(&Ass_Widget);
        break;
    case AssPage3:
        PelFloAss_Page3_widget(&Ass_Widget);
        break;
    case AssPage4:
        PelFloAss_Page4_widget(&Ass_Widget);
        break;
    case AssPage5:
        PelFloAss_Page5_widget(&Ass_Widget);
        break;
    
    default:
        break;
    }
}

/**
 * @brief 盆底评估页ui设计
 * @param  none
 */
void PelFlo_Ass_ui(void)
{
    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, MAIN_BG_COLOR, 0);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(page_cont);
    lv_obj_clear_flag(page_cont, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    lv_obj_set_scrollbar_mode(page_cont, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/

    Ass_Page_Load(AssPage1);
 
}
static void page_scroll_key_cb(lv_event_t* e)
{
    Scroll_Data_t* scroll = (Scroll_Data_t*)lv_event_get_user_data(e);
    uint32_t key = lv_event_get_key(e);
    int16_t step = 30;

    if (key == LV_KEY_DOWN ) {
        ESP_LOGI(TAG, "LV_KEY_DOWN 页面当前位置：%d\r\n",  lv_obj_get_scroll_y(scroll->Obj));
        if( lv_obj_get_scroll_y(scroll->Obj) >= scroll->Origin_Y) return;
        lv_obj_scroll_by(scroll->Obj, 0, -step, LV_ANIM_ON);
        lv_event_stop_processing(e);
        
    }
    else if (key == LV_KEY_UP) {
        ESP_LOGI(TAG, "LV_KEY_UP 页面当前位置：%d\r\n",  lv_obj_get_scroll_y(scroll->Obj));
        if( lv_obj_get_scroll_y(scroll->Obj) <= scroll->END_Y) return;
        lv_obj_scroll_by(scroll->Obj, 0, step, LV_ANIM_ON);
        lv_event_stop_processing(e);
        
    }
   
}
/**
 * @brief 盆底评估第1页
 * @param  none
 */
static void PelFloAss_Page1_widget(Ass_Widget_t* widget)
{
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);

    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                 /*启用用滑动条*/
    /* 页面滚动设置*/
    Ass_Scroll.Obj = cont;
    Ass_Scroll.Origin_Y = 210;
    Ass_Scroll.END_Y = 0;
    widget->Scroll = &Ass_Scroll;
    /*说明框*/
    lv_obj_t* Obj = Create_Obj(cont, 404, 153, 0xffffff, 0);
    lv_obj_set_pos(Obj, 25, 5);
    /*添加样式*/
    lv_obj_add_style(Obj, &style_shadow, LV_PART_MAIN);      // 底层：阴影
    lv_obj_add_style(Obj, &style_gradient, LV_PART_MAIN);    // 中层：渐变
    /*标题*/
    lv_obj_t* title = Creat_Label(Obj, "盆底评估说明", basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(title, 20, 20);
    lv_obj_set_style_text_color(title, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    /*正文*/
    lv_obj_t* label = Creat_Label(Obj, "·", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(label, 20, 55);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text = Creat_TextLabel(Obj, 350, 85,
        "本次评估用时5分钟将通过肌电信号结合智能算法,"
        "检测您的盆底肌健康情况。请跟着语音和图示做盆底肌"
        "收缩、放松,全程不要用肚子、大腿用力,保证结果准确"
        "哦。", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text, 30, 50);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(text, 10, 0);          // 行间距 10px

    /*线材接线图*/
    lv_obj_t* Obj_img = Create_Obj(cont, 404, 300, 0xffffff, 0);
    lv_obj_set_pos(Obj_img, 30, 200);
    lv_obj_set_style_radius(Obj_img, 15, LV_PART_MAIN);
    lv_obj_set_style_border_width(Obj_img, 1, LV_PART_MAIN);
    lv_obj_set_style_border_opa(Obj_img, 255, LV_PART_MAIN);
    lv_obj_set_style_border_color(Obj_img, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_clear_flag(Obj_img, LV_OBJ_FLAG_SCROLLABLE);           /*禁用滑动条*/
    /*接线图*/
    lv_obj_t* Img = Creat_Image(Obj_img, "C:/Users/zqt/Desktop/PDJ_LVGL/png/page2/2_4_2.bin");
    lv_obj_set_pos(Img, 0, 0);

    /*底部模糊图层*/
    lv_obj_t* Obj2 = Create_Obj(cont, 480, 60, 0x2195f6, 0);
    lv_obj_set_pos(Obj2, -8, 225);
    lv_obj_set_style_border_width(Obj2, 0, LV_PART_MAIN);
    /* lv_obj_set_style_bg_color(Obj2, lv_color_hex(0x2195f6), LV_PART_MAIN);*/
    lv_obj_set_style_bg_opa(Obj2, 120, LV_PART_MAIN);
    lv_obj_clear_flag(Obj2, LV_OBJ_FLAG_SCROLLABLE);           /*禁用滑动条*/
    Obj_origin_y = 225;                     //按钮起始位置
    lv_obj_add_event_cb(cont, Obj_scroll_cb, LV_EVENT_SCROLL, Obj2);


    /*查看历史按钮*/
    widget->History_Btn = Create_Button(cont, 110, 40, 0xffffff, 30,0);
    lv_obj_set_pos(widget->History_Btn, 195, 200);
    lv_obj_add_style(widget->History_Btn, &style_BlueShadow, LV_PART_MAIN);
    widget->History_Btn_Label = Creat_Label(widget->History_Btn, "查看历史", basic_widget.Chinise_Font_Btn);
    lv_obj_center(widget->History_Btn_Label);
    lv_obj_set_style_text_color(widget->History_Btn_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    HisBtn_origin_y = 200;          //按钮起始位置
    lv_obj_add_event_cb(cont, HisBtn_scroll_cb, LV_EVENT_SCROLL, widget->History_Btn);  //将按钮添加到页面滚动事件
    lv_obj_add_event_cb(widget->History_Btn, AssPage1_Btn_FocEvent_cb,                  //将按钮添加获得焦点事件
        LV_EVENT_ALL, widget->History_Btn_Label);

    /*开始评估按钮*/
    widget->Ass_Btn = Create_Button(cont, 110, 40, 0xffffff, 30,0);
    lv_obj_set_pos(widget->Ass_Btn, 324, 200);
    lv_obj_add_style(widget->Ass_Btn, &style_BlueShadow, LV_PART_MAIN);
    widget->Ass_Btn_Label = Creat_Label(widget->Ass_Btn, "开始评估", basic_widget.Chinise_Font_Btn);
    lv_obj_center(widget->Ass_Btn_Label);
    lv_obj_set_style_text_color(widget->Ass_Btn_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    AssBtn_origin_y = 200;                     //按钮起始位置
    lv_obj_add_event_cb(cont, AssBtn_scroll_cb, LV_EVENT_SCROLL, widget->Ass_Btn);  //将按钮添加到页面滚动事件
    lv_obj_add_event_cb(widget->Ass_Btn, AssPage1_Btn_FocEvent_cb,                  //将按钮添加获得焦点事件
        LV_EVENT_ALL, widget->Ass_Btn_Label);

    /* ===== [我们] 注册按钮到 keypad group（左右切换 + 确认/ESC） ===== */
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        lv_group_add_obj(g, widget->History_Btn);
        lv_group_add_obj(g, widget->Ass_Btn);
        lv_group_focus_obj(widget->Ass_Btn);
    }
    /* 新增：绑定按键上下滚动 */
    lv_obj_add_event_cb(widget->History_Btn, page_scroll_key_cb, LV_EVENT_KEY, widget->Scroll);
    lv_obj_add_event_cb(widget->Ass_Btn,      page_scroll_key_cb, LV_EVENT_KEY, widget->Scroll);
}

/**
 * @brief 盆底评估第2页 评估过程
 * @param  none
 */
static void PelFloAss_Page2_widget(Ass_Widget_t* widget)
{
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);

    /*左上框*/
    lv_obj_t* Obj_left = Create_Obj(cont, 250, 90, 0xffffff, 0);
    lv_obj_set_pos(Obj_left, 15, 5);
    /*添加样式*/
    lv_obj_add_style(Obj_left, &style_shadow, LV_PART_MAIN);       // 底层：阴影
    lv_obj_add_style(Obj_left, &style_gradient, LV_PART_MAIN);     // 中层：渐变
    /*图标*/
    lv_obj_t* Img_left = Creat_Image(Obj_left, "C:/Users/zqt/Desktop/PDJ_LVGL/png/AssPage/2_1.bin");
    lv_obj_set_pos(Img_left, 10, 10);
    /*标题*/
    lv_obj_t* title1 = Creat_Label(Obj_left, "阴道", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title1, 35, 15);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    //当前评估阶段名称
    lv_obj_t* label = Creat_Label(Obj_left, "前静息期", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(label, 70, 15);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Phase_Label = label;   /* [我们] 保存阶段名控件，评估中动态更新 */

    lv_obj_t* title3 = Creat_Label(Obj_left, "最大肌电位", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title3, 25, 40);
    lv_obj_set_style_text_color(title3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Max_Label = Creat_Label(Obj_left, "50.8uV", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(widget->Max_Label, 35, 60);
    lv_obj_set_style_text_color(widget->Max_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    lv_obj_t* title4 = Creat_Label(Obj_left, "瞬时肌电位", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title4, 98, 40);
    lv_obj_set_style_text_color(title4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Instan_Label = Creat_Label(Obj_left, "50.8uV", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(widget->Instan_Label, 105, 60);
    lv_obj_set_style_text_color(widget->Instan_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    lv_obj_t* title5 = Creat_Label(Obj_left, "剩余时间", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title5, 168, 40);
    lv_obj_set_style_text_color(title5, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->RemTime_Label = Creat_Label(Obj_left, "4:30", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(widget->RemTime_Label, 170, 60);
    lv_obj_set_style_text_color(widget->RemTime_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    /*右上框*/
    lv_obj_t* Obj_rigth = Create_Obj(cont, 160, 90, 0xffffff, 0);
    lv_obj_set_pos(Obj_rigth, 280, 5);
    /*添加样式*/
    lv_obj_add_style(Obj_rigth, &style_shadow, LV_PART_MAIN);      // 底层：阴影
    lv_obj_add_style(Obj_rigth, &style_gradient, LV_PART_MAIN);    // 中层：渐变
    /*图标*/
    lv_obj_t* Img_rigth = Creat_Image(Obj_rigth, "C:/Users/zqt/Desktop/PDJ_LVGL/png/AssPage/2_2.bin");
    lv_obj_set_pos(Img_rigth, 10, 10);
    /*标题*/
    lv_obj_t* title2 = Creat_Label(Obj_rigth, "下节预览", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title2, 35, 15);
    lv_obj_set_style_text_color(title2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /* [我们] 下节动作名 */
    widget->Next_Action_Label = Creat_Label(Obj_rigth, "放松", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(widget->Next_Action_Label, 35, 42);
    lv_obj_set_style_text_color(widget->Next_Action_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_t* Obj2 = Create_Obj(Obj_rigth, 110, 3, FONT_GREEN_COLOR, 0);
    lv_obj_set_pos(Obj2, 30, 60);
    lv_obj_set_style_bg_color(Obj2, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    //曲线表
    Ass_Guide_Init(cont);                   /* [我们] 先建引导层（在图表下层，作底图） */
    // PelFloAss_Page2_Chart2(widget, cont);   /* [我们] 同事原 &widget 多取一层地址 */
    PelFloAss_Page2_Chart(widget, cont);
    Ass_Guide_Update(0);                    /* [我们] 初始前静息期：先画全程低线引导 */

    /*标题*/
    lv_obj_t* title6 = Creat_Label(cont, "腹部监控", basic_widget.Chinise_Font_Unit);
    lv_obj_set_pos(title6, 390, 105);
    lv_obj_set_style_text_color(title6, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /* [我们] 当前动作提示（随动作切换更新） */
    widget->Action_Label = Creat_Label(cont, "放松", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(widget->Action_Label, 45, 100);
    lv_obj_set_style_text_color(widget->Action_Label, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);

    /* ===== [我们] 评估中：注册 cont 承接 ESC（中止评估回菜单） ===== */
    lv_group_t *g2 = app_keypad_get_group();
    if (g2) { lv_group_add_obj(g2, cont); lv_group_focus_obj(cont); s_page_esc_obj = cont; }
    lv_obj_add_event_cb(cont, Ass_Page_Esc_cb, LV_EVENT_KEY, NULL);
}
/**
 * @brief 盆底评估第3页 生成报告中
 * @param  none
 */
static void PelFloAss_Page3_widget(Ass_Widget_t* widget)
{
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
 
    /*图标*/
    lv_obj_t* Img = Creat_Image(cont, "C:/Users/zqt/Desktop/PDJ_LVGL/png/AssPage/3_1.bin");
    lv_obj_set_pos(Img, 172, 38);
    /*标题*/
    lv_obj_t* title1 = Creat_Label(cont, "评估完成，正在生成报告，请稍候...", basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(title1, 85, 200);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_timer_t* report_timer = lv_timer_create(report_timer_cb, 100, cont);
    /*lv_timer_set_repeat_count(report_timer, 30);*/
    lv_timer_pause(report_timer);
    widget->Report_Timer = report_timer;

    /* ===== [我们] 生成报告中：注册 cont 承接 ESC（返回菜单） ===== */
    lv_group_t *g3 = app_keypad_get_group();
    if (g3) { lv_group_add_obj(g3, cont); lv_group_focus_obj(cont); s_page_esc_obj = cont; }
    lv_obj_add_event_cb(cont, Ass_Page_Esc_cb, LV_EVENT_KEY, NULL);
}
/**
 * @brief 盆底评估第4页 检测报告
 * @param  none
 */
void PelFloAss_Page4_widget(Ass_Widget_t* widget)
{
    static lv_point_precise_t line_1[] = { {98, 0},{0, 0} };
    static lv_point_precise_t line_2[] = { {0, 0},{0, 33} };
    static lv_point_precise_t line_3[] = { {0, 0},{0, 331} };
   
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                 /*启用用滑动条*/
    lv_obj_set_style_pad_all(cont, 0, 0);
    widget->Scroll = &Record_Scroll;
    Record_wdiget(widget, cont);        //数据表格

    /*标题*/
    lv_obj_t* title1 = Creat_Label(cont, "检测报告", basic_widget.Chinise_Font_Title);
    //lv_obj_set_pos(title1, 190, 0);
    lv_obj_align(title1, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    //创建返回按钮
    lv_obj_t* btn = Create_Button(cont, 64, 24, FONT_WHITE_COLOR, 20,0);
    //lv_obj_set_pos(btn, 370, 0);
    lv_obj_set_pos(btn, 392, 14);
    lv_obj_add_style(btn, &style_BlueShadow, LV_PART_MAIN);
    lv_obj_t* label2 = Creat_Label(btn, "返回", basic_widget.Chinise_Font_Btn);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_center(label2);
    basic_widget.Report_BackBtn = btn;

    /* [我们] LV_EVENT_ALL：让 ESC/确认都能触发 BackToMenu_Event_cb（同事只注册 CLICKED 导致 ESC 无效） */
    lv_obj_add_event_cb(btn, BackToMenu_Event_cb, LV_EVENT_ALL, NULL);

    /* ===== [我们] 返回按钮注册到 keypad group ===== */
    lv_group_t *g4 = app_keypad_get_group();
    if (g4) { lv_group_add_obj(g4, btn); lv_group_focus_obj(btn); }
     /* 新增：绑定按键上下滚动 */
    widget->Scroll->Origin_Y = 150;
    widget->Scroll->END_Y = 0;
    lv_obj_add_event_cb(basic_widget.Report_BackBtn, page_scroll_key_cb, LV_EVENT_KEY, widget->Scroll);
    
}
/**
 * @brief 盆底评估第5页 健康档案
 * @param  none
 */
static void PelFloAss_Page5_widget(Ass_Widget_t* widget)
{
    lv_obj_t* cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/
    
   /* Record_DataMenu_widget(widget, cont, 30);*/
    Record_DataMenu_widget(widget, cont, 30);
}

static void menu_item_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    lv_obj_t *item = lv_event_get_target(e);

    /* ========== 上下键移动焦点 ========== */
    if (key == LV_KEY_UP || key == LV_KEY_DOWN) {
        Ass_Widget_t *widget = (Ass_Widget_t *)lv_event_get_user_data(e);
        if (!widget) return;

        int total = widget->RecordData.Menu_Total_Item + 1;
        if (total <= 0) return;

        int cur = -1;
        for (int i = 0; i < total; i++) {
            if (widget->RecordData.Menu[i] == item) {
                cur = i;
                break;
            }
        }
        if (cur < 0) return;

        int next = cur + (key == LV_KEY_UP ? -1 : 1);
        lv_obj_t *target = NULL;

        if (next >= 0 && next < total) {
            target = widget->RecordData.Menu[next];
        } else {
            if (s_record_back_btn && lv_obj_is_valid(s_record_back_btn)) {
                target = s_record_back_btn;
            }
        }

        if (!target) return;

        lv_group_focus_obj(target);
        lv_obj_scroll_to_view(target, LV_ANIM_ON);
        lv_event_stop_processing(e);
    }
    /* ========== ENTER 打开报告页 ========== */
    else if (key == LV_KEY_ENTER) {
        if (item && lv_obj_is_valid(item)) {
            /* LVGL 9 接口：用 lv_obj_send_event */
            lv_obj_send_event(item, LV_EVENT_SHORT_CLICKED, NULL);
            // 如果上面没反应，改成：
            // lv_obj_send_event(item, LV_EVENT_CLICKED, NULL);
        }
        lv_event_stop_processing(e);
    }
}
static void back_btn_key_cb(lv_event_t *e)
{
    uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_UP && key != LV_KEY_DOWN) return;

    Ass_Widget_t *widget = (Ass_Widget_t *)lv_event_get_user_data(e);
    if (!widget) return;

    int total = widget->RecordData.Menu_Total_Item + 1;
    if (total <= 0) return;

    lv_obj_t *target = NULL;

    if (key == LV_KEY_UP) {
        /* 回到最后一个菜单项 */
        target = widget->RecordData.Menu[total - 1];
    } else {
        /* 回到第一个菜单项 */
        target = widget->RecordData.Menu[0];
    }

    if (target && lv_obj_is_valid(target)) {
        lv_group_focus_obj(target);
        lv_obj_scroll_to_view(target, LV_ANIM_ON);
    }

    lv_event_stop_processing(e);
}
static void record_menu_focus_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_FOCUSED) {
        lv_obj_t *obj = lv_event_get_target(e);
        lv_obj_scroll_to_view(obj, LV_ANIM_ON);
    }
}

/**
 * @brief 记录界面菜单栏设计（记录总览）
 * @param  widget 记录控件结构体
 * @param  page_cont 父页面
 * @param  amount 记录的总数量
 */

static void Record_DataMenu_widget(Ass_Widget_t* widget, lv_obj_t* page_cont, uint8_t amount)
{
    lv_obj_t* cont;
    lv_obj_t* label;
    lv_obj_t* sub_page;
    /*创建一个新的空白菜单*/
    lv_obj_t* menu = lv_menu_create(page_cont);
    lv_obj_set_size(menu, 480, 290);
    lv_obj_center(menu);
    lv_obj_set_style_bg_opa(menu, 255, LV_PART_MAIN);
    lv_obj_set_style_bg_color(menu, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_clear_flag(menu, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    lv_obj_set_scrollbar_mode(menu, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/
    //设置Header模式（底部固定返回按钮
    lv_menu_set_mode_header(menu, LV_MENU_HEADER_TOP_UNFIXED);

    widget->Datamenu = menu;

    /*获取返回按钮部件*/
    lv_obj_t* back_btn = lv_menu_get_main_header_back_button(menu);

   //标题头部样式
    lv_obj_t* hdr = lv_menu_get_main_header(menu);
    /* 背景色 + 边框 */
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x1E88E5), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(hdr, 255, LV_PART_MAIN);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    lv_obj_set_style_border_width(hdr, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(hdr, lv_color_hex(0x1565C0), LV_PART_MAIN);
    /* 固定高度（默认是自适应的，想改就设） */
    lv_obj_set_style_pad_ver(hdr, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_hor(hdr, 12, LV_PART_MAIN);
    //设置Title
    lv_obj_t* title = lv_obj_get_child(hdr, 1);         //默认：back_btn=0, title=1 或反过来
    lv_obj_set_flex_grow(title, 0);                     //关 flex grow，让 title 不被 flex 拉伸
    lv_obj_set_style_margin_left(title, 180, 0);        //用 margin 推位置(flex item 唯一能稳推的方式）
    lv_obj_set_style_text_font(title, basic_widget.Chinise_Font_Title, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(FONT_GREEN_COLOR), 0);

    //创建一个新的空白菜单页
    lv_obj_t* main_page = lv_menu_page_create(menu, "健康档案");
    lv_obj_set_style_bg_color(main_page, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_size(main_page, 480, 280);
    lv_obj_clear_flag(main_page, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/

    //创建一个空白部分 用来储存记录
    lv_obj_t* obj = lv_menu_section_create(main_page);
    lv_obj_set_size(obj, 480, 170);
    lv_obj_set_style_bg_color(obj, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    Menu_Record_Scroll.Obj = obj;
    widget->Scroll = &Menu_Record_Scroll;
    for (uint8_t i = 0; i < amount; i++) {
        char arr[50], arr2[5];

        //创建一个新的空白菜单页
        lv_obj_t* sub_page = lv_menu_page_create(menu, "检测报告");
        lv_obj_add_event_cb(sub_page, Record_Menu_event_cb, LV_EVENT_STYLE_CHANGED, (void *)(uintptr_t)i);
        //创建一个新的容器
        cont = lv_menu_cont_create(obj);
        lv_obj_set_style_height(cont, 40, LV_PART_MAIN);
        /* ===== 新增：可聚焦 + 高亮样式 ===== */
        lv_obj_add_flag(cont, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_CLICK_FOCUSABLE);
        lv_obj_set_style_bg_color(cont, lv_color_hex(FONT_GRAY_COLOR), LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(cont, lv_color_hex(0x00B0FF), LV_STATE_FOCUSED);

        /* ===== 新增：聚焦时滚动到可见 ===== */
        lv_obj_add_event_cb(cont, record_menu_focus_cb, LV_EVENT_FOCUSED, NULL);

        /* ===== 新增：菜单项上下键移动焦点 + ENTER 打开报告页 ===== */
        lv_obj_add_event_cb(cont, menu_item_key_cb, LV_EVENT_KEY, widget);
        
      
        widget->RecordData.Menu[i] = cont;
        widget->RecordData.Menu_Total_Item = i;

        //创建文本记录
        label = lv_label_create(cont);
        lv_obj_set_style_text_font(label, basic_widget.Chinise_Font_Btn, 0);
        lv_label_set_text_fmt(label, "          盆底评估                                     "
            "                         2027.%d.%d", (int)lv_rand(1, 12), (int)lv_rand(1, 31));
        lv_obj_set_style_text_color(label, lv_color_black(), LV_PART_MAIN);
        //打开新页面
        // lv_menu_set_load_page_event(menu, cont, sub_page);
    }
    //创建返回按钮
    lv_obj_t* section = lv_menu_cont_create(main_page);
    lv_obj_t* btn = Create_Button(section, 88, 33, FONT_WHITE_COLOR, 20,0);
    lv_obj_set_flex_grow(btn, 0);                 //1.关 flex grow,让 title 不被 flex 拉伸
    lv_obj_set_style_margin_left(btn, 350, 0);    //用 margin 推位置（flex item 唯一能稳推的方式)
    lv_obj_set_style_margin_top(btn, 30, 0);
    lv_obj_add_style(btn, &style_BlueShadow, LV_PART_MAIN);
    lv_obj_t* label2 = Creat_Label(btn, "返回", basic_widget.Chinise_Font_Btn);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_center(label2);
    basic_widget.Record_BackBtn = btn;
    s_record_back_btn = btn;

    /* [我们] LV_EVENT_ALL：让 ESC/确认都能触发 BackToMenu_Event_cb（同事只注册 CLICKED 导致 ESC 无效） */
    lv_obj_add_event_cb(btn, BackToMenu_Event_cb, LV_EVENT_ALL, menu);
    /* 返回按钮自身按键：UP 回最后一个菜单项，DOWN 回第一个菜单项 */
    // lv_obj_add_event_cb(btn, back_btn_key_cb, LV_EVENT_KEY, widget);
    /* ===== [我们] 返回按钮注册到 keypad group ===== */
    lv_group_t *g5 = app_keypad_get_group();
    if (g5) { 
        for(uint8_t i=0; i <= widget->RecordData.Menu_Total_Item; i++) {
            lv_group_add_obj(g5,  widget->RecordData.Menu[i]);
        }
        lv_group_add_obj(g5, btn); 
        lv_group_focus_obj(widget->RecordData.Menu[0]); 
    }
    lv_group_set_wrap(g5, true);
    // /* 新增：绑定按键上下滚动 */
    // widget->Scroll->Origin_Y = (40 * widget->RecordData.Menu_Total_Item)-80;
    // widget->Scroll->END_Y = 0;
    // lv_obj_add_event_cb(btn, page_scroll_key_cb, LV_EVENT_KEY, widget->Scroll);
    // ESP_LOGI(TAG, "页面停止位置：%d\r\n",  widget->Scroll->Origin_Y);
    /* lv_menu_set_sidebar_page(menu, main_page);*/
    lv_menu_set_page(menu, main_page);      //设置为主区域

}

Scroll_Data_t TabHeader_Obj_Scroll; //表格标题底部容器
/**
 * @brief 评估报告界面
 * @param  widget 记录控件结构体
 * @param  page_cont 父页面
 */
void Record_wdiget(Ass_Widget_t* widget, lv_obj_t* page_cont)
{

    static lv_point_precise_t line_1[] = { {98, 0},{0, 0} };
    static lv_point_precise_t line_2[] = { {0, 0},{0, 40} };
    static lv_point_precise_t line_3[] = { {0, 0},{0, 298} };
    static lv_point_precise_t line_full[] = { {440, 0},{0, 0} };

    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR,0);
    lv_obj_set_style_radius(cont, 18, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_set_style_bg_opa(cont, 255, LV_PART_MAIN);
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                 /*启动滑动*/
    lv_obj_set_style_pad_all(cont, 0, 0);

    /*表格容器：相对上一版回落3px，避免整体过度靠上；检测报告标题和返回按钮位置不动*/
    lv_obj_t* Obj_table = Create_Obj(cont, 480, 370, FONT_YELLOW_COLOR, 0);
    lv_obj_set_pos(Obj_table, -4, 77);
    lv_obj_set_style_pad_all(Obj_table, 0, 0);
    widget->Scroll->Obj = Obj_table;
    /*创建数据表格*/
    Record_EcgTable_wdiget(widget, Obj_table);
    

    /*******************表格头固定不变容器*******************/
    lv_obj_t* Obj_header = Create_Obj(cont, 480, 64, FONT_BLUE_COLOR, 1);
    lv_obj_set_pos(Obj_header, 0, 26);
    lv_obj_set_style_pad_all(Obj_header, 0, 0);
    /*表头外层透明，避免阶段名称表头与前静息期蓝块之间露出底色*/
    lv_obj_set_style_bg_opa(Obj_header, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(Obj_header, 0, LV_PART_MAIN);
    /*顶部横框蓝：纵向位置保持不变，仅整体向左微调4px*/
    lv_obj_t* Obj2 = Create_Obj(Obj_header, 440, 32, FONT_GREEN_COLOR, 1);
    /*lv_obj_set_pos(Obj2, 12, 22);*/
    lv_obj_align(Obj2, LV_ALIGN_BOTTOM_MID, -4, -1);
    lv_obj_set_style_pad_all(Obj2, 0, 0);
    lv_obj_set_style_radius(Obj2, 0, LV_PART_MAIN);
    /*表头各列严格按照数据表列宽居中*/
    lv_obj_t* title2 = Creat_Label(Obj2, "阶段名称", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title2, 98);
    lv_obj_set_pos(title2, 0, 8);
    lv_obj_set_style_text_align(title2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title2, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    lv_obj_t* title8 = Creat_Label(Obj2, "参数名称", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title8, 96);
    lv_obj_set_pos(title8, 98, 8);
    lv_obj_set_style_text_align(title8, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title8, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    lv_obj_t* title9 = Creat_Label(Obj2, "参考值", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title9, 82);
    lv_obj_set_pos(title9, 194, 8);
    lv_obj_set_style_text_align(title9, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title9, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    lv_obj_t* title10 = Creat_Label(Obj2, "测试结果", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title10, 96);
    lv_obj_set_pos(title10, 276, 8);
    lv_obj_set_style_text_align(title10, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title10, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    lv_obj_t* title11 = Creat_Label(Obj2, "得分", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title11, 68);
    lv_obj_set_pos(title11, 372, 8);
    lv_obj_set_style_text_align(title11, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_color(title11, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*横线*/
    lv_obj_t* line6 = creat_line(Obj2, 98, 1, line_1, 1);
    lv_obj_set_pos(line6, 0, 31);
    lv_obj_set_style_line_color(line6, lv_color_hex(0xffffff), LV_PART_MAIN);

    /*竖线：使用1px实体白条；粗细需要调整时修改lv_obj_set_size()的第2个参数*/
    lv_obj_t* line7 = lv_obj_create(Obj2);
    lv_obj_set_size(line7, 1, 32);
    lv_obj_set_pos(line7, 98, 0);
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
    lv_obj_set_pos(line_header10, 372, 0);
    lv_obj_set_style_bg_color(line_header10, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line_header10, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line_header10, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line_header10, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line_header10, 0, 0);
    lv_obj_clear_flag(line_header10, LV_OBJ_FLAG_SCROLLABLE);
    /*******************表格头固定不变容器*******************/

    /*底部横框蓝*/
    lv_obj_t* Obj3 = Create_Obj(Obj_table, 305, 34, FONT_GREEN_COLOR, 1);
    lv_obj_set_pos(Obj3, 98, 309);
    lv_obj_set_style_radius(Obj3, 0, LV_PART_MAIN);
    /*底部横框灰*/
    lv_obj_t* Obj4 = Create_Obj(Obj_table, 56, 35, 0xF2F4FA, 1);
    lv_obj_set_pos(Obj4, 403, 309);
    lv_obj_set_style_radius(Obj4, 0, LV_PART_MAIN);

    /*竖框蓝：方角无边框；顶部与阶段名称表头保持约1px重叠，消除白色接缝*/
    lv_obj_t* Obj1 = lv_obj_create(Obj_table);
    lv_obj_set_size(Obj1, 99, 332);
    lv_obj_set_pos(Obj1, 19, 11);
    lv_obj_set_style_bg_color(Obj1, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Obj1, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(Obj1, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(Obj1, 0, LV_PART_MAIN);
    lv_obj_set_style_border_opa(Obj1, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_outline_width(Obj1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(Obj1, 0, 0);
    lv_obj_clear_flag(Obj1, LV_OBJ_FLAG_SCROLLABLE);

    /*四个阶段分别对应右侧 2/3/2/2 行数据*/
    lv_obj_t* phase1 = lv_obj_create(Obj1);
    lv_obj_set_size(phase1, 98, 67);
    lv_obj_set_pos(phase1, 0, 0);
    lv_obj_set_style_bg_opa(phase1, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(phase1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(phase1, 0, 0);
    lv_obj_clear_flag(phase1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title3 = Creat_Label(phase1, "前静息期", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title3, 98);
    lv_obj_set_style_text_align(title3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title3, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title3, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    lv_obj_t* phase2 = lv_obj_create(Obj1);
    lv_obj_set_size(phase2, 98, 98);
    lv_obj_set_pos(phase2, 0, 67);
    lv_obj_set_style_bg_opa(phase2, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(phase2, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(phase2, 0, 0);
    lv_obj_clear_flag(phase2, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title4 = Creat_Label(phase2, "快速收缩期", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title4, 98);
    lv_obj_set_style_text_align(title4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title4, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title4, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    lv_obj_t* phase3 = lv_obj_create(Obj1);
    lv_obj_set_size(phase3, 98, 66);
    lv_obj_set_pos(phase3, 0, 165);
    lv_obj_set_style_bg_opa(phase3, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(phase3, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(phase3, 0, 0);
    lv_obj_clear_flag(phase3, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title5 = Creat_Label(phase3, "持续收缩期", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title5, 98);
    lv_obj_set_style_text_align(title5, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title5, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title5, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    lv_obj_t* phase4 = lv_obj_create(Obj1);
    lv_obj_set_size(phase4, 98, 67);
    lv_obj_set_pos(phase4, 0, 231);
    lv_obj_set_style_bg_opa(phase4, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(phase4, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(phase4, 0, 0);
    lv_obj_clear_flag(phase4, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t* title6 = Creat_Label(phase4, "后静息期", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title6, 98);
    lv_obj_set_style_text_align(title6, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title6, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title6, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);

    /*横线：阶段分界线使用1px实体白条，避免实机显示过粗*/
    lv_obj_t* line1 = lv_obj_create(Obj_table);
    lv_obj_set_size(line1, 98, 1);
    lv_obj_set_pos(line1, 20, 78);
    lv_obj_set_style_bg_color(line1, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line1, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line1, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line1, 0, 0);
    lv_obj_clear_flag(line1, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* line2 = lv_obj_create(Obj_table);
    lv_obj_set_size(line2, 440, 1);
    lv_obj_set_pos(line2, 20, 176);
    lv_obj_set_style_bg_color(line2, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line2, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line2, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line2, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line2, 0, 0);
    lv_obj_clear_flag(line2, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* line3 = lv_obj_create(Obj_table);
    lv_obj_set_size(line3, 440, 1);
    lv_obj_set_pos(line3, 20, 242);
    lv_obj_set_style_bg_color(line3, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line3, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line3, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line3, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line3, 0, 0);
    lv_obj_clear_flag(line3, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* line4 = lv_obj_create(Obj_table);
    lv_obj_set_size(line4, 440, 1);
    lv_obj_set_pos(line4, 20, 309);
    lv_obj_set_style_bg_color(line4, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line4, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line4, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line4, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line4, 0, 0);
    lv_obj_clear_flag(line4, LV_OBJ_FLAG_SCROLLABLE);

    /*竖线：使用1px实体白条；粗细需要调整时修改lv_obj_set_size()的第2个参数*/
    lv_obj_t* line_phase = lv_obj_create(Obj_table);
    lv_obj_set_size(line_phase, 1, 298);
    lv_obj_set_pos(line_phase, 118, 11);
    lv_obj_set_style_bg_color(line_phase, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line_phase, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line_phase, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line_phase, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line_phase, 0, 0);
    lv_obj_clear_flag(line_phase, LV_OBJ_FLAG_SCROLLABLE);

    /*参数名称 / 参考值*/
    lv_obj_t* line8 = lv_obj_create(Obj_table);
    lv_obj_set_size(line8, 1, 298);
    lv_obj_set_pos(line8, 214, 11);
    lv_obj_set_style_bg_color(line8, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line8, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line8, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line8, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line8, 0, 0);
    lv_obj_clear_flag(line8, LV_OBJ_FLAG_SCROLLABLE);

    /*参考值 / 测试结果*/
    lv_obj_t* line9 = lv_obj_create(Obj_table);
    lv_obj_set_size(line9, 1, 298);
    lv_obj_set_pos(line9, 296, 11);
    lv_obj_set_style_bg_color(line9, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line9, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line9, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line9, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line9, 0, 0);
    lv_obj_clear_flag(line9, LV_OBJ_FLAG_SCROLLABLE);

    /*测试结果 / 得分*/
    lv_obj_t* line10 = lv_obj_create(Obj_table);
    lv_obj_set_size(line10, 1, 298);
    lv_obj_set_pos(line10, 392, 11);
    lv_obj_set_style_bg_color(line10, lv_color_hex(0xffffff), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line10, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line10, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line10, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line10, 0, 0);
    lv_obj_clear_flag(line10, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* title7 = Creat_Label(Obj3, "总得分", basic_widget.Chinise_Font_Btn);
    lv_obj_set_width(title7, 305);
    lv_obj_set_style_text_align(title7, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title7, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(title7, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    /* [我们] 总得分值（相对标定 0~100） */
    lv_obj_t* score_label = Creat_Label(cont, "0 分", basic_widget.Chinise_Font_Btn);
    lv_obj_set_pos(score_label, 290, 363);
    lv_obj_set_style_text_color(score_label, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    lv_label_set_text_fmt(score_label, "%d分", eval_get_result()->total);   /* 字库无空格，紧排 */

    /*横线*/
    lv_obj_t* line = lv_obj_create(cont);
    lv_obj_set_size(line, 100, 1);
    lv_obj_set_pos(line, 17, 89);
    lv_obj_set_style_bg_color(line, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(line, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(line, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(line, 0, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);

    // lv_obj_t* Obj10 = Create_Obj(cont, 56, 35, 0xF2F4FA, 1);
    // lv_obj_set_pos(Obj10, 20, 50);
    // lv_obj_set_style_radius(Obj10, 0, LV_PART_MAIN);

}

/**
 * @brief 评估报告表格
 * @param  widget 记录控件结构体
 * @param  page_cont 父页面
 */
void Record_EcgTable_wdiget(Ass_Widget_t* widget, lv_obj_t* page_cont)
{
    char* text[] = {"平均值","变异性","最大值","上升时间","恢复时间","平均值","变异性","平均值","变异性" };

    /*创建表格样式*/
    static lv_style_t style_table_bg;
    lv_style_init(&style_table_bg);
    lv_style_set_bg_opa(&style_table_bg, LV_OPA_COVER);
    lv_style_set_bg_color(&style_table_bg, lv_color_hex(0xffffff));
    lv_style_set_text_color(&style_table_bg, lv_color_white());
    lv_style_set_border_opa(&style_table_bg, 0);
    /*创建基本部件：参数名称列左边界与表头第二列完全对齐*/
    lv_obj_t* obj = Create_Obj(page_cont, 346, 340, 0x6483CD, 1);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN);
    /* 数据表恢复到上一版较合适的纵向位置；横向位置保持不变 */
    lv_obj_set_pos(obj, 112, -33);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    /*创建表格*/
    lv_obj_t* table = lv_table_create(obj);
    lv_obj_set_size(table, 342, 303);
    lv_obj_add_style(table, &style_table_bg, LV_PART_ITEMS);
    lv_obj_add_style(table, &style_table_bg, LV_PART_MAIN);
    /*lv_obj_align_to(table, table_header, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);*/
    lv_obj_set_pos(table, -8, 22);

    lv_obj_set_style_pad_ver(table, 9, LV_PART_ITEMS);
    lv_obj_set_style_pad_hor(table, 0, LV_PART_ITEMS);
    lv_obj_set_style_border_width(table, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(table, 0, LV_PART_ITEMS);
    lv_obj_set_scrollbar_mode(table, LV_SCROLLBAR_MODE_ON); // 开启滚动条
    lv_obj_set_scroll_dir(table, LV_DIR_VER);
    
    /* [我们] 填真实评估结果（相对标定：参考值列无固定参考，填 "-"） */
    const eval_result_t *r = eval_get_result();

    for (uint8_t i = 0; i < 9; i++) {
        lv_table_set_cell_value_fmt(table, i, 0, "%s", text[i]);
        lv_table_set_cell_value(table, i, 1, "");   /* 相对标定无固定参考，留空（"-"/"无" 在 Btn 字库均为方框） */
    }
    /* 前静息期 */
    lv_table_set_cell_value_fmt(table, 0, 2, "%.1fuV", r->rest1_avg);
    lv_table_set_cell_value_fmt(table, 0, 3, "%d", r->score_rest);
    lv_table_set_cell_value_fmt(table, 1, 2, "%.1f", r->rest1_var);
    lv_table_set_cell_value_fmt(table, 1, 3, "%d", r->score_rest);
    /* 快速收缩期 */
    lv_table_set_cell_value_fmt(table, 2, 2, "%.1fuV·%.1fx", r->fast_max, r->fast_max_ratio);
    lv_table_set_cell_value_fmt(table, 2, 3, "%d", r->score_force);
    lv_table_set_cell_value_fmt(table, 3, 2, "%.2fs", r->fast_rise);
    lv_table_set_cell_value_fmt(table, 3, 3, "%d", r->score_rise);
    lv_table_set_cell_value_fmt(table, 4, 2, "%.2fs", r->fast_recover);
    lv_table_set_cell_value_fmt(table, 4, 3, "%d", r->score_recover);
    /* 持续收缩期 */
    lv_table_set_cell_value_fmt(table, 5, 2, "%.1fuV·%.1fx", r->sust_avg, r->sust_ratio);
    lv_table_set_cell_value_fmt(table, 5, 3, "%d", r->score_endu);
    lv_table_set_cell_value_fmt(table, 6, 2, "%.1f", r->sust_var);
    lv_table_set_cell_value_fmt(table, 6, 3, "%d", r->score_sust);
    /* 后静息期 */
    lv_table_set_cell_value_fmt(table, 7, 2, "%.1fuV", r->rest2_avg);
    lv_table_set_cell_value_fmt(table, 7, 3, "%d", r->score_rest);
    lv_table_set_cell_value_fmt(table, 8, 2, "%.1f", r->rest2_var);
    lv_table_set_cell_value_fmt(table, 8, 3, "%d", r->score_rest);
    lv_table_set_column_count(table, 4);        //设置4列
    lv_table_set_row_count(table, 9);           //设置9行

    /*四列宽度与上方表头完全一致*/
    lv_table_set_column_width(table, 0, 96);
    lv_table_set_column_width(table, 1, 82);
    lv_table_set_column_width(table, 2, 96);
    lv_table_set_column_width(table, 3, 68);
    
    
    Set_Chinese_Font(table, basic_widget.Chinise_Font_Btn);
    lv_obj_set_style_text_align(table, LV_TEXT_ALIGN_CENTER, LV_PART_ITEMS);
    lv_obj_set_style_text_color(table, lv_color_hex(FONT_GRAY_COLOR), LV_PART_ITEMS);

    /*lv_table_set_row_count(table, 20);*/
    /* lv_obj_set_size(table, LV_SIZE_CONTENT, LV_SIZE_CONTENT);*/
    lv_obj_add_event_cb(table, Draw_EcgTable_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);
    /* lv_obj_add_event_cb(table, table_event_handler, LV_EVENT_SCROLL, NULL);*/
    lv_obj_add_flag(table, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);

}


/* 全局/静态变量，保存状态 */
uint32_t g_write_idx = 0;
lv_chart_cursor_t* cur = 0;
lv_chart_series_t* ser_green;
lv_chart_series_t* ser_red;
#define SER_ID_FILL     0x01  // 填充线标识
#define SER_ID_NO_FILL  0x02  // 非填充线标识
/**
 * @brief 盆底评估第2页的绿色曲线图
 * @param  none
 */
static void PelFloAss_Page2_Chart(Ass_Widget_t* widget, lv_obj_t* page_cont)
{
    uint16_t i;
     /*样式：透明背景 只留边框*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 1);
    lv_style_set_bg_opa(&style, 0);
    lv_style_set_border_width(&style, 0);
    /*lv_style_set_border_color(&style, lv_color_hex(MAIN_CHART_OBJ_BG_COLOR));*/

    draw_pressure_excel(page_cont);     //绘制背景虚线

    /*创建曲线表*/
    lv_obj_t* chart = lv_chart_create(page_cont);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);   /*Show lines and points too*/
    lv_obj_set_pos(chart, 45, 130);
    lv_obj_set_size(chart, 370, 120);
    lv_obj_add_style(chart, &style, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_set_style_radius(chart, 0, 0);
    lv_obj_set_style_pad_bottom(chart, 0, 0);
    lv_chart_set_div_line_count(chart, 0, 0);
    /* [我们] 阶段式显示：点数列由评估引擎每阶段动态设置（Aemg_add_data 里 set_point_count），
     *       每阶段清空重画、非滚动（点数 = 阶段秒×20；初始=前静息 20s×20=400，首个 tick 覆盖） */
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 3000);   /* µV, 覆盖收缩~3mV */
    lv_chart_set_point_count(chart, 400);   /* 初始=前静息 20s×20=400 点 */
    
    /*设置折线线宽 设置线条端点为圆形*/
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);  // 3px 宽
    lv_obj_set_style_line_rounded(chart, true, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_INDICATOR);

    /*series 1:绿线*/
   ser_green = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_LIGHT_GREEN), LV_CHART_AXIS_PRIMARY_Y);
  
    lv_obj_add_flag(chart, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(chart, Mydraw_event_cb, LV_EVENT_DRAW_TASK_ADDED, ser_green);
    widget->Chart = chart;

    lv_timer_t* timer = lv_timer_create(Aemg_add_data, 50, chart);   /* 50ms 采样, 对齐力度引擎更新 */
    widget->Chart_Timer = timer;
    lv_timer_pause(widget->Chart_Timer);
}
/**
 * @brief 盆底评估第2页的红色曲线图
 * @param  none
 */
static void PelFloAss_Page2_Chart2(Ass_Widget_t* widget, lv_obj_t* page_cont)
{
    uint16_t i;
     /*样式：透明背景 只留边框*/
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_radius(&style, 1);
    lv_style_set_bg_opa(&style, 0);
    lv_style_set_border_width(&style, 0);
    /*lv_style_set_border_color(&style, lv_color_hex(MAIN_CHART_OBJ_BG_COLOR));*/
    draw_pressure_excel(page_cont);     //绘制背景虚线
    /*创建曲线表*/
    lv_obj_t* chart = lv_chart_create(page_cont);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);   /*Show lines and points too*/
    lv_obj_set_pos(chart, 45, 130);
    lv_obj_set_size(chart, 370, 120);
    lv_obj_add_style(chart, &style, LV_PART_MAIN);
    lv_obj_set_style_pad_all(chart, 0, 0);
    lv_obj_set_style_radius(chart, 0, 0);
    lv_obj_set_style_pad_bottom(chart, 0, 0);
    lv_chart_set_div_line_count(chart, 0, 0);
    /*CIRCULAR 模式*/
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 3000);   /* µV, 覆盖收缩~3mV */
    lv_chart_set_point_count(chart, 1000);

    /*设置折线线宽 设置线条端点为圆形*/
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);  // 3px 宽
    lv_obj_set_style_line_rounded(chart, true, LV_PART_ITEMS);
    lv_obj_set_style_bg_opa(chart, 0, LV_PART_INDICATOR);

    /*series 2:红线*/
    ser_red = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    for (uint16_t i = 0; i < 1000; i++) {
        lv_chart_set_next_value(chart, ser_red, ecg_sample[i]); // 中间值
    }
    //for (uint16_t i = 0; i < 200; i++) {
    //    lv_chart_set_next_value(chart, ser_red, 0); // 中间值
    //}
    //for (uint16_t i = 200; i < 400; i++) {
    //    lv_chart_set_next_value(chart, ser_red, 25); // 中间值
    //}
    //for (uint16_t i = 400; i < 600; i++) {
    //    lv_chart_set_next_value(chart, ser_red, 50); // 中间值
    //}
    //for (uint16_t i = 600; i < 800; i++) {
    //    lv_chart_set_next_value(chart, ser_red, 75); // 中间值
    //}
    //for (uint16_t i = 800; i < 1000; i++) {
    //    lv_chart_set_next_value(chart, ser_red, 100); // 中间值
    //}

   /* lv_obj_add_flag(chart, LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
    lv_obj_add_event_cb(chart, Mydraw2_event_cb, LV_EVENT_DRAW_TASK_ADDED, NULL);*/
}




/**
 * @brief 创建曲线表格
 * @param cont
 */
static void draw_pressure_excel(lv_obj_t* cont)
{

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
