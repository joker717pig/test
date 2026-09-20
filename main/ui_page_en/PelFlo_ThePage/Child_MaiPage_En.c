#include "Child_MaiPage_En.h"
#include "PelFlo_The_En_ui.h"
#include "PelFlo_TheChild_En_event.h"
#include "shared_En_ui.h"
#include "menu_ui.h"
#include "menu_ui_event_cb.h"
#include "basic.h"
#include "app_keypad.h"
#include <string.h>
#include "app_therapy.h"
#include "emg_force.h"
#include "emg_dsp.h"
#include "voice.h"

#define MAI_GROUP_MAX 48
static lv_obj_t* s_mai_en_objs[MAI_GROUP_MAX];
static int s_mai_en_cnt = 0;
static lv_obj_t* s_mai_en_esc_obj = NULL;

extern Treat_Timer_Ctx_t* MaiPage_En_CtxData[STAGE_MAX];
extern Chart_Data_t* Mai_En_Chart_Data_List[STAGE_MAX];
static Scroll_Data_t Mai_En_TreIns_Scroll;

static void Mai_En_GroupRegister(lv_obj_t* obj)
{
    lv_group_t *g = app_keypad_get_group();
    if (g && obj) {
        lv_group_add_obj(g, obj);
        if (s_mai_en_cnt < MAI_GROUP_MAX)
            s_mai_en_objs[s_mai_en_cnt++] = obj;
    }
}

void Child_MaiPage_En_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        for (int i = 0; i < s_mai_en_cnt; i++) {
            if (s_mai_en_objs[i] && lv_obj_is_valid(s_mai_en_objs[i]))
                lv_group_remove_obj(s_mai_en_objs[i]);
        }
        if (s_mai_en_esc_obj && lv_obj_is_valid(s_mai_en_esc_obj))
            lv_group_remove_obj(s_mai_en_esc_obj);
    }
    s_mai_en_cnt = 0;
    s_mai_en_esc_obj = NULL;
}

static void Mai_En_StopTimer(void)
{
    for (int i = 0; i < STAGE_MAX; i++) {
        if (MaiPage_En_CtxData[i]->timer_Treat) { lv_timer_del(MaiPage_En_CtxData[i]->timer_Treat); MaiPage_En_CtxData[i]->timer_Treat = NULL; }
        if (MaiPage_En_CtxData[i]->timer_Next)  { lv_timer_del(MaiPage_En_CtxData[i]->timer_Next);  MaiPage_En_CtxData[i]->timer_Next  = NULL; }
        MaiPage_En_CtxData[i]->status = TIMER_PAUSED;
    }
    for (int i = 0; i < STAGE_MAX; i++) {
        if (Mai_En_Chart_Data_List[i]) {
            if (Mai_En_Chart_Data_List[i]->Timer_Treat) { lv_timer_del(Mai_En_Chart_Data_List[i]->Timer_Treat); Mai_En_Chart_Data_List[i]->Timer_Treat = NULL; }
            if (Mai_En_Chart_Data_List[i]->Timer_Back)  { lv_timer_del(Mai_En_Chart_Data_List[i]->Timer_Back);  Mai_En_Chart_Data_List[i]->Timer_Back  = NULL; }
        }
    }
    emg_force_stop_acq();
}

void Mai_En_BackToMenu(void)
{
    therapy_stop();
    Mai_En_StopTimer();
    Child_MaiPage_En_LeaveGroup();
    Goto_MenuPage();
}

static void page_scroll_key_cb(lv_event_t* e)
{
    Scroll_Data_t* scroll = (Scroll_Data_t*)lv_event_get_user_data(e);
    uint32_t key = lv_event_get_key(e);
    uint16_t step = 40;
    if(key == LV_KEY_DOWN)
    {
        if(lv_obj_get_scroll_y(scroll->Obj) >= scroll->Origin_Y) return;
        lv_obj_scroll_by(scroll->Obj, 0, -step, LV_ANIM_ON);
        lv_event_stop_processing(e);
    }
    else if(key == LV_KEY_UP)
    {
        if(lv_obj_get_scroll_y(scroll->Obj) <= scroll->END_Y) return;
        lv_obj_scroll_by(scroll->Obj, 0, step, LV_ANIM_ON);
        lv_event_stop_processing(e);
    }
}

