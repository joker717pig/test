#include "Child_ThePage_En.h"
#include "PelFlo_The_En_ui.h"
#include "PelFlo_TheChild_En_event.h"
#include "shared_En_ui.h"       /* [我们] 英文共享控件：Instr_En_Widget/Select_En_Widget2/TreIns_En_Widget/ParSet_En_Widget/Treat_En_Widget/Chart_En_widget */
#include "menu_ui.h"
#include "menu_ui_event_cb.h"   /* [我们] Chart_Draw_event_cb（语言无关，复用中文） */
#include "basic.h"
#include "app_keypad.h"         /* [我们] keypad group 注册 */
#include "app_therapy.h"        /* [我们] 阶段C：治疗控制（退出/ESC 停引擎） */
#include "emg_force.h"
#include "emg_dsp.h"
#include <string.h>             /* [我们] memset */
#include "voice.h"
/* ============================================================================
 * [我们] B5-C 通用守则基础设施（坑 6.8 / B2-4 / B3-3 同族，英文版独立 static 状态）
 * ==========================================================================*/

#define THE_GROUP_MAX 48
static lv_obj_t* s_the_en_objs[THE_GROUP_MAX];
static int s_the_en_cnt = 0;
static lv_obj_t* s_the_en_esc_obj = NULL;        /* 曲线图页无按钮时的 ESC 焦点对象 */

/* [我们] 数据实例数组前向声明（定义见文件下部，The_En_StopTimer 先引用） */
extern Treat_Timer_Ctx_t* ThePage_En_CtxData[STAGE_MAX];
extern Chart_Data_t* The_En_Chart_Data_List[STAGE_MAX];
extern Chart_Data_t The_En_Chart_Data3;

static Scroll_Data_t The_En_TreIns_Scroll;

static void The_En_GroupRegister(lv_obj_t* obj)
{
    lv_group_t *g = app_keypad_get_group();
    if (g && obj) {
        lv_group_add_obj(g, obj);
        if (s_the_en_cnt < THE_GROUP_MAX)
            s_the_en_objs[s_the_en_cnt++] = obj;
    }
}

void Child_ThePage_En_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        for (int i = 0; i < s_the_en_cnt; i++) {
            if (s_the_en_objs[i] && lv_obj_is_valid(s_the_en_objs[i]))
                lv_group_remove_obj(s_the_en_objs[i]);
        }
        if (s_the_en_esc_obj && lv_obj_is_valid(s_the_en_esc_obj))
            lv_group_remove_obj(s_the_en_esc_obj);
    }
    s_the_en_cnt = 0;
    s_the_en_esc_obj = NULL;
}

/* 停止治疗/曲线图全部定时器（页面退出/切页前调用） */
static void The_En_StopTimer(void)
{
    for (int i = 0; i < STAGE_MAX; i++) {
        if (ThePage_En_CtxData[i]->timer_Treat) { lv_timer_del(ThePage_En_CtxData[i]->timer_Treat); ThePage_En_CtxData[i]->timer_Treat = NULL; }
        if (ThePage_En_CtxData[i]->timer_Next)  { lv_timer_del(ThePage_En_CtxData[i]->timer_Next);  ThePage_En_CtxData[i]->timer_Next  = NULL; }
        ThePage_En_CtxData[i]->status = TIMER_PAUSED;
    }
    for (int i = 0; i < STAGE_MAX; i++) {
        if (The_En_Chart_Data_List[i]) {
            if (The_En_Chart_Data_List[i]->Timer_Treat) { lv_timer_del(The_En_Chart_Data_List[i]->Timer_Treat); The_En_Chart_Data_List[i]->Timer_Treat = NULL; }
            if (The_En_Chart_Data_List[i]->Timer_Back)  { lv_timer_del(The_En_Chart_Data_List[i]->Timer_Back);  The_En_Chart_Data_List[i]->Timer_Back  = NULL; }
        }
    }
    if (The_En_Chart_Data3.Timer_Treat) { lv_timer_del(The_En_Chart_Data3.Timer_Treat); The_En_Chart_Data3.Timer_Treat = NULL; }
    if (The_En_Chart_Data3.Timer_Back)  { lv_timer_del(The_En_Chart_Data3.Timer_Back);  The_En_Chart_Data3.Timer_Back = NULL; }
    emg_force_stop_acq();
}

