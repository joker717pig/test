#include "Chlid_DysPage_En.h"
#include "PelFlo_The_En_ui.h"
#include "PelFlo_TheChild_En_event.h"
#include "shared_En_ui.h"
#include "menu_ui.h"
#include "basic.h"
#include "app_keypad.h"
#include <string.h>
#include "app_therapy.h"
#include "voice.h"

#define DYS_GROUP_MAX 32
static lv_obj_t* s_dys_en_objs[DYS_GROUP_MAX];
static int s_dys_en_cnt = 0;

extern Treat_Timer_Ctx_t DysPage_En_CtxData1;
static Scroll_Data_t Dys_En_TreIns_Scroll;

static void Dys_En_GroupRegister(lv_obj_t* obj)
{
    lv_group_t *g = app_keypad_get_group();
    if (g && obj) {
        lv_group_add_obj(g, obj);
        if (s_dys_en_cnt < DYS_GROUP_MAX)
            s_dys_en_objs[s_dys_en_cnt++] = obj;
    }
}

void Child_DysPage_En_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        for (int i = 0; i < s_dys_en_cnt; i++) {
            if (s_dys_en_objs[i] && lv_obj_is_valid(s_dys_en_objs[i]))
                lv_group_remove_obj(s_dys_en_objs[i]);
        }
    }
    s_dys_en_cnt = 0;
}

static void Dys_En_StopTimer(void)
{
    if (DysPage_En_CtxData1.timer_Treat) { lv_timer_del(DysPage_En_CtxData1.timer_Treat); DysPage_En_CtxData1.timer_Treat = NULL; }
    if (DysPage_En_CtxData1.timer_Next)  { lv_timer_del(DysPage_En_CtxData1.timer_Next);  DysPage_En_CtxData1.timer_Next  = NULL; }
    DysPage_En_CtxData1.status = TIMER_PAUSED;
}

void Dys_En_BackToMenu(void)
{
    therapy_stop();
    Dys_En_StopTimer();
    Child_DysPage_En_LeaveGroup();
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
        lv_obj_scroll_by(scroll->Obj,0,-step,LV_ANIM_ON);
        lv_event_stop_processing(e);
    }
    else if(key == LV_KEY_UP)
    {
        if(lv_obj_get_scroll_y(scroll->Obj) <= scroll->END_Y) return;
        lv_obj_scroll_by(scroll->Obj,0,step,LV_ANIM_ON);
        lv_event_stop_processing(e);
    }
}


static void DysPage1_widget(Instr_Data_t* param, lv_obj_t* page_cont);
static void DysPage2_widget(Select_Data_t* param, lv_obj_t* page_cont);
static void DysPage3_widget(TreIns_Data_t* param, lv_obj_t* page_cont);
static void DysPage4_widget(Param_Data_t* param, lv_obj_t* page_cont);
static void DysPage5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont);

static void DysPage_Page1_Key_cb(lv_event_t* e);
static void DysPage_Page2_Key_cb(lv_event_t* e);
static void DysPage_Page3_Key_cb(lv_event_t* e);
static void DysPage_Page4_Key_cb(lv_event_t* e);
static void DysPage_Page5_Key_cb(lv_event_t* e);

/* ============================================================================
 * 数据实例（英文文本取自客户定稿）
 * ==========================================================================*/

Instr_Data_t    Dys_En_Instr_data1 = {
    .Obj_Hig = 260,
    .Obj_Wid = 260,
    .Title = "High-Tone Pelvic Floor\nDysfunction",
    .Text = "Indicated for intervention of hypertonic pelvic floor dysfunction. "
            "It improves local blood circulation of the pelvic floor, effectively "
            "relieves abnormal hypertonicity of pelvic floor muscles, relaxes tense "
            "muscle groups, alleviates soreness and discomfort in the pelvic floor "
            "region, and gradually restores the normal physiological state and "
            "healthy function of pelvic floor muscles.",
    .Title_Color = FONT_RED_COLOR,
};
static Stage_Desc_t Stage_Tbl[1] = {
    [STAGE_1] = {"Stage 1", 15},
};

Select_Data_t   Dys_En_Select_data1 = {
    .High_Obj = 520,
    .Pos_Y = -24,
    .Sta_Table = Stage_Tbl,
    .Stage_Num = 1,
};
TreIns_Data_t   Dys_En_Treins_data1 = {
    .Obj_Hig = 120,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "History",
    .Label_Btn2 = "Start",
    .Title = "Stage 1 •",
    .Text = "Stage 1 : 15 treatment sessions, 30 mins each.\n"
            "Therapy: Electrical Stimulation\n"
            "Electrical stimulation: Passive treatment, no voluntary "
            "contraction.",
};
Param_Data_t Dys_En_Param_Data1 = {
    .Value_Intens1 = 1,
    .Value_Intens2 = 1,
    .Value_Freq = 40,
    .Value_Pulse = 600,
    .Slder_Color = FONT_RED_COLOR ,
    .Text_Set1 = "Setup Complete, Next",
    .Text_Set2 = "Setup Complete, Start",
    .Style_KnobColor = &style_RedKnob,
};