static void Mai_En_RegisterEscObj(lv_obj_t* cont, Chart_Data_t* data);
static void MaiPage_En_ChartEsc_cb(lv_event_t* e);

static void Mai_En_RegisterEscObj(lv_obj_t* cont, Chart_Data_t* data)
{
    s_mai_en_esc_obj = cont;
    lv_group_t *g = app_keypad_get_group();
    if (g) { lv_group_add_obj(g, cont); lv_group_focus_obj(cont); }
    lv_obj_add_event_cb(cont, MaiPage_En_ChartEsc_cb, LV_EVENT_KEY, data);
}

static void MaiPage_En_ChartEsc_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        Chart_Data_t* data = (Chart_Data_t*)lv_event_get_user_data(e);
        if (data) {
            if (data->Timer_Treat) { lv_timer_del(data->Timer_Treat); data->Timer_Treat = NULL; }
            if (data->Timer_Back)  { lv_timer_del(data->Timer_Back);  data->Timer_Back  = NULL; }
        }
        Mai_En_StopTimer();
        Child_MaiPage_En_LeaveGroup();
        emg_force_stop_acq();
        PelFlo_ThePage_En_Load(MenPage);
    }
}

static void MaiPage1_widget(Instr_Data_t* param, lv_obj_t* page_cont);
static void MaiPage2_widget(Select_Data_t* param, lv_obj_t* page_cont);
static void MaiPage3_widget(TreIns_Data_t* param, lv_obj_t* page_cont);
static void MaiPage4_widget(Param_Data_t* param, lv_obj_t* page_cont);
static void MaiPage5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont);
static void MaiPage6_widget(Chart_Data_t* param, lv_obj_t* page_cont);
static void MaiPage7_widget(Chart_Data_t* param, lv_obj_t* page_cont);
static void MaiPage8_widget(Chart_Data_t* param, lv_obj_t* page_cont);

static void MaiPage_Page1_Key_cb(lv_event_t* e);
static void MaiPage_Page2_Key_cb(lv_event_t* e);
static void MaiPage_Page3_Key_cb(lv_event_t* e);
static void MaiPage_Page4_Key_cb(lv_event_t* e);
static void MaiPage_Page5_Key_cb(lv_event_t* e);

/* ============================================================================
 * 数据实例（英文文本取自客户定稿）
 * ==========================================================================*/

Instr_Data_t    Mai_En_Instr_data1 = {
    .Obj_Hig = 235,
    .Obj_Wid = 260,
    .Title = "Pelvic Floor Maintenance",
    .Text = "Suitable for postpartum women without pelvic floor dysfunction "
            "who wish to strengthen pelvic floor function. It is also applicable "
            "to those requiring maintenance after rehabilitation of pelvic floor "
            "and sexual dysfunction, as well as people who conduct daily pelvic "
            "floor care to prevent pelvic floor disorders.",
    .Title_Color = FONT_RED_COLOR,
};
static Stage_Desc_t Stage_Tbl[STAGE_MAX] = {
    [STAGE_1] = {"Stage 1", 7},
    [STAGE_2] = {"Stage 2", 5},
    [STAGE_3] = {"Stage 3", 3},
    [STAGE_4] = {"Stage 4", 5},
};