/* 统一退出：停定时器 + 移出 group + 回主菜单（删除路径调用后须提前 return，坑 B4-3） */
void The_En_BackToMenu(void)
{
    therapy_stop();      /* [我们] 阶段C：停治疗引擎（停波 + 会话复位） */
    The_En_StopTimer();
    Child_ThePage_En_LeaveGroup();
    Goto_MenuPage();     /* [我们] 按 g_app_lang 自动回英文主菜单 */
}

static void page_scroll_key_cb(lv_event_t* e)
{
    Scroll_Data_t* scroll = (Scroll_Data_t*)lv_event_get_user_data(e);
    uint32_t key = lv_event_get_key(e);
    uint16_t step = 40;
    if(key == LV_KEY_DOWN) { 
    if(lv_obj_get_scroll_y(scroll->Obj) >= scroll->Origin_Y) return; 
    lv_obj_scroll_by(scroll->Obj,0,-step,LV_ANIM_ON); 
    lv_event_stop_processing(e); }
    else if(key == LV_KEY_UP) { 
    if(lv_obj_get_scroll_y(scroll->Obj) <= scroll->END_Y) return; 
    lv_obj_scroll_by(scroll->Obj,0,step,LV_ANIM_ON); 
    lv_event_stop_processing(e); }
}

static void The_En_RegisterEscObj(lv_obj_t* cont, Chart_Data_t* data);
static void ThePage_En_ChartEsc_cb(lv_event_t* e);

static void The_En_RegisterEscObj(lv_obj_t* cont, Chart_Data_t* data)
{
    s_the_en_esc_obj = cont;
    lv_group_t *g = app_keypad_get_group();
    if (g) { lv_group_add_obj(g, cont); lv_group_focus_obj(cont); }
    lv_obj_add_event_cb(cont, ThePage_En_ChartEsc_cb, LV_EVENT_KEY, data);
}

static void ThePage_En_ChartEsc_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        Chart_Data_t* data = (Chart_Data_t*)lv_event_get_user_data(e);
        if (data) {
            if (data->Timer_Treat) { lv_timer_del(data->Timer_Treat); data->Timer_Treat = NULL; }
            if (data->Timer_Back) { lv_timer_del(data->Timer_Back); data->Timer_Back = NULL; }
        }
        
        therapy_stop();
        The_En_StopTimer();
        Child_ThePage_En_LeaveGroup();
        PelFlo_ThePage_En_Load(MenPage);
        return;
    }
}


/* 确认框键盘：ESC=否（关闭弹窗） */
static void The_En_Page5MsgBox_Key_cb(lv_event_t* e)
{
    uint32_t key = lv_event_get_key(e);
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    lv_obj_t* obj = lv_event_get_target(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);
    if(code == LV_EVENT_KEY) {
        if(key == LV_KEY_ESC) {
            if (ctx->Window.Msgbox && lv_obj_is_valid(ctx->Window.Msgbox)){
                lv_msgbox_close_async(ctx->Window.Msgbox);
            }
        } else if(key == LV_KEY_LEFT) {
            if(obj == ctx->Window.No_Btn)  lv_group_focus_obj(ctx->Window.Yes_Btn);
        } else if(key == LV_KEY_RIGHT) {
            if(obj == ctx->Window.Yes_Btn)  lv_group_focus_obj(ctx->Window.No_Btn);
        }
    }
}

