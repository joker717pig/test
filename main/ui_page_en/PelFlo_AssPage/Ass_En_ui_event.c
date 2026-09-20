/**
  ******************************************************************************
  * @文件名称   Ass_En_ui_event.c
  * @文件描述   英文盆底评估页事件回调（仅跳转相关的 4 个，其余语言无关的中文
  *            回调在 PelFlo_Ass_ui_event.c 直接复用）。逻辑与中文一致，仅把
  *            Ass_Page_Load 换成英文 Ass_En_Page_Load、记录表格换英文版。
  ******************************************************************************
  */

#include "Ass_En_ui_event.h"
#include "Ass_En_ui.h"
#include "PelFlo_Ass_ui.h"
#include <string.h>
#include "Frame_enent.h"
#include "basic.h"
#include "app_keypad.h"   /* keypad group */
#include "emg_force.h"
#include "emg_dsp.h"
#include "therapy_eval.h"

#define FILTER_WIN 7  /* 滑动窗口大小（与中文评估一致） */

static int32_t filter_buf[FILTER_WIN] = { 0 };
static uint8_t filter_idx = 0;
static eval_state_t s_plot_phase = EVAL_IDLE;
static uint16_t s_plot_idx = 0;

static int32_t smooth_value(int32_t new_val);

void Ass_En_Aemg_add_data_real(lv_timer_t* t)
{
    lv_obj_t *chart = lv_timer_get_user_data(t);
    lv_chart_series_t *ser = lv_chart_get_series_next(chart, NULL);
    if (!ser) return;
    int32_t env_uv = (int32_t)(emg_force_get_env() * EMG_DSP_RAW16_LSB_UV);
    eval_state_t st = eval_get_state();
    const char *phase = "Ready";
    if (st == EVAL_REST1) {
        phase = "Resting phase 1";
    }
    else if (st == EVAL_FAST){
        phase = "Fast contraction phase";
    } 
    else if (st == EVAL_SUST){
        phase = "Sustained contraction phase";
    } 
    else if (st == EVAL_REST2){
        phase = "Resting phase 2";
    } 
    else if (st == EVAL_DONE){
        phase = "Completed";
    } 
    if (Ass_Widget.Phase_Label) {
        lv_label_set_text(Ass_Widget.Phase_Label, phase);
    } 
    rx_train_kind_t action_kind = eval_action_kind();
    rx_train_kind_t next_kind = eval_next_action_kind();
    const char *action = (action_kind == RX_TRAIN_HOLD) ? "Hold contraction" :
                         (action_kind == RX_TRAIN_CONTRACT) ? "Contract" : "Relax";
    const char *next = (next_kind == RX_TRAIN_HOLD) ? "Hold contraction" :
                       (next_kind == RX_TRAIN_CONTRACT) ? "Contract" : "Relax";
    if (Ass_Widget.Instan_Label) {
        lv_label_set_text_fmt(Ass_Widget.Instan_Label, "%d", (int)env_uv);
    } 
    if (Ass_Widget.Action_Label){
        lv_label_set_text(Ass_Widget.Action_Label, action);
    } 
    if (Ass_Widget.Next_Action_Label) {
        lv_label_set_text(Ass_Widget.Next_Action_Label, next);
    } 
    if (Ass_Widget.RemTime_Label) { 
        uint16_t rem = eval_total_remain_s(); 
        lv_label_set_text_fmt(Ass_Widget.RemTime_Label, "%d:%02d", rem / 60, rem % 60); 
    }
    if (st != s_plot_phase) {
        s_plot_phase = st;
        if (st >= EVAL_REST1 && st <= EVAL_REST2) {
            ass_en_guide_update((uint8_t)(st - EVAL_REST1));
            uint16_t n = eval_phase_total_s() * 20u; if (n < 2) n = 2;
            lv_chart_set_point_count(chart, n);
            lv_chart_set_all_value(chart, ser, LV_CHART_POINT_NONE);
            s_plot_idx = 0;
            memset(filter_buf, 0, sizeof(filter_buf)); filter_idx = 0;
        }
    }
    if (st == EVAL_DONE) {
        emg_force_stop_acq();
        if (Ass_Widget.Chart_Timer) {
            lv_timer_del(Ass_Widget.Chart_Timer); 
            Ass_Widget.Chart_Timer = NULL; 
        }
        Ass_En_Page_Load(AssPage3);
        if (Ass_Widget.Report_Timer) {
            lv_timer_resume(Ass_Widget.Report_Timer);
        } 
        return;
    }
    int32_t value = smooth_value(env_uv);
    uint16_t idx = eval_phase_elapsed_ticks();
    uint16_t max_idx = lv_chart_get_point_count(chart);
    if (!max_idx) max_idx = 1;
    if (idx >= max_idx) idx = max_idx - 1;
    for (uint16_t i = s_plot_idx; i <= idx; i++) {
        lv_chart_set_value_by_id(chart, ser, i, value);
    }
    s_plot_idx = idx + 1;
}

/**
 * @brief 英文盆底评估第1页按钮焦点/导航/点击回调
 */