Select_Data_t   Mai_En_Select_data1 = {
    .High_Obj = 700,
    .Pos_Y = -26,
    .Sta_Table = Stage_Tbl,
    .Stage_Num = 4,
};
TreIns_Data_t   Mai_En_Treins_data1 = {
    .Obj_Hig = 125,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "History",
    .Label_Btn2 = "Start",
    .Title = "Stage 1 • ",
    .Text = "Stage 1 : 7 treatment sessions, 30 mins each.\n"
            "Therapy: Electrical Stimulation\n"
            "Electrical stimulation: Passive treatment, no voluntary "
            "contraction.",
};
TreIns_Data_t   Mai_En_Treins_data2 = {
    .Obj_Hig = 195,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "History",
    .Label_Btn2 = "Start",
    .Title = "Stage 2 • ",
    .Text = "Stage 2 : 5 treatment sessions, 30 mins each.\n"
            "Combined therapies: Electrical Stimulation + "
            "Conditional Stimulation\n"
            "Electrical stimulation: Passive treatment, no voluntary "
            "contraction.\n"
            "Conditional stimulation: Actively control curve within "
            "guided range via graphics & voice; stimulation triggers "
            "when out of range.",
};
TreIns_Data_t   Mai_En_Treins_data3 = {
    .Obj_Hig = 240,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "History",
    .Label_Btn2 = "Start",
    .Title = "Stage 3 • ",
    .Text = "Stage 3 : 3 treatment sessions, 30 mins each.\n"
            "Combined therapies: Electrical Stimulation + "
            "Conditional Stimulation + Biofeedback\n"
            "Electrical Stimulation: Passive treatment, no "
            "voluntary contraction\n"
            "Conditional Stimulation: Actively control curve within "
            "guided range via graphics & voice; stimulation triggers "
            "when out of range.\n"
            "Biofeedback: Actively control curve within guided "
            "range via graphics & voice.",
};
TreIns_Data_t   Mai_En_Treins_data4 = {
    .Obj_Hig = 190,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "History",
    .Label_Btn2 = "Start",
    .Title = "Stage 4 • ",
    .Text = "Stage 4 : 5 treatment sessions, 30 mins each.\n"
            "Combined therapies: Electrical Stimulation + Biofeedback\n"
            "Electrical Stimulation: Passive treatment, no voluntary contraction\n"
            "Biofeedback: Actively control curve within guided range via graphics & voice.",
};

TreIns_Data_t* Mai_En_Treins_list[STAGE_MAX] = {
   &Mai_En_Treins_data1,
   &Mai_En_Treins_data2,
   &Mai_En_Treins_data3,
   &Mai_En_Treins_data4,
};
Param_Data_t Mai_En_Param_Data1 = {
    .Value_Intens1 = 1,
    .Value_Intens2 = 1,
    .Value_Freq = 40,
    .Value_Pulse = 500,
    .Slder_Color = FONT_RED_COLOR ,
    .Text_Set1 = "Setup Complete, Next",
    .Text_Set2 = "Setup Complete, Start",
    .Style_KnobColor = &style_RedKnob,
};
Param_Data_t* Mai_En_Param_list[STAGE_MAX] = {
    &Mai_En_Param_Data1,
    &Mai_En_Param_Data1,
    &Mai_En_Param_Data1,
    &Mai_En_Param_Data1,
};
Treat_Timer_Ctx_t MaiPage_En_CtxData1 = {
    .total_sec = 10 * 1000,
    .remain_sec = 5 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 5,
};
Treat_Timer_Ctx_t MaiPage_En_CtxData2 = {
    .total_sec = 10 * 1000,
    .remain_sec = 3 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 6,
};
Treat_Timer_Ctx_t MaiPage_En_CtxData3 = {
    .total_sec = 10 * 1000,
    .remain_sec = 1 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 7,
};
Treat_Timer_Ctx_t* MaiPage_En_CtxData[STAGE_MAX] = {
    &MaiPage_En_CtxData1,
    &MaiPage_En_CtxData2,
    &MaiPage_En_CtxData3,
    &MaiPage_En_CtxData1,
};
Chart_Data_t Mai_En_Chart_Data1 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t Mai_En_Chart_Data2 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t Mai_En_Chart_Data3 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t Mai_En_Chart_Data4 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t* Mai_En_Chart_Data_List[STAGE_MAX] = {
    &Mai_En_Chart_Data1,
    &Mai_En_Chart_Data2,
    &Mai_En_Chart_Data3,
    &Mai_En_Chart_Data4,
};
static char title_buf[20];