/* Page5 返回确认：显示 Back_En_Window + 是/否 进 group（默认聚焦"Yes"），ESC=否 */
void The_En_Page5_ShowConfirm(Treat_Timer_Ctx_t* ctx)
{
    Back_En_Window(&ctx->Window, "Confirm Return?");
    lv_group_t *g = app_keypad_get_group();
    if (g && lv_obj_is_valid(ctx->Window.Yes_Btn)) {
        lv_group_add_obj(g, ctx->Window.Yes_Btn);
        lv_group_add_obj(g, ctx->Window.No_Btn);
        lv_group_focus_obj(ctx->Window.Yes_Btn);
    }
    lv_obj_add_event_cb(ctx->Window.Yes_Btn, Back_Btn_Event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(ctx->Window.No_Btn,  Back_Btn_Event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(ctx->Window.Yes_Btn, The_En_Page5MsgBox_Key_cb, LV_EVENT_KEY, ctx);
    lv_obj_add_event_cb(ctx->Window.No_Btn,  The_En_Page5MsgBox_Key_cb, LV_EVENT_KEY, ctx);
}

/* ============================================================================
 * 治疗页 9 子页 widget 前向声明
 * ==========================================================================*/
static void ThePage1_widget(Instr_Data_t* param, lv_obj_t* page_cont);
static void ThePage2_widget(Select_Data_t* param, lv_obj_t* page_cont);
static void ThePage3_widget(TreIns_Data_t* param, lv_obj_t* page_cont);
static void ThePage4_widget(Param_Data_t* param, lv_obj_t* page_cont);
static void ThePage5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont);
static void ThePage6_widget(Chart_Data_t* param, lv_obj_t* page_cont);
static void ThePage7_widget(Chart_Data_t* param, lv_obj_t* page_cont);
static void ThePage8_widget(Chart_Data_t* param, lv_obj_t* page_cont);

static void ThePage_Page1_Key_cb(lv_event_t* e);
static void ThePage_Page2_Key_cb(lv_event_t* e);
static void ThePage_Page3_Key_cb(lv_event_t* e);
static void ThePage_Page4_Key_cb(lv_event_t* e);
static void ThePage_Page5_Key_cb(lv_event_t* e);

/* ============================================================================
 * 数据实例（英文文本取自客户定稿 EDA_EMG_LVGL_PC_EN；保留 esp32 30min 会话时长）
 * ==========================================================================*/

Instr_Data_t    The_En_Instr_data1 = {
    .Obj_Hig = 235,
    .Obj_Wid = 260,
    .Title = "Pelvic Floor Muscle Therapy",
    .Text = "Applicable to mild & moderate stress urinary incontinence, "
            "stress-dominant mixed urinary incontinence; Grade I/II anterior & "
            "posterior vaginal wall prolapse (cystocele, rectocele); Grade I/II "
            "uterine prolapse. Relieves chronic pelvic pain; adjuvant therapy for "
            "chronic pelvic inflammatory disease and vaginismus.",
    .Title_Color = FONT_RED_COLOR,
};
static Stage_Desc_t Stage_Tbl[STAGE_MAX] = {
    [STAGE_1] = {"Stage 1", 8},
    [STAGE_2] = {"Stage 2", 5},
    [STAGE_3] = {"Stage 3", 5},
    [STAGE_4] = {"Stage 4", 7},
};

Select_Data_t   The_En_Select_data1 = {
    .High_Obj = 900,
    .Pos_Y = -32,
    .Sta_Table = Stage_Tbl,
    .Stage_Num = 4,
};
TreIns_Data_t   The_En_Treins_data1 = {
    .Obj_Hig = 125,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "History",
    .Label_Btn2 = "Start",
    .Title = "Stage 1 •",
    .Text = "Stage 1 : 8 treatment sessions, 30 mins each.\n"
            "Therapy: Electrical Stimulation\n"
            "Electrical stimulation: Passive treatment, no voluntary\n"
            "contraction.",
};
TreIns_Data_t   The_En_Treins_data2 = {
    .Obj_Hig = 200,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "History",
    .Label_Btn2 = "Start",
    .Title = "Stage 2 •",
    .Text = "Stage 2 : 5 treatment sessions, 30 mins each.\n"
            "Combined therapies: Electrical Stimulation + "
            "Conditional Stimulation\n"
            "Electrical stimulation: Passive treatment, no voluntary "
            "contraction.\n"
            "Conditional stimulation: Actively control curve within "
            "guided range via graphics & voice; stimulation triggers "
            "when out of range.",
};
TreIns_Data_t   The_En_Treins_data3 = {
    .Obj_Hig = 240,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "History",
    .Label_Btn2 = "Start",
    .Title = "Stage 3 •",
    .Text = "Stage 3 : 5 treatment sessions, 30 mins each.\n"
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
TreIns_Data_t   The_En_Treins_data4 = {
    .Obj_Hig = 185,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "History",
    .Label_Btn2 = "Start",
    .Title = "Stage 4 • ",
    .Text = "Stage 4 : 7 treatment sessions, 30 mins each.\n"
            "Combined therapies: Electrical Stimulation + Biofeedback\n"
            "Electrical Stimulation: Passive treatment, no voluntary contraction\n"
            "Biofeedback: Actively control curve within guided range via graphics & voice.",
};

TreIns_Data_t* The_En_Treins_list[STAGE_MAX] = {
   &The_En_Treins_data1,
   &The_En_Treins_data2,
   &The_En_Treins_data3,
   &The_En_Treins_data4,
};
Param_Data_t The_En_Param_Data1 = {
    .Value_Intens1 = 1,
    .Value_Intens2 = 1,
    .Value_Freq = 4,
    .Value_Pulse = 500,
    .Slder_Color = FONT_RED_COLOR ,
    .Text_Set1 = "Setup Complete, Next",
    .Text_Set2 = "Setup Complete, Start",
    .Style_KnobColor = &style_RedKnob,
};
Param_Data_t* The_En_Param_list[STAGE_MAX] = {
    &The_En_Param_Data1,
    &The_En_Param_Data1,
    &The_En_Param_Data1,
    &The_En_Param_Data1,
};
Treat_Timer_Ctx_t ThePage_En_CtxData1 = {
    .total_sec = 30 * 60 * 1000,   /* [我们] 阶段C：处方3疗程1 单次 30 分钟 */
    .remain_sec = 30 * 60 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 1,
};
Treat_Timer_Ctx_t ThePage_En_CtxData2 = {
    .total_sec = 30 * 60 * 1000,
    .remain_sec = 30 * 60 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 2,
};
Treat_Timer_Ctx_t ThePage_En_CtxData3 = {
    .total_sec = 30 * 60 * 1000,
    .remain_sec = 30 * 60 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 3,
};
Treat_Timer_Ctx_t* ThePage_En_CtxData[STAGE_MAX] = {
    &ThePage_En_CtxData1,
    &ThePage_En_CtxData2,
    &ThePage_En_CtxData3,
    &ThePage_En_CtxData1,
};
Chart_Data_t The_En_Chart_Data1 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t The_En_Chart_Data2 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t The_En_Chart_Data3 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t The_En_Chart_Data4 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t* The_En_Chart_Data_List[STAGE_MAX] = {
    &The_En_Chart_Data1,
    &The_En_Chart_Data2,
    &The_En_Chart_Data3,
    &The_En_Chart_Data4,
};
static char title_buf[20];
/**
 * @brief 盆底治疗子页面创建入口（英文版）
 */
void Child_ThePage_En_Load(ChiThePage_En_ID_t page_id)
{
    The_En_StopTimer();
    Child_ThePage_En_LeaveGroup();
    Page_Clean();
    uint8_t idx = Get_CurStage_Idx();
    switch (page_id) {
    case ChiThePage_Instr:
        ThePage1_widget(&The_En_Instr_data1, g_Ui.page_container);
        break;
    case ChiThePage_StaSel:
        ThePage2_widget(&The_En_Select_data1, g_Ui.page_container);
        break;
    case ChiThePage_TreIns:
        uint8_t num = Get_CurTime_Idx();
        lv_snprintf(title_buf, sizeof(title_buf), " Treatment %d", num);
        The_En_Treins_list[idx]->Title_time = title_buf;
        ThePage3_widget(The_En_Treins_list[idx], g_Ui.page_container);
        break;
    case ChiThePage_ParSet:
    {
        uint8_t schemes_idx = Get_CurTreat_Idx();
        The_En_Param_Data1.Value_Intens1 = 0;
        The_En_Param_Data1.Value_Intens2 = 0;             
        The_En_Param_Data1.stage_stim = therapy_cur_stage_stim(&g_rx_all[RX_3]->schemes[schemes_idx]);
        The_En_Param_Data1.Value_Freq =therapy_cur_freq(&g_rx_all[RX_3]->schemes[schemes_idx],1);
        The_En_Param_Data1.Value_Pulse = therapy_cur_pw(&g_rx_all[RX_3]->schemes[schemes_idx],1);
        ThePage4_widget(&The_En_Param_Data1, g_Ui.page_container);
        audio_play_request(VOICE_ID_MAX_INTENSITY_EN,1500);              //语音播放：调节强度
        break;
    }
    case ChiThePage_Treat:
        {
         uint8_t schemes_idx = Get_CurTreat_Idx();
        ThePage_En_CtxData1.treat_data.Value_Freq =therapy_cur_freq(&g_rx_all[RX_3]->schemes[schemes_idx],1);
        ThePage_En_CtxData1.treat_data.Value_Pulse =therapy_cur_pw(&g_rx_all[RX_3]->schemes[schemes_idx],1);
        ThePage_En_CtxData1.treat_data.Intens_Value =The_En_Param_Data1.Value_Intens1;
        ThePage_En_CtxData1.total_sec = therapy_get_stim_time(&g_rx_all[RX_3]->schemes[schemes_idx]);
        ThePage5_widget(&ThePage_En_CtxData1, g_Ui.page_container);
        break;
    }
    case ChiThePage_Chart1:
         emg_force_reset();
         emg_force_start_acq(500, 1);
         ThePage6_widget(&The_En_Chart_Data1, g_Ui.page_container);
         Chart_Init_Point(The_En_Chart_Data1.Chart);
         if (idx == STAGE_4) therapy_init_chart_instr(&The_En_Chart_Data1);
         else therapy_init_chart(&The_En_Chart_Data1);
         break;
    case ChiThePage_Chart2:
        emg_force_reset();
        emg_force_start_acq(500, 1);
        ThePage7_widget(&The_En_Chart_Data2, g_Ui.page_container);
        Chart_Init_Point(The_En_Chart_Data2.Chart);
        break;
    case ChiThePage_Chart3:
        ThePage8_widget(&The_En_Chart_Data3, g_Ui.page_container);
        break;
    default:
        break;
    }
}

/**
 * @brief 治疗页面（四宫格「治疗」格入口，英文版）
 */
void Child_ThePage_En_ui(void)
{
    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(page_cont);

    ThePage1_widget(&The_En_Instr_data1, page_cont);
}

/* ============================================================================
 * [我们] 各页 KEY 导航回调（通用守则）
 * ==========================================================================*/

static void ThePage_Page1_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        PelFlo_ThePage_En_Load(MenPage);
    }
}

