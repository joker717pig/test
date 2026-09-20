#include "PelFlo_TheChild_En_event.h"
#include "Child_ThePage_En.h"   /* The_En_Param_Data1 */
#include "Child_MaiPage_En.h"
#include "Chlid_DysPage_En.h"
#include "Chlid_KgePage_En.h"
#include "PelFlo_The_En_ui.h"
#include "shared_En_ui.h"       /* Warn_En_Window / Back_En_Window */
#include "app_therapy.h"        /* [我们] 阶段C：治疗控制 */
#include "menu_ui.h"
#include "menu_ui_event_cb.h"   /* [我们] 复用中文语言无关：Back_Btn_Event_cb / Chart_Draw_event_cb */
#include "basic.h"
#include "app_keypad.h"         /* [我们] app_keypad_get_group（曲线图确认框键盘化） */
#include "esp_log.h"
#include "emg_force.h"
#include "emg_dsp.h"
#include "voice.h"
#include <string.h>

static const char *TAG = "TheChildEn_evt";

#define FILTER_WIN 7

static int32_t filter_buf[FILTER_WIN] = { 0 };
static uint8_t filter_idx = 0;

/* [我们] static 前向声明 */
static uint8_t Get_StageBtn(Select_Data_t* data, lv_obj_t* btn);
static void Set_StageBtn_UnSel(Select_Data_t* data);
static void timer_next_cb(lv_timer_t* t);
static void Trest_Widget_timer_cb(lv_timer_t* t);
static void Timer_BackToMenu_En_Event_cb(lv_timer_t* t);
static void Chart_MsgBox_Key_cb(lv_event_t* e);
static void Chart_ShowConfirm_Key_En(Chart_Data_t* data);
static void Trest_En_ShowCompleted(Treat_Timer_Ctx_t* ctx);