void Child_MaiPage_En_Load(ChiMaiPage_En_ID_t page_id)
{
    Mai_En_StopTimer();
    Child_MaiPage_En_LeaveGroup();
    Page_Clean();
    uint8_t idx = Get_CurStage_Idx();
    switch (page_id) {
    case ChiMaiPage_Instr:
        MaiPage1_widget(&Mai_En_Instr_data1, g_Ui.page_container);
        break;
    case ChiMaiPage_StaSel:
        MaiPage2_widget(&Mai_En_Select_data1, g_Ui.page_container);
        break;
    case ChiMaiPage_TreIns:
        uint8_t num = Get_CurTime_Idx();
        lv_snprintf(title_buf, sizeof(title_buf), " Treatment %d", num);
        Mai_En_Treins_list[idx]->Title_time = title_buf;
        MaiPage3_widget(Mai_En_Treins_list[idx], g_Ui.page_container);
        break;
    case ChiMaiPage_ParSet:
    {
        uint8_t schemes_idx = Get_CurTreat_Idx();
        Mai_En_Param_Data1.Value_Intens1 = 0;
        Mai_En_Param_Data1.Value_Intens2 = 0;             
        Mai_En_Param_Data1.stage_stim = therapy_cur_stage_stim(&g_rx_all[RX_2]->schemes[schemes_idx]);
        Mai_En_Param_Data1.Value_Freq = therapy_cur_freq(&g_rx_all[RX_2]->schemes[schemes_idx],1);
        Mai_En_Param_Data1.Value_Pulse =therapy_cur_pw(&g_rx_all[RX_2]->schemes[schemes_idx],1);
        MaiPage4_widget(&Mai_En_Param_Data1, g_Ui.page_container);
        audio_play_request(VOICE_ID_MAX_INTENSITY_EN,1500);              //语音播放：调节强度
        break;
    }
    case ChiMaiPage_Treat:
   {
        uint8_t schemes_idx = Get_CurTreat_Idx();
        MaiPage_En_CtxData1.treat_data.Value_Freq =therapy_cur_freq(&g_rx_all[RX_2]->schemes[schemes_idx],1);
        MaiPage_En_CtxData1.treat_data.Value_Pulse = therapy_cur_pw(&g_rx_all[RX_2]->schemes[schemes_idx],1);
        MaiPage_En_CtxData1.treat_data.Intens_Value =Mai_En_Param_Data1.Value_Intens1;
        MaiPage_En_CtxData1.total_sec = therapy_get_stim_time(&g_rx_all[RX_2]->schemes[schemes_idx]);
        MaiPage5_widget(&MaiPage_En_CtxData1, g_Ui.page_container);
        break;
    }
    case ChiMaiPage_Chart1:
        emg_force_reset();
        emg_force_start_acq(500, 1);
        MaiPage6_widget(&Mai_En_Chart_Data1, g_Ui.page_container);
        Chart_Init_Point(Mai_En_Chart_Data1.Chart);
        if (idx == STAGE_4)
        {
            therapy_init_chart_instr(&Mai_En_Chart_Data1);
        }
        else
        {
            therapy_init_chart(&Mai_En_Chart_Data1);
        }
        break;
    case ChiMaiPage_Chart2:
        emg_force_reset();
        emg_force_start_acq(500, 1);
        MaiPage7_widget(&Mai_En_Chart_Data2, g_Ui.page_container);
        Chart_Init_Point(Mai_En_Chart_Data2.Chart);
        therapy_init_chart_instr(&Mai_En_Chart_Data2);
        break;
    case ChiMaiPage_Chart3:
        MaiPage8_widget(Mai_En_Chart_Data_List[idx], g_Ui.page_container);
        break;
    default:
        break;
    }
}

void Child_MaiPage_En_ui(void)
{
    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(page_cont);

    MaiPage1_widget(&Mai_En_Instr_data1, page_cont);
}