Treat_Timer_Ctx_t DysPage_En_CtxData1 = {
    .total_sec = 10 * 1000,
    .remain_sec = 2 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 9,
};
static char title_buf[20];

void Child_DysPage_En_Load(ChiDysPage_En_ID_t page_id)
{
    therapy_stop();
    Dys_En_StopTimer();
    Child_DysPage_En_LeaveGroup();
    Page_Clean();
    switch (page_id) {
    case ChiDysPage_Instr:
        DysPage1_widget(&Dys_En_Instr_data1, g_Ui.page_container);
        break;
    case ChiDysPage_StaSel:
        DysPage2_widget(&Dys_En_Select_data1, g_Ui.page_container);
        break;
    case ChiDysPage_TreIns:
        uint8_t num = Get_CurTime_Idx();
        lv_snprintf(title_buf, sizeof(title_buf), "Treatment %d", num);
        Dys_En_Treins_data1.Title_time = title_buf;
        DysPage3_widget(&Dys_En_Treins_data1, g_Ui.page_container);
        break;
    case ChiDysPage_ParSet:
    {
        uint8_t schemes_idx = Get_CurTreat_Idx();
        Dys_En_Param_Data1.Value_Intens1 = 0;
        Dys_En_Param_Data1.Value_Intens2 = 0;          
        Dys_En_Param_Data1.stage_stim = therapy_cur_stage_stim(&g_rx_all[RX_1]->schemes[schemes_idx]);
        Dys_En_Param_Data1.Value_Freq =  therapy_cur_freq(&g_rx_all[RX_1]->schemes[schemes_idx],1);
        Dys_En_Param_Data1.Value_Pulse = therapy_cur_pw(&g_rx_all[RX_1]->schemes[schemes_idx],1);
        DysPage4_widget(&Dys_En_Param_Data1, g_Ui.page_container);
        audio_play_request(VOICE_ID_MAX_INTENSITY_EN,1500);              //语音播放：调节强度
        break;
    }
    case ChiDysPage_Treat:
    {
        uint8_t schemes_idx = Get_CurTreat_Idx();
        DysPage_En_CtxData1.treat_data.Value_Freq = therapy_cur_freq(&g_rx_all[RX_1]->schemes[schemes_idx],1);
        DysPage_En_CtxData1.treat_data.Value_Pulse = therapy_cur_pw(&g_rx_all[RX_1]->schemes[schemes_idx],1);
        DysPage_En_CtxData1.treat_data.Intens_Value = Dys_En_Param_Data1.Value_Intens1;
        DysPage_En_CtxData1.total_sec = therapy_get_stim_time(&g_rx_all[RX_1]->schemes[schemes_idx]);
        DysPage5_widget(&DysPage_En_CtxData1, g_Ui.page_container);
        break;
    }
    default:
        break;
    }
}

void Child_DysPage_En_ui(void)
{
    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(page_cont);

    DysPage1_widget(&Dys_En_Instr_data1, page_cont);
}

/* ============================================================================
 * [我们] 各页 KEY 导航回调
 * ==========================================================================*/

static void DysPage_Page1_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        PelFlo_ThePage_En_Load(MenPage);
    }
}

static uint8_t Dys_FindStage(Select_Data_t* data, lv_obj_t* btn)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (data->sta[i].btn == btn) return i;
    }
    return data->Btn_MaxSum;
}

static void Dys_StageUnSel(Select_Data_t* data)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (lv_obj_is_valid(data->sta[i].obj)) lv_obj_remove_style(data->sta[i].obj, &style_gradient, LV_PART_MAIN);
        if (lv_obj_is_valid(data->sta[i].img)) lv_obj_set_style_image_recolor(data->sta[i].img, lv_color_hex(FONT_GRAY_COLOR), 0);
    }
}

static void DysPage_Page2_Key_cb(lv_event_t* e)
{
    Select_Data_t* data = (Select_Data_t*)lv_event_get_user_data(e);
    lv_obj_t* obj = lv_event_get_target(e);

    if (lv_event_get_code(e) == LV_EVENT_FOCUSED) {
        uint8_t num = Dys_FindStage(data, obj);
        if (num < data->Btn_MaxSum) {
            Dys_StageUnSel(data);
            if (lv_obj_is_valid(data->sta[num].obj)) lv_obj_add_style(data->sta[num].obj, &style_gradient, LV_PART_MAIN);
            if (lv_obj_is_valid(data->sta[num].img)) lv_obj_set_style_image_recolor(data->sta[num].img, lv_color_hex(FONT_RED_COLOR), 0);
        }
        return;
    }
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC) { Child_DysPage_En_Load(ChiDysPage_Instr); return; }

    uint8_t num = Dys_FindStage(data, obj);
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