/* 治疗完成展示（隐藏开始/暂停，返回按钮放大居中，Label 文案/字体对齐客户定稿） */
static void Trest_En_ShowCompleted(Treat_Timer_Ctx_t* ctx)
{
    lv_obj_add_flag(ctx->treat_data.Start_Btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ctx->treat_data.Pause_Btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_pos(ctx->treat_data.Back_Btn, 150, 220);
    lv_obj_set_size(ctx->treat_data.Back_Btn, 162, 40);
    lv_label_set_text(ctx->treat_data.Label_RemTime, "Completed");
    lv_obj_set_style_text_font(ctx->treat_data.Label_RemTime, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_obj_set_width(ctx->treat_data.Label_RemTime, 180);
    lv_obj_set_style_text_align(ctx->treat_data.Label_RemTime, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(ctx->treat_data.Label_RemTime, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_text_color(ctx->treat_data.Label_RemTime, lv_color_hex(FONT_RED_COLOR), LV_PART_MAIN);
    if (Get_CurStage_Idx() != STAGE_1) {
        therapy_pause();
        audio_play_request(VOICE_ID_NEXT_EN, 1500);
        lv_label_set_text(ctx->treat_data.Label_Hint, "Entering next session");
        lv_obj_set_style_text_font(ctx->treat_data.Label_Hint, &lv_font_montserrat_12, LV_PART_MAIN);
        lv_obj_align(ctx->treat_data.Label_Hint, LV_ALIGN_CENTER, 0, 18);
        if (ctx->timer_Next == NULL) {
            ctx->timer_Next = lv_timer_create(timer_next_cb, 20, ctx);
        }
    }
    else {
        lv_label_set_text(ctx->treat_data.Label_Hint, "Great~");
        therapy_pause();
        audio_play_request(VOICE_ID_DONE_EN, 1500);
        lv_obj_set_style_text_font(ctx->treat_data.Label_Hint, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_width(ctx->treat_data.Label_Hint, 100);
        lv_obj_set_style_text_align(ctx->treat_data.Label_Hint, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(ctx->treat_data.Label_Hint, LV_ALIGN_CENTER, 0, 18);
        lv_obj_set_style_text_color(ctx->treat_data.Label_Hint, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    }
}

/**
 * @brief 电刺激治疗页定时器回调（英文版；保留 esp32 治疗引擎驱动倒计时渲染）
 */
static void Trest_Widget_timer_cb(lv_timer_t* t)
{
    Treat_Timer_Ctx_t* ctx = lv_timer_get_user_data(t);

    if (therapy_active() || ctx->status == TIMER_END) {
        lv_label_set_text_fmt(ctx->treat_data.Label_RemTime, "%02d:%02d:%02d",
            (int)(ctx->remain_sec / 60000),
            (int)((ctx->remain_sec % 60000) / 1000),
            (int)((ctx->remain_sec % 1000) / 10));
        if (ctx->treat_data.Label_Intens) {
            lv_label_set_text_fmt(ctx->treat_data.Label_Intens, "%d", (int)ctx->treat_data.Intens_Value);
        }
        if (therapy_active()) {
            if (ctx->treat_data.Label_Pulse)
                lv_label_set_text_fmt(ctx->treat_data.Label_Pulse, "%duS", (int)therapy_current_pw());
            if (ctx->treat_data.Label_Freq)
                lv_label_set_text_fmt(ctx->treat_data.Label_Freq, "%dHz", (int)therapy_current_freq());
            if (ctx->treat_data.Label_stage_stime)
                lv_label_set_text_fmt(ctx->treat_data.Label_stage_stime, "Current Phase:%d", (int)therapy_current_stage_stim());
        }
        if (ctx->status == TIMER_END) {
            Trest_En_ShowCompleted(ctx);
            if (ctx->timer_Treat) {
                lv_timer_del(ctx->timer_Treat);
                ctx->timer_Treat = NULL; 
            }
        }
        return;
    }

}

/**
 * @brief 治疗页按钮(开始、暂停、返回)事件回调（英文版）
 */
void ChildPage_P5Btn_En_Event_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        /* Start */
        if (btn == ctx->treat_data.Start_Btn) {
            ctx->status = TIMER_RUNNING;
            ctx->last_tick = lv_tick_get();
            if (ctx->timer_Treat == NULL) {
                ctx->timer_Treat =lv_timer_create(Trest_Widget_timer_cb, 1, ctx);
            }
            lv_obj_set_style_text_color(ctx->treat_data.Label_RemTime, lv_color_hex(ctx->treat_data.Color), LV_PART_MAIN); 
            int ts = therapy_status();
            if (ts == TIMER_PAUSED) {
                audio_play_request(VOICE_ID_START_EN, 1500);
                therapy_resume();
            }
            else if (ts != TIMER_RUNNING) {
                uint8_t schemes_idx = Get_CurTreat_Idx();
                /* 盆底肌治疗 */
                if (Get_CurPage_Idx() == ThePage) {
                    therapy_start(
                        &g_rx_all[RX_3]->schemes[schemes_idx],
                        (int)The_En_Param_Data1.Value_Intens1,
                        (int)The_En_Param_Data1.Value_Intens2,
                        ctx);
                }

                /* 盆底肌保养 */
                else if (Get_CurPage_Idx() == MaiPage) {
                    therapy_start(
                        &g_rx_all[RX_2]->schemes[schemes_idx],
                        (int)Mai_En_Param_Data1.Value_Intens1,
                        (int)Mai_En_Param_Data1.Value_Intens2,
                        ctx );
                }
                else if (Get_CurPage_Idx() == DysPage) {
                   therapy_start(
                   &g_rx_all[RX_1]->schemes[schemes_idx],
                   (int)Dys_En_Param_Data1.Value_Intens1,
                   (int)Dys_En_Param_Data1.Value_Intens2,
                   ctx);
                }
            }
        }

        /* Pause */
        else if (btn == ctx->treat_data.Pause_Btn) {
            ctx->status = TIMER_PAUSED;
            lv_obj_set_style_text_color(ctx->treat_data.Label_RemTime,lv_color_hex(FONT_GRAY_COLOR),LV_PART_MAIN);
            therapy_pause();
            audio_play_request(VOICE_ID_PAUSE_EN, 1500);
        }
        /* Back */
        else if (btn == ctx->treat_data.Back_Btn) {
            ctx->status = TIMER_PAUSED;
            therapy_pause();
            if (ctx->remain_sec == 0) {
                ctx->remain_sec = ctx->total_sec;
                ThePage_ID_t page_idx = Get_CurPage_Idx();
                switch (page_idx) {
                case ThePage:
                    The_En_BackToMenu();
                    break;
                case MaiPage:
                    Mai_En_BackToMenu();
                    break;
                case DysPage:
                    Dys_En_BackToMenu();
                    break;
                case KgePage:
                    Kge_En_BackToMenu();
                    break;
                default:
                    Goto_MenuPage();
                    break;
                }
                return;
            }
            else {
                The_En_Page5_ShowConfirm(ctx);
            }
        }

        lv_label_set_text_fmt(ctx->treat_data.Label_Intens,"%d",(int)ctx->treat_data.Intens_Value);
    }
}
/**
 * @brief 参数页设置按钮点击事件回调（英文版；Value_Stage 逻辑对齐客户定稿）
 */
void ChildPage_P4SetBtn_En_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        if (data->Set_Btn == obj) {
            if (strcmp(lv_label_get_text(data->Label_Set),"Setup Complete, Next") == 0) {
                data->Value_Stage = 2;
                data->Value_Intens2 = 0;
                lv_label_set_text(data->Label_Stage, "2/2");
                lv_label_set_text(data->Label_Set, "Setup Complete, Start");
                
               /*
                * 当前只处理盆底肌治疗 ThePage。
                * 切换到参数 2/2 时，从 RX_3 当前疗程读取第二组刺激参数。
                */
                uint8_t schemes_idx = Get_CurTreat_Idx();
                ThePage_ID_t page_idx = Get_CurPage_Idx();
                uint8_t rx_idx = 0;
                switch(page_idx){
                    case ThePage: rx_idx = RX_3; break;
                    case MaiPage: rx_idx = RX_2; break;
                    case DysPage: rx_idx = RX_1; break;
                    case KgePage: rx_idx = RX_4; break;
                    default: break;
                }
                data->Value_Freq = therapy_cur_freq(&g_rx_all[rx_idx]->schemes[schemes_idx], 2);
                data->Value_Pulse = therapy_cur_pw(&g_rx_all[rx_idx]->schemes[schemes_idx], 2);
                lv_label_set_text_fmt(data->Label_Freq, "%dHz",(int)data->Value_Freq);
                lv_label_set_text_fmt(data->Label_Pulse,"%duS", (int)data->Value_Pulse);
                /*强度同步切换到第二组*/
                lv_slider_set_value(data->Slider, 0, LV_ANIM_OFF);          //初始化滑动条值为0
                lv_label_set_text(data->Label_Intens, "0");
                therapy_send_intensity(0,data->Value_Freq,data->Value_Pulse);
                lv_group_t* g = app_keypad_get_group();
                if (g && lv_obj_is_valid(data->Plu_Btn)) {
                    lv_group_focus_obj(data->Plu_Btn);
                }
            }
            else if (strcmp(lv_label_get_text(data->Label_Set), "Setup Complete, Start") == 0) {
                therapy_send_intensity(0,data->Value_Freq,data->Value_Pulse);
                ThePage_ID_t page_idx = Get_CurPage_Idx();
                switch (page_idx) {
                case ThePage: Child_ThePage_En_Load(ChiThePage_Treat);  break;
                case MaiPage: Child_MaiPage_En_Load(ChiMaiPage_Treat);  break;
                case DysPage: Child_DysPage_En_Load(ChiDysPage_Treat);  break;
                case KgePage: Child_KgePage_En_Load(ChiKgePage_Treat);  break;
                default: break;
                }
            }
        }
    }
}

/**
 * @brief 阶段疗程说明页 开始治疗/查看历史 事件回调（英文版）
 */
void ChildPage_TreIns_En_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        if (data->History_Btn == obj) {
            ESP_LOGI(TAG, "History_Btn stub");
        }
        else if (data->Start_Btn == obj) {
            ThePage_ID_t page_idx = Get_CurPage_Idx();
            switch (page_idx) {
            case ThePage: Child_ThePage_En_Load(ChiThePage_ParSet);  break;
            case MaiPage: Child_MaiPage_En_Load(ChiMaiPage_ParSet);  break;
            case DysPage: Child_DysPage_En_Load(ChiDysPage_ParSet);  break;
            // case KgePage: Child_KgePage_En_Load(ChiKgePage_ParSet);  break;
            default: break;
            }
        }
    }
}

/**
 * @brief 阶段疗程选择按钮事件回调（英文版）
 */
void ChildPage_StageBtn_En_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Select_Data_t* data = (Select_Data_t*)lv_event_get_user_data(e);

    uint8_t num = 0;
    if (code == LV_EVENT_KEY) {
        num = Get_StageBtn(data, obj);
        LV_LOG_USER("num:%d", (int)num);
        if (num < data->Btn_MaxSum) {
            Set_StageBtn_UnSel(data);
            lv_obj_add_style(data->sta[num].obj, &style_gradient, LV_PART_MAIN);
            lv_obj_set_style_image_recolor(data->sta[num].img, lv_color_hex(FONT_RED_COLOR), 0);
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
        num = Get_StageBtn(data, obj);
        if (num < data->Btn_MaxSum) {
            Set_CurTreat_Idx(num);
            Treat_Point_t* p = &data->Treat_Point[num];
            LV_LOG_USER("stage:%d ", (int)p->stage);
            Set_CurStage_Idx(p->stage);
            Set_CurTime_Idx(p->time);
            uint8_t rx_idx = 0;
            ThePage_ID_t page_idx = Get_CurPage_Idx();
            switch (page_idx) {
            case ThePage: Child_ThePage_En_Load(ChiThePage_TreIns);rx_idx = RX_3;  break;
            case MaiPage: Child_MaiPage_En_Load(ChiMaiPage_TreIns);rx_idx = RX_2;  break;
            case DysPage: Child_DysPage_En_Load(ChiDysPage_TreIns);rx_idx = RX_1;  break;
            case KgePage: Child_KgePage_En_Load(ChiKgePage_TreIns);rx_idx = RX_4;  break;
            default: break;
            }
            therapy_init_scheme(&g_rx_all[rx_idx]->schemes[num]);
        }
    }
}

/**
 * @brief 开始选择疗程按钮事件回调（英文版）
 */
void Child_Page_StartBtn_En_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Instr_Data_t* data = (Instr_Data_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {
    }
    else if (code == LV_EVENT_CLICKED) {
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
        if (obj == data->Btn) {
            ThePage_ID_t page_idx = Get_CurPage_Idx();
            switch (page_idx) {
            case ThePage: Child_ThePage_En_Load(ChiThePage_StaSel);  break;
            case MaiPage: Child_MaiPage_En_Load(ChiMaiPage_StaSel);  break;
            case DysPage: Child_DysPage_En_Load(ChiDysPage_StaSel);  break;
            case KgePage: Child_KgePage_En_Load(ChiKgePage_StaSel);  break;
            default: break;
            }
        }
    }
}

/**
 * @brief 下一节倒计时定时器跳转函数（英文版）
 */
static void timer_next_cb(lv_timer_t* t)
{
    Treat_Timer_Ctx_t* ctx = lv_timer_get_user_data(t);

    static uint8_t count = 0;

    LV_LOG_USER("timer_next_cb:%d", (int)count);
    count++;
    if (count >= 30) {
        count = 0;
        lv_timer_del(ctx->timer_Next);
        ctx->timer_Next = NULL;
        therapy_resume();
        ThePage_ID_t page_idx = Get_CurPage_Idx();
        switch (page_idx) {
        case ThePage: Child_ThePage_En_Load(ChiThePage_Chart1);  break;
        case MaiPage: Child_MaiPage_En_Load(ChiMaiPage_Chart1);  break;
        case KgePage: Child_KgePage_En_Load(ChiKgePage_Chart1);  break;
        default: break;
        }
        return;
    }
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

/**
 * @brief 阶段三曲线图1定时器添加数据回调（英文版）
 */
void Chart_Stage3_En_Add_data(lv_timer_t* t)
{
    Chart_Data_t* data = (Chart_Data_t*)lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(data->Chart, NULL);
    uint16_t p = lv_chart_get_point_count(data->Chart);
    uint16_t s = lv_chart_get_x_start_point(data->Chart, ser);
    int32_t* a = lv_chart_get_y_array(data->Chart, ser);
    data->tick++;

    if (data->status == TIMER_RUNNING) {
        /* 获取实时 EMG 包络值，并转换成 uV */
        int32_t env_uv = (int32_t)(emg_force_get_env() * EMG_DSP_RAW16_LSB_UV);
        /* 滑动平均，让显示曲线更稳定 */
        int32_t smoothed = smooth_value(env_uv);
        therapy_emg_feedback(smoothed);
        /* 当前图表显示范围先限制在 0~100uV */
        if (smoothed >= 100) smoothed = 100;
        if (smoothed <= 0) smoothed = 0;

        /* 显示实时肌电值 */
        lv_label_set_text_fmt(data->Label_InsEMG, "%duV", (int)smoothed);
        /* 剩余训练时间由 app_therapy 更新 */
        lv_label_set_text_fmt(data->Label_TimeLeft, "%02dmin %02dsec",
            (int)(data->Value_TimeLeft / 60000),
            (int)((data->Value_TimeLeft % 60000) / 1000));

        /* 将真实肌电值加入曲线 */
        lv_chart_set_next_value(data->Chart, ser, smoothed);

        /*
         * 一轮曲线画满以后重新从左边开始。
         * chart_repeat / cur_repeat_cnt 在 menu_ui 中维护。
         */
        int32_t chart_point_count = lv_chart_get_point_count(data->Chart);
        if(data->tick >= chart_point_count) {
            data->tick = 0;
            cur_repeat_cnt++;
            if (cur_repeat_cnt < chart_repeat) {
                lv_chart_set_all_value(data->Chart, ser, LV_CHART_POINT_NONE);
                lv_chart_set_x_start_point(data->Chart, ser, 0);
            }
            ESP_LOGI(TAG, "当前次数：%d,重复次数：%d,chart point=%d\r\n", cur_repeat_cnt,chart_repeat,chart_point_count);
        }
    }
    else if (data->status == TIMER_END) {
        data->tick = 0;
        g_guide_cont_line = NULL;
        cur_repeat_cnt = 0;
        /* 当前主动训练阶段结束，停止 EMG 采集 */
        emg_force_stop_acq();
        if (data->Timer_Treat) {
            lv_timer_del(data->Timer_Treat);
            data->Timer_Treat = NULL;
        }
        audio_play_request(VOICE_ID_NEXT_EN, 1500);
        /*
         * RX_3 主动训练结束，
         * 进入下一张治疗曲线。
         */
        ThePage_ID_t page_idx = Get_CurPage_Idx();
        switch (page_idx) {
        case ThePage:
            Child_ThePage_En_Load(ChiThePage_Chart2);
            break;
        case MaiPage:
            Child_MaiPage_En_Load(ChiMaiPage_Chart2);
            break;
        case KgePage:
            Child_KgePage_En_Load(ChiKgePage_Chart2);
            break;
        default:
            break;
        }
        /* 清除上一阶段的滤波数据 */
        memset(filter_buf, 0, sizeof(filter_buf));
        filter_idx = 0;
        return;
    }
    /*
     * 将后面的几个点置空，
     * 保持曲线尾部的动态绘制效果。
     */
    a[(s + 1) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;
    a[(s + 3) % p] = LV_CHART_POINT_NONE;
    a[(s + 4) % p] = LV_CHART_POINT_NONE;
    a[(s + 5) % p] = LV_CHART_POINT_NONE;
}

/**
 * @brief 阶段三曲线图2定时器添加数据回调（英文版）
 */
void Chart2_Stage3_En_Add_data(lv_timer_t* t)
{
    Chart_Data_t* data = (Chart_Data_t*)lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(data->Chart, NULL);
    if (ser == NULL) return;

    uint16_t p = lv_chart_get_point_count(data->Chart);
    uint16_t s = lv_chart_get_x_start_point(data->Chart, ser);
    int32_t* a = lv_chart_get_y_array(data->Chart, ser);
    data->tick++;

    if (data->status == TIMER_RUNNING) {
        int32_t env_uv = (int32_t)(emg_force_get_env() * EMG_DSP_RAW16_LSB_UV);
        int32_t smoothed = smooth_value(env_uv);
        therapy_emg_feedback(smoothed);
        if (smoothed > 100) smoothed = 100;
        if (smoothed < 0) smoothed = 0;

        lv_label_set_text_fmt(data->Label_InsEMG, "%duV", (int)smoothed);
        lv_label_set_text_fmt(data->Label_TimeLeft, "%02dmin %02dsec",
                                    (int)(data->Value_TimeLeft / 60000),
                                    (int)((data->Value_TimeLeft % 60000) / 1000));
        lv_chart_set_next_value(data->Chart, ser, smoothed);
    }
    else if (data->status == TIMER_END) {
        /*
         * 防御处理：
         * 如果处方在 5 秒内提前结束，
         * 也进入 Chart3，让 Chart3 做统一完成处理。
         */
        data->tick = 0;
        if (data->Timer_Treat) {
            lv_timer_del(data->Timer_Treat);
            data->Timer_Treat = NULL;
        }
        ThePage_ID_t page_idx = Get_CurPage_Idx();
        switch (page_idx) {
        case ThePage: Child_ThePage_En_Load(ChiThePage_Chart3); break;
        case MaiPage: Child_MaiPage_En_Load(ChiMaiPage_Chart3); break;
        case KgePage: Child_KgePage_En_Load(ChiKgePage_Chart3); break;
        default: break;
        }
        return;
    }
    a[(s + 1) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;
    a[(s + 3) % p] = LV_CHART_POINT_NONE;
    a[(s + 4) % p] = LV_CHART_POINT_NONE;
    a[(s + 5) % p] = LV_CHART_POINT_NONE;
}



/**
 * @brief 返回菜单定时器跳转函数（曲线图完成后延时返回，英文版）
 */
static void Timer_BackToMenu_En_Event_cb(lv_timer_t* t)
{
    Chart_Data_t* data = lv_timer_get_user_data(t);

    static uint8_t count = 0;
    count++;
    if (count >= 30) {
        count = 0;
        lv_timer_del(data->Timer_Back);
        data->Timer_Back = NULL;
        The_En_BackToMenu();
        return;
    }
}

/**
 * @brief 曲线图页弹窗按钮事件回调（是 → 延时返回菜单，否 → 关窗，英文版）
 */
void Chart_Btn_En_Event_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Chart_Data_t* data = (Chart_Data_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        if (data->Window.Yes_Btn == btn) {
            lv_group_t *g = app_keypad_get_group();
            if (g) {
                if (lv_obj_is_valid(data->Window.Yes_Btn)) lv_group_remove_obj(data->Window.Yes_Btn);
                if (lv_obj_is_valid(data->Window.No_Btn))  lv_group_remove_obj(data->Window.No_Btn);
            }
            lv_msgbox_close_async(data->Window.Msgbox);
            therapy_stop();
            lv_label_set_text(data->Label_Finish, "Completed, returning...");
            lv_obj_set_style_text_font(data->Label_Finish, &lv_font_montserrat_12, LV_PART_MAIN);
            if (data->Timer_Back == NULL) {
                data->Timer_Back = lv_timer_create(Timer_BackToMenu_En_Event_cb, 20, data);
            }
        }
        else if (data->Window.No_Btn == btn) {
            lv_msgbox_close_async(data->Window.Msgbox);
        }
    }
}

/* 曲线图确认框键盘：ESC=否 */
static void Chart_MsgBox_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        Chart_Data_t* data = (Chart_Data_t*)lv_event_get_user_data(e);
        if (data->Window.Msgbox && lv_obj_is_valid(data->Window.Msgbox))
            lv_msgbox_close_async(data->Window.Msgbox);
    }
}

/* 曲线图确认框键盘化：是/否 进 group（默认聚焦"Yes"）+ ESC=否 */
static void Chart_ShowConfirm_Key_En(Chart_Data_t* data)
{
    lv_group_t *g = app_keypad_get_group();
    if (g && lv_obj_is_valid(data->Window.Yes_Btn)) {
        lv_group_add_obj(g, data->Window.Yes_Btn);
        lv_group_add_obj(g, data->Window.No_Btn);
        lv_group_focus_obj(data->Window.Yes_Btn);
    }
    lv_obj_add_event_cb(data->Window.Yes_Btn, Chart_MsgBox_Key_cb, LV_EVENT_KEY, data);
    lv_obj_add_event_cb(data->Window.No_Btn,  Chart_MsgBox_Key_cb, LV_EVENT_KEY, data);
}

/**
 * @brief 阶段三曲线图3定时器添加数据回调（英文版，结束后弹 Confirm Return?）
 */
void Chart3_Stage3_En_Add_data(lv_timer_t* t)
{
    Chart_Data_t* data = (Chart_Data_t*)lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(data->Chart, NULL);
    if (ser == NULL) return;

    uint16_t p = lv_chart_get_point_count(data->Chart);
    uint16_t s = lv_chart_get_x_start_point(data->Chart, ser);
    int32_t* a = lv_chart_get_y_array(data->Chart, ser);
    data->tick++;

    if (data->status == TIMER_RUNNING) {
        /* 实时 EMG */
        int32_t env_uv = (int32_t)(emg_force_get_env() * EMG_DSP_RAW16_LSB_UV);
        int32_t smoothed = smooth_value(env_uv);
        if (smoothed > 100) smoothed = 100;
        if (smoothed < 0) smoothed = 0;

        lv_label_set_text_fmt(data->Label_InsEMG, "%duV", (int)smoothed);
        /*
         * 这个 Value_TimeLeft 仍然来自
         * app_therapy 的 train_instr_remain_ms
         */
        lv_label_set_text_fmt(data->Label_TimeLeft, "%02dmin %02dsec",
            (int)(data->Value_TimeLeft / 60000),
            (int)((data->Value_TimeLeft % 60000) / 1000));

        lv_chart_set_next_value(data->Chart, ser, smoothed);

        int32_t chart_point_count = lv_chart_get_point_count(data->Chart);
        if (data->tick >= chart_point_count) {
            data->tick = 0;
            cur_repeat_cnt++;
            if (cur_repeat_cnt < chart_repeat) {
                lv_chart_set_all_value(data->Chart, ser, LV_CHART_POINT_NONE);
                lv_chart_set_x_start_point(data->Chart, ser, 0);
            }
            ESP_LOGI(TAG, "当前次数：%d,重复次数：%d,chart point=%d\r\n", cur_repeat_cnt, chart_repeat, chart_point_count);
        }
    }
    else if (data->status == TIMER_END) {
        data->tick = 0;
        cur_repeat_cnt = 0;
        /*
         * 整个 RX_STEP_TRAIN_INSTR 已经结束。
         */
        g_guide_cont = NULL;
        emg_force_stop_acq();
        therapy_pause();
        /* Stage 3 全部治疗完成语音 */
        audio_play_request(VOICE_ID_DONE_EN, 1500);
        if (data->Timer_Treat) {
            lv_timer_del(data->Timer_Treat);
            data->Timer_Treat = NULL;
        }
        /*
         * 整个 Stage 3 治疗完成。
         */
        Back_En_Window(&data->Window, "Confirm Return?");
        lv_obj_add_event_cb(data->Window.No_Btn, Chart_Btn_En_Event_cb, LV_EVENT_CLICKED, data);
        lv_obj_add_event_cb(data->Window.Yes_Btn, Chart_Btn_En_Event_cb, LV_EVENT_CLICKED, data);
        Chart_ShowConfirm_Key_En(data);
        memset(filter_buf, 0, sizeof(filter_buf));
        filter_idx = 0;
        return;
    }
    a[(s + 1) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;
    a[(s + 3) % p] = LV_CHART_POINT_NONE;
    a[(s + 4) % p] = LV_CHART_POINT_NONE;
    a[(s + 5) % p] = LV_CHART_POINT_NONE;
}


/**
 * @brief 阶段二/四曲线图定时器添加数据回调（英文版，结束后弹 Confirm Return?）
 */
void Chart_Stage2_4_En_Add_data(lv_timer_t* t)
{
    Chart_Data_t* data = (Chart_Data_t*)lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(data->Chart, NULL);
    if (ser == NULL) return;

    uint16_t p = lv_chart_get_point_count(data->Chart);
    uint16_t s = lv_chart_get_x_start_point(data->Chart, ser);
    int32_t* a = lv_chart_get_y_array(data->Chart, ser);
    data->tick++;

    if (data->status == TIMER_RUNNING) {
        /* 获取实时 EMG 包络值 */
        int32_t env_uv = (int32_t)(emg_force_get_env() * EMG_DSP_RAW16_LSB_UV);
        /* 使用上一节添加的 7 点滑动平均 */
        int32_t smoothed = smooth_value(env_uv);
        therapy_emg_feedback(smoothed);
        if (smoothed >= 100) smoothed = 100;
        if (smoothed <= 0) smoothed = 0;

        /* 显示实时肌电值 */
        lv_label_set_text_fmt(data->Label_InsEMG, "%duV", (int)smoothed);
        /* 剩余时间由 app_therapy 更新 */
        lv_label_set_text_fmt(data->Label_TimeLeft, "%02dmin %02dsec",
            (int)(data->Value_TimeLeft / 60000),
            (int)((data->Value_TimeLeft % 60000) / 1000));

        /* 写入真实曲线数据 */
        lv_chart_set_next_value(data->Chart, ser, smoothed);

        /*
         * 当前一屏画完后重新从左侧开始。
         */
        int32_t chart_point_count = lv_chart_get_point_count(data->Chart);
        if(data->tick >= chart_point_count) {
            data->tick = 0;
            cur_repeat_cnt++;
            if (cur_repeat_cnt < chart_repeat) {
                lv_chart_set_all_value(data->Chart, ser, LV_CHART_POINT_NONE);
                lv_chart_set_x_start_point(data->Chart, ser, 0);
            }
            ESP_LOGI(TAG, "当前次数：%d,重复次数：%d,chart point=%d\r\n", cur_repeat_cnt,chart_repeat,chart_point_count);
        }
    }
    else if (data->status == TIMER_END) {
        data->tick = 0;
        g_guide_cont_line = NULL;
        cur_repeat_cnt = 0;
        /* 当前训练已经由处方判定结束 */
        emg_force_stop_acq();
        therapy_pause();
        if (data->Timer_Treat) {
            lv_timer_del(data->Timer_Treat);
            data->Timer_Treat = NULL;
        }
        /*
         * Stage 2 到这里整个训练页面就结束了。
         * 不再跳 Chart2。
         */
        audio_play_request( VOICE_ID_DONE_EN,1500);
        Back_En_Window(&data->Window, "Confirm Return?");
        lv_obj_add_event_cb(data->Window.No_Btn, Chart_Btn_En_Event_cb, LV_EVENT_CLICKED, data);
        lv_obj_add_event_cb(data->Window.Yes_Btn, Chart_Btn_En_Event_cb, LV_EVENT_CLICKED, data);
        Chart_ShowConfirm_Key_En(data);
        /* 清除上一阶段滤波数据 */
        memset(filter_buf, 0, sizeof(filter_buf));
        filter_idx = 0;
        return;
    }
    a[(s + 1) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;
    a[(s + 3) % p] = LV_CHART_POINT_NONE;
    a[(s + 4) % p] = LV_CHART_POINT_NONE;
    a[(s + 5) % p] = LV_CHART_POINT_NONE;
}


/**
 * @brief 获取疗程（英文版）
 */
static uint8_t Get_StageBtn(Select_Data_t* data, lv_obj_t* btn)
{
    for (uint8_t i = 0; i <= data->Btn_MaxSum; i++) {
        if (btn == data->sta[i].btn) {
            return i;
        }
    }
    return data->Btn_MaxSum;
}

/**
 * @brief 将全部疗程照片设未选中（英文版）
 */
static void Set_StageBtn_UnSel(Select_Data_t* data)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; ++i) {
        lv_obj_remove_style(data->sta[i].obj, &style_gradient, LV_PART_MAIN);
        lv_obj_set_style_image_recolor(data->sta[i].img, lv_color_hex(FONT_GRAY_COLOR), 0);
    }
}