/* ============================================================================
 * [我们] 各页 KEY 导航回调
 * ==========================================================================*/

static void MaiPage_Page1_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        PelFlo_ThePage_En_Load(MenPage);
    }
}

static uint8_t Mai_FindStage(Select_Data_t* data, lv_obj_t* btn)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (data->sta[i].btn == btn) return i;
    }
    return data->Btn_MaxSum;
}

static void Mai_StageUnSel(Select_Data_t* data)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (lv_obj_is_valid(data->sta[i].obj)) lv_obj_remove_style(data->sta[i].obj, &style_gradient, LV_PART_MAIN);
        if (lv_obj_is_valid(data->sta[i].img)) lv_obj_set_style_image_recolor(data->sta[i].img, lv_color_hex(FONT_GRAY_COLOR), 0);
    }
}

static void MaiPage_Page2_Key_cb(lv_event_t* e)
{
    Select_Data_t* data = (Select_Data_t*)lv_event_get_user_data(e);
    lv_obj_t* obj = lv_event_get_target(e);

    if (lv_event_get_code(e) == LV_EVENT_FOCUSED) {
        uint8_t num = Mai_FindStage(data, obj);
        if (num < data->Btn_MaxSum) {
            Mai_StageUnSel(data);
            if (lv_obj_is_valid(data->sta[num].obj)) lv_obj_add_style(data->sta[num].obj, &style_gradient, LV_PART_MAIN);
            if (lv_obj_is_valid(data->sta[num].img)) lv_obj_set_style_image_recolor(data->sta[num].img, lv_color_hex(FONT_RED_COLOR), 0);
        }
        return;
    }
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC) { Child_MaiPage_En_Load(ChiMaiPage_Instr); return; }

    uint8_t num = Mai_FindStage(data, obj);
    if (num >= data->Btn_MaxSum) return;

    const uint8_t per_row = 3;
    uint8_t target = 0xFF;
    if (key == LV_KEY_LEFT && num % per_row != 0)                      target = num - 1;
    else if (key == LV_KEY_RIGHT && num % per_row != per_row - 1)      target = num + 1;
    else if (key == LV_KEY_UP && num >= per_row)                       target = num - per_row;
    else if (key == LV_KEY_DOWN && num + per_row < data->Btn_MaxSum)   target = num + per_row;

    if (target != 0xFF && lv_obj_is_valid(data->sta[target].btn)) {
        lv_group_focus_obj(data->sta[target].btn);
        lv_event_stop_processing(e);
    }
}

static void MaiPage_Page3_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_MaiPage_En_Load(ChiMaiPage_StaSel); return; }

    if (key == LV_KEY_LEFT && obj == data->Start_Btn) lv_group_focus_obj(data->History_Btn);
    else if (key == LV_KEY_RIGHT && obj == data->History_Btn) lv_group_focus_obj(data->Start_Btn);
}

static void MaiPage_Page4_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_MaiPage_En_Load(ChiMaiPage_TreIns); return; }

    // if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
    //     int32_t v = lv_slider_get_value(data->Slider);
    //     v += (key == LV_KEY_RIGHT) ? 1 : -1;
    //     if (v < 0) v = 0;
    //     if (v > 10) v = 10;
    //     lv_slider_set_value(data->Slider, v, LV_ANIM_OFF);
    //     if (data->Value_Stage == 1)      data->Value_Intens1 = v;
    //     else if (data->Value_Stage == 2) data->Value_Intens2 = v;
    //     if (lv_obj_is_valid(data->Label_Intens)) lv_label_set_text_fmt(data->Label_Intens, "%d", (int)v);
    //     lv_event_stop_processing(e);
    //     return;
    // }

     /* 上下 = 线性焦点链：Min -> Plu -> Set */
    if (key == LV_KEY_UP) {
        if (obj == data->Set_Btn) lv_group_focus_obj(data->Plu_Btn);
    }
    else if (key == LV_KEY_DOWN) {
        if (obj == data->Min_Btn) lv_group_focus_obj(data->Set_Btn);
        else if (obj == data->Plu_Btn) lv_group_focus_obj(data->Set_Btn);
    }

    else if (key == LV_KEY_LEFT) {
        if (obj == data->Plu_Btn) lv_group_focus_obj(data->Min_Btn);
    }
    else if (key == LV_KEY_RIGHT) {
        if (obj == data->Min_Btn) lv_group_focus_obj(data->Plu_Btn);
    }
}