static void DysPage_Page3_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_DysPage_En_Load(ChiDysPage_StaSel); return; }

    if (key == LV_KEY_LEFT && obj == data->Start_Btn) lv_group_focus_obj(data->History_Btn);
    else if (key == LV_KEY_RIGHT && obj == data->History_Btn) lv_group_focus_obj(data->Start_Btn);
}

static void DysPage_Page4_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_DysPage_En_Load(ChiDysPage_TreIns); return; }

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

    /*else if (key == LV_KEY_LEFT) {
        if (obj == data->Plu_Btn) lv_group_focus_obj(data->Min_Btn);
    }
    else if (key == LV_KEY_RIGHT) {
        if (obj == data->Min_Btn) lv_group_focus_obj(data->Plu_Btn);
    }*/
   else if (key == LV_KEY_LEFT) {
        if (data->Min_Btn) {
            lv_group_focus_obj(data->Min_Btn);                       /* 先移焦点 */
            lv_obj_send_event(data->Min_Btn, LV_EVENT_CLICKED, NULL); /* 再执行减 */
            lv_event_stop_processing(e);   /* 阻止 group 再做左右切焦点 */
        }
    }
    else if (key == LV_KEY_RIGHT) {
        if (data->Plu_Btn) {
            lv_group_focus_obj(data->Plu_Btn);                       /* 先移焦点 */
            lv_obj_send_event(data->Plu_Btn, LV_EVENT_CLICKED, NULL); /* 再执行加 */
            lv_event_stop_processing(e);
        }
    }
}

static void DysPage_Page5_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) {
        if (ctx->remain_sec == 0) {
            Dys_En_BackToMenu();
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
 * 5 子页 widget
 * ==========================================================================*/

static void DysPage5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont)
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

    Dys_En_GroupRegister(param->treat_data.Plu_Btn);
    Dys_En_GroupRegister(param->treat_data.Min_Btn);
    Dys_En_GroupRegister(param->treat_data.Pause_Btn);
    Dys_En_GroupRegister(param->treat_data.Start_Btn);
    Dys_En_GroupRegister(param->treat_data.Back_Btn);
    lv_group_focus_obj(param->treat_data.Start_Btn);
    lv_obj_add_event_cb(param->treat_data.Plu_Btn,   DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Min_Btn,   DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Pause_Btn, DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn,  DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
}

static void DysPage4_widget(Param_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    ParSet_En_Widget(page_cont, param);
    lv_obj_add_event_cb(param->Set_Btn, ChildPage_P4SetBtn_En_Event_cb, LV_EVENT_CLICKED, param);

    Dys_En_GroupRegister(param->Min_Btn);
    Dys_En_GroupRegister(param->Plu_Btn);
    Dys_En_GroupRegister(param->Set_Btn);
    lv_group_focus_obj(param->Plu_Btn);
    lv_obj_add_event_cb(param->Min_Btn, DysPage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Plu_Btn, DysPage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Set_Btn, DysPage_Page4_Key_cb, LV_EVENT_KEY, param);
}

static void DysPage3_widget(TreIns_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /* 页面滚动设置 */
    Dys_En_TreIns_Scroll.Obj = cont;
    Dys_En_TreIns_Scroll.Origin_Y = 210;
    Dys_En_TreIns_Scroll.END_Y = 0;
    param->Scroll = &Dys_En_TreIns_Scroll;

    TreIns_En_Widget(cont, param);
    lv_obj_add_event_cb(param->History_Btn, ChildPage_TreIns_En_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->Start_Btn,   ChildPage_TreIns_En_Event_cb, LV_EVENT_CLICKED, param);

    Dys_En_GroupRegister(param->History_Btn);
    Dys_En_GroupRegister(param->Start_Btn);
    lv_group_focus_obj(param->Start_Btn);
    lv_obj_add_event_cb(param->History_Btn, DysPage_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Start_Btn,   DysPage_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->History_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
    lv_obj_add_event_cb(param->Start_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
}

static void DysPage2_widget(Select_Data_t* param, lv_obj_t* page_cont)
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
        Dys_En_GroupRegister(param->sta[i].btn);
        lv_obj_add_event_cb(param->sta[i].btn, DysPage_Page2_Key_cb, LV_EVENT_ALL, param);
    }
    if (param->Btn_MaxSum > 0) lv_group_focus_obj(param->sta[0].btn);
}

static void DysPage1_widget(Instr_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    Instr_En_Widget(cont, param);

    lv_obj_add_event_cb(param->Btn, Child_Page_StartBtn_En_Event_cb, LV_EVENT_ALL, param);

    Dys_En_GroupRegister(param->Btn);
    lv_group_focus_obj(param->Btn);
    lv_obj_add_event_cb(param->Btn, DysPage_Page1_Key_cb, LV_EVENT_KEY, param);
}