static uint8_t The_FindStage(Select_Data_t* data, lv_obj_t* btn)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (data->sta[i].btn == btn) return i;
    }
    return data->Btn_MaxSum;
}

static void The_StageUnSel(Select_Data_t* data)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (lv_obj_is_valid(data->sta[i].obj)) lv_obj_remove_style(data->sta[i].obj, &style_gradient, LV_PART_MAIN);
        if (lv_obj_is_valid(data->sta[i].img)) lv_obj_set_style_image_recolor(data->sta[i].img, lv_color_hex(FONT_GRAY_COLOR), 0);
    }
}

static void ThePage_Page2_Key_cb(lv_event_t* e)
{
    Select_Data_t* data = (Select_Data_t*)lv_event_get_user_data(e);
    lv_obj_t* obj = lv_event_get_target(e);

    if (lv_event_get_code(e) == LV_EVENT_FOCUSED) {
        uint8_t num = The_FindStage(data, obj);
        if (num < data->Btn_MaxSum) {
            The_StageUnSel(data);
            if (lv_obj_is_valid(data->sta[num].obj)) lv_obj_add_style(data->sta[num].obj, &style_gradient, LV_PART_MAIN);
            if (lv_obj_is_valid(data->sta[num].img)) lv_obj_set_style_image_recolor(data->sta[num].img, lv_color_hex(FONT_RED_COLOR), 0);
        }
        return;
    }
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;

    uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ESC) { Child_ThePage_En_Load(ChiThePage_Instr); return; }

    uint8_t num = The_FindStage(data, obj);
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