static void MaiPage_Page5_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) {
        if (ctx->remain_sec == 0) {
            Mai_En_BackToMenu();
        }
        else {
            // if (ctx->timer_Treat) { lv_timer_del(ctx->timer_Treat); ctx->timer_Treat = NULL; }
            // ctx->status = TIMER_PAUSED;
            therapy_pause();
            The_En_Page5_ShowConfirm(ctx);
        }
        return;
    }

    if (key == LV_KEY_LEFT) {
        if (obj == ctx->treat_data.Start_Btn) lv_group_focus_obj(ctx->treat_data.Pause_Btn);
        else if (obj == ctx->treat_data.Back_Btn) lv_group_focus_obj(ctx->treat_data.Start_Btn);
    }
    else if (key == LV_KEY_RIGHT) {
        if (obj == ctx->treat_data.Pause_Btn) lv_group_focus_obj(ctx->treat_data.Start_Btn);
        else if (obj == ctx->treat_data.Start_Btn) lv_group_focus_obj(ctx->treat_data.Back_Btn);
    }
    else if (key == LV_KEY_UP) {
        if (obj == ctx->treat_data.Pause_Btn) lv_group_focus_obj(ctx->treat_data.Plu_Btn);
        else if (obj == ctx->treat_data.Start_Btn) lv_group_focus_obj(ctx->treat_data.Plu_Btn);
        else if (obj == ctx->treat_data.Back_Btn) lv_group_focus_obj(ctx->treat_data.Plu_Btn);
        else if (obj == ctx->treat_data.Min_Btn) lv_group_focus_obj(ctx->treat_data.Plu_Btn);
    }
    else if (key == LV_KEY_DOWN) {
        if (obj == ctx->treat_data.Plu_Btn) lv_group_focus_obj(ctx->treat_data.Min_Btn);
        else if (obj == ctx->treat_data.Min_Btn) lv_group_focus_obj(ctx->treat_data.Back_Btn);
    }
}

/* ============================================================================
 * 9 子页 widget
 * ==========================================================================*/

static void MaiPage8_widget(Chart_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    Chart_En_widget(cont, param);
    lv_obj_add_event_cb(param->Chart, Chart_Draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, param);
    if(param->Timer_Treat){lv_timer_del(param->Timer_Treat);param->Timer_Treat=NULL;}
    param->tick = 0;
    param->Timer_Treat = lv_timer_create(Chart3_Stage3_En_Add_data, 10, param);
    Mai_En_RegisterEscObj(cont, param);
}

static void MaiPage7_widget(Chart_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    Therapy_Guide_Init(cont);
    Chart_En_widget(cont, param);
    lv_obj_add_event_cb(param->Chart, Chart_Draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, param);
    if(param->Timer_Treat)
    {
    lv_timer_del(param->Timer_Treat);
    param->Timer_Treat=NULL;
    }
    param->tick = 0;
    param->Timer_Treat = lv_timer_create(Chart3_Stage3_En_Add_data, 10, param);
    Mai_En_RegisterEscObj(cont, param);
}

static void MaiPage6_widget(Chart_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    // lv_obj_set_style_pad_all(cont, 0, 0);

    Therapy_Guide_LineInit(cont);
    Chart_En_widget(cont, param);
    lv_obj_add_event_cb(param->Chart, Chart_Draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, param);

    if(param->Timer_Treat) { lv_timer_del(param->Timer_Treat); param->Timer_Treat=NULL; }
    param->tick = 0;
    if (Get_CurStage_Idx() == STAGE_3)
    {
        param->Timer_Treat = lv_timer_create(Chart_Stage3_En_Add_data, 10, param);
    }
    else
    {
    param->Timer_Treat = lv_timer_create(Chart_Stage2_4_En_Add_data, 10, param);
    }
    Mai_En_RegisterEscObj(cont, param);
}