void Ass_En_Page1_Btn_FocEvent_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_FOCUSED) {
        if (obj == Ass_Widget.History_Btn) {
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x68D0C3), LV_PART_MAIN);
            lv_obj_set_style_bg_color(Ass_Widget.Ass_Btn, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_set_style_text_color(Ass_Widget.Ass_Btn_Label, lv_color_hex(0x68D0C3), LV_PART_MAIN);
            lv_obj_set_style_text_color(Ass_Widget.History_Btn_Label, lv_color_hex(0xffffff), LV_PART_MAIN);
        }
        else if (obj == Ass_Widget.Ass_Btn) {
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x68D0C3), LV_PART_MAIN);
            lv_obj_set_style_bg_color(Ass_Widget.History_Btn, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_set_style_text_color(Ass_Widget.Ass_Btn_Label, lv_color_hex(0xffffff), LV_PART_MAIN);
            lv_obj_set_style_text_color(Ass_Widget.History_Btn_Label, lv_color_hex(0x68D0C3), LV_PART_MAIN);
        }
    }
    else if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_LEFT && obj == Ass_Widget.Ass_Btn) {
            lv_group_focus_obj(Ass_Widget.History_Btn);
        }
        else if (key == LV_KEY_RIGHT && obj == Ass_Widget.History_Btn) {
            lv_group_focus_obj(Ass_Widget.Ass_Btn);
        }
        else if (key == LV_KEY_ESC) {
            Ass_ShowBackConfirm();   /* 复用中文（显示确认弹窗） */
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
        if (obj == Ass_Widget.History_Btn) {
            // Ass_En_Page_Load(AssPage5);         /* 创建健康档案（英文） */
            return;
        }
        else if (obj == Ass_Widget.Ass_Btn) {
            emg_force_reset();
            emg_force_start_acq(500, 1);
            if (eval_start() != ESP_OK) {
                emg_force_stop_acq();
                return;
            }
            s_plot_phase = EVAL_IDLE;
            s_plot_idx = 0;
            Ass_En_Page_Load(AssPage2);         /* 创建开始评估页（英文） */
            if (Ass_Widget.Chart_Timer != NULL) {
                lv_timer_resume(Ass_Widget.Chart_Timer);
            }
            else {
                LV_LOG_WARN("Trying to resume Chart_Timer a NULL timer!");
            }
            return;
        }
    }
}

/**
 * @brief 英文报告生成定时器回调（跳转英文检测报告页）
 */
void Ass_En_report_timer_cb(lv_timer_t* t)
{
    (void)t;
    static uint8_t count = 0;
    LV_LOG_USER("report_timer_cb(EN):%d", count);
    count++;
    if (count >= 30) {
        count = 0;
        Ass_En_Page_Load(AssPage4);             /* 创建英文检测报告页 */
        lv_timer_pause(Ass_Widget.Report_Timer);
    }
}

/**
 * @brief 英文曲线定时器回调（评估完成跳转英文生成报告页）
 */
void Ass_En_Aemg_add_data(lv_timer_t* t)
{
    static int32_t y = 0;
    lv_obj_t* chart = lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(chart, NULL);

    uint16_t p = lv_chart_get_point_count(chart);
    uint16_t s = lv_chart_get_x_start_point(chart, ser);
    int32_t* a = lv_chart_get_y_array(chart, ser);

    if (y >= 500) {
        y = 0;
        /* [我们] 评估完成：Chart_Timer 无用，直接删除防悬垂/泄漏 */
        if (Ass_Widget.Chart_Timer) { lv_timer_del(Ass_Widget.Chart_Timer); Ass_Widget.Chart_Timer = NULL; }
        Ass_En_Page_Load(AssPage3);             /* 创建英文生成报告页 */
        lv_timer_resume(Ass_Widget.Report_Timer);
        return;
    }

    lv_label_set_text_fmt(Ass_Widget.Instan_Label, "%d", (int)ecg_sample2[y]);
    int32_t smoothed = smooth_value(ecg_sample[y]);
    lv_chart_set_next_value(chart, ser, smoothed);
    y++;

    g_write_idx = (g_write_idx + 1) % CHART_N;

    a[(s + 1) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;
    a[(s + 3) % p] = LV_CHART_POINT_NONE;
    a[(s + 4) % p] = LV_CHART_POINT_NONE;
    a[(s + 5) % p] = LV_CHART_POINT_NONE;
}

/**
 * @brief 英文健康档案记录项回调（生成英文记录表格）
 */
void Ass_En_Record_Menu_event_cb(lv_event_t* e)
{
    lv_obj_t* target = lv_event_get_target(e);

    lv_obj_t* obj = lv_menu_section_create(target);
    lv_obj_set_style_bg_color(obj, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_size(obj, 480, 300);
    lv_obj_set_flex_grow(obj, 0);
    lv_obj_set_style_margin_top(obj, -25, 0);
    Record_En_wdiget(&Ass_Widget, obj);
}

static int32_t smooth_value(int32_t new_val)
{
    filter_buf[filter_idx] = new_val;
    filter_idx = (filter_idx + 1) % FILTER_WIN;

    int32_t sum = 0;
    for (uint8_t i = 0; i < FILTER_WIN; i++) {
        sum += filter_buf[i];
    }
    return sum / FILTER_WIN;
}