static void ThePage_Page3_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_ThePage_En_Load(ChiThePage_StaSel); return; }

    if (key == LV_KEY_LEFT && obj == data->Start_Btn) lv_group_focus_obj(data->History_Btn);
    else if (key == LV_KEY_RIGHT && obj == data->History_Btn) lv_group_focus_obj(data->Start_Btn);
}

static void ThePage_Page4_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_ThePage_En_Load(ChiThePage_TreIns); return; }

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

   /* else if (key == LV_KEY_LEFT) {
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

static void ThePage_Page5_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) {
        if (ctx->remain_sec == 0) {
            The_En_BackToMenu();
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
 * 9 子页 widget（英文共享控件 + 按键导航注册）
 * ==========================================================================*/

static void ThePage8_widget(Chart_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    Chart_En_widget(cont, param);
    lv_obj_add_event_cb(param->Chart, Chart_Draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, param);
    if (param->Timer_Treat) { lv_timer_del(param->Timer_Treat); param->Timer_Treat = NULL; }
    param->tick = 0;
    param->Timer_Treat = lv_timer_create(Chart3_Stage3_En_Add_data, 10, param);
    The_En_RegisterEscObj(cont, param);
}

static void ThePage7_widget(Chart_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    /* 按指示训练的引导波形 */
    Therapy_Guide_Init(cont);
    Chart_En_widget(cont, param);
    lv_obj_add_event_cb(param->Chart, Chart_Draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, param);

    /* 防止旧 timer 残留 */
    if (param->Timer_Treat) {
        lv_timer_del(param->Timer_Treat);
        param->Timer_Treat = NULL;
    }
    param->tick = 0;
    param->Timer_Treat = lv_timer_create(Chart3_Stage3_En_Add_data, 10, param);
    therapy_init_chart_instr(param);
    The_En_RegisterEscObj(cont, param);
}