static void MaiPage5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    Treat_En_Widget(cont, param);

    lv_obj_add_event_cb(param->treat_data.Pause_Btn, ChildPage_P5Btn_En_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, ChildPage_P5Btn_En_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn,  ChildPage_P5Btn_En_Event_cb, LV_EVENT_CLICKED, param);

    Mai_En_GroupRegister(param->treat_data.Plu_Btn);
    Mai_En_GroupRegister(param->treat_data.Min_Btn);
    Mai_En_GroupRegister(param->treat_data.Pause_Btn);
    Mai_En_GroupRegister(param->treat_data.Start_Btn);
    Mai_En_GroupRegister(param->treat_data.Back_Btn);
    lv_group_focus_obj(param->treat_data.Start_Btn);
    lv_obj_add_event_cb(param->treat_data.Plu_Btn,   MaiPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Min_Btn,   MaiPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Pause_Btn, MaiPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, MaiPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn,  MaiPage_Page5_Key_cb, LV_EVENT_KEY, param);
}

static void MaiPage4_widget(Param_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    ParSet_En_Widget(page_cont, param);
    lv_obj_add_event_cb(param->Set_Btn, ChildPage_P4SetBtn_En_Event_cb, LV_EVENT_CLICKED, param);

    Mai_En_GroupRegister(param->Min_Btn);
    Mai_En_GroupRegister(param->Plu_Btn);
    Mai_En_GroupRegister(param->Set_Btn);
    lv_group_focus_obj(param->Set_Btn);
    lv_obj_add_event_cb(param->Min_Btn, MaiPage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Plu_Btn, MaiPage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Set_Btn, MaiPage_Page4_Key_cb, LV_EVENT_KEY, param);
}

static void MaiPage3_widget(TreIns_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    /* 页面滚动设置 */
    Mai_En_TreIns_Scroll.Obj = cont;
    Mai_En_TreIns_Scroll.Origin_Y = 210;
    Mai_En_TreIns_Scroll.END_Y = 0;
    param->Scroll = &Mai_En_TreIns_Scroll;
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    TreIns_En_Widget(cont, param);
    lv_obj_add_event_cb(param->History_Btn, ChildPage_TreIns_En_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->Start_Btn,   ChildPage_TreIns_En_Event_cb, LV_EVENT_CLICKED, param);

    Mai_En_GroupRegister(param->History_Btn);
    Mai_En_GroupRegister(param->Start_Btn);
    lv_group_focus_obj(param->Start_Btn);
    lv_obj_add_event_cb(param->History_Btn, MaiPage_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Start_Btn,   MaiPage_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->History_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
    lv_obj_add_event_cb(param->Start_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);

}

static void MaiPage2_widget(Select_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cont, 0, 0);
    Select_En_Widget2(cont, param);
    for (uint8_t i = 0; i < param->Btn_MaxSum; i++) {
        lv_obj_add_event_cb(param->sta[i].btn, ChildPage_StageBtn_En_Event_cb, LV_EVENT_ALL, param);
        Mai_En_GroupRegister(param->sta[i].btn);
        lv_obj_add_event_cb(param->sta[i].btn, MaiPage_Page2_Key_cb, LV_EVENT_ALL, param);
    }
    if (param->Btn_MaxSum > 0) lv_group_focus_obj(param->sta[0].btn);
}

static void MaiPage1_widget(Instr_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    Instr_En_Widget(cont, param);

    lv_obj_add_event_cb(param->Btn, Child_Page_StartBtn_En_Event_cb, LV_EVENT_ALL, param);

    Mai_En_GroupRegister(param->Btn);
    lv_group_focus_obj(param->Btn);
    lv_obj_add_event_cb(param->Btn, MaiPage_Page1_Key_cb, LV_EVENT_KEY, param);
}