static void ThePage6_widget(Chart_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    /*
     * 初始化 Chart1 的引导线。
     * Stage2 / Stage3 / Stage4 都使用这一层。
     */
    Therapy_Guide_LineInit(cont);
    Chart_En_widget(cont, param);
    lv_obj_add_event_cb(param->Chart, Chart_Draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, param);
    /*
     * 防止上一个曲线页面的 timer 没清掉。
     */
    if (param->Timer_Treat) {
        lv_timer_del(param->Timer_Treat);
        param->Timer_Treat = NULL;
    }
    param->tick = 0;
    if (Get_CurStage_Idx() == STAGE_3) {
        param->Timer_Treat = lv_timer_create(Chart_Stage3_En_Add_data, 10, param);
    }
    else {
        param->Timer_Treat = lv_timer_create(Chart_Stage2_4_En_Add_data, 10, param);
    }
    The_En_RegisterEscObj(cont, param);
}


static void ThePage5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont)
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

    The_En_GroupRegister(param->treat_data.Plu_Btn);
    The_En_GroupRegister(param->treat_data.Min_Btn);
    The_En_GroupRegister(param->treat_data.Pause_Btn);
    The_En_GroupRegister(param->treat_data.Start_Btn);
    The_En_GroupRegister(param->treat_data.Back_Btn);
    lv_group_focus_obj(param->treat_data.Start_Btn);
    lv_obj_add_event_cb(param->treat_data.Plu_Btn,   ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Min_Btn,   ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Pause_Btn, ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn,  ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
}

static void ThePage4_widget(Param_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    ParSet_En_Widget(page_cont, param);
    lv_obj_add_event_cb(param->Set_Btn, ChildPage_P4SetBtn_En_Event_cb, LV_EVENT_CLICKED, param);

    The_En_GroupRegister(param->Min_Btn);
    The_En_GroupRegister(param->Plu_Btn);
    The_En_GroupRegister(param->Set_Btn);
    lv_group_focus_obj(param->Plu_Btn);
    lv_obj_add_event_cb(param->Min_Btn, ThePage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Plu_Btn, ThePage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Set_Btn, ThePage_Page4_Key_cb, LV_EVENT_KEY, param);
}

static void ThePage3_widget(TreIns_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    /* 页面滚动设置 */
    The_En_TreIns_Scroll.Obj = cont;
    The_En_TreIns_Scroll.Origin_Y = 210;
    The_En_TreIns_Scroll.END_Y = 0;
    param->Scroll = &The_En_TreIns_Scroll;

    TreIns_En_Widget(cont, param);
    lv_obj_add_event_cb(param->History_Btn, ChildPage_TreIns_En_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->Start_Btn,   ChildPage_TreIns_En_Event_cb, LV_EVENT_CLICKED, param);

    The_En_GroupRegister(param->History_Btn);
    The_En_GroupRegister(param->Start_Btn);
    lv_group_focus_obj(param->Start_Btn);
    lv_obj_add_event_cb(param->History_Btn, ThePage_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Start_Btn,   ThePage_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->History_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
    lv_obj_add_event_cb(param->Start_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
}

static void ThePage2_widget(Select_Data_t* param, lv_obj_t* page_cont)
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
        The_En_GroupRegister(param->sta[i].btn);
        lv_obj_add_event_cb(param->sta[i].btn, ThePage_Page2_Key_cb, LV_EVENT_ALL, param);
    }
    if (param->Btn_MaxSum > 0) lv_group_focus_obj(param->sta[0].btn);
}

static void ThePage1_widget(Instr_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    Instr_En_Widget(cont, param);

    lv_obj_add_event_cb(param->Btn, Child_Page_StartBtn_En_Event_cb, LV_EVENT_ALL, param);

    The_En_GroupRegister(param->Btn);
    lv_group_focus_obj(param->Btn);
    lv_obj_add_event_cb(param->Btn, ThePage_Page1_Key_cb, LV_EVENT_KEY, param);
}
