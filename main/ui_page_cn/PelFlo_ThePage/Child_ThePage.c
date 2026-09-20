#include "Child_ThePage.h"
#include "PelFlo_The_ui.h"
#include "PelFlo_TheChild_event.h"
#include "menu_ui.h"
#include "menu_ui_event_cb.h"   /* [我们] Chart_Draw_event_cb / Chart3_Stage3_Add_data / Chart_Stage2_4_Add_data（B5-B） */
#include "basic.h"
#include "app_keypad.h"         /* [我们] keypad group 注册 */
#include "app_therapy.h"        /* [我们] 阶段C：治疗控制（退出/ESC 停引擎） */
#include <string.h>             /* [我们] memset */
#include "emg_force.h"     /* 阶段 3.3: 实时力度包络 */
#include "emg_dsp.h"       /* EMG_DSP_RAW16_LSB_UV 换算 */
#include "esp_log.h"         /* [我们] stub 日志 */
#include "voice.h"
/* ============================================================================
 * [我们] B5-C 通用守则基础设施（坑 6.8 / B2-4 / B3-3 同族，参照 PosReh/Ass）
 * ==========================================================================*/
static char *TAG = "Child_ThePage";
#define THE_GROUP_MAX 48
static lv_obj_t* s_the_objs[THE_GROUP_MAX];   /* 本页已注册进 group 的对象 */
static int s_the_cnt = 0;
static lv_obj_t* s_the_esc_obj = NULL;        /* 曲线图页无按钮时的 ESC 焦点对象 */

/* [我们] 数据实例数组前向声明（定义见文件下部，The_StopTimer 先引用） */
extern Treat_Timer_Ctx_t* ThePage_CtxData[STAGE_MAX];
extern Chart_Data_t* The_Chart_Data_List[STAGE_MAX];
Scroll_Data_t The_TreIns_Scroll;           //评估页滚动页面数据
/* 注册对象进 keypad group 并记录（切页时统一移出，防 refocus FOCUSED 悬垂） */
static void The_GroupRegister(lv_obj_t* obj)
{
    lv_group_t *g = app_keypad_get_group();
    if (g && obj) {
        lv_group_add_obj(g, obj);
        if (s_the_cnt < THE_GROUP_MAX)
            s_the_objs[s_the_cnt++] = obj;
    }
}

/* 把本页注册的对象移出 group（lv_obj_is_valid 防悬垂，坑 B2-4） */
void Child_ThePage_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        for (int i = 0; i < s_the_cnt; i++) {
            if (s_the_objs[i] && lv_obj_is_valid(s_the_objs[i]))
                lv_group_remove_obj(s_the_objs[i]);
        }
        if (s_the_esc_obj && lv_obj_is_valid(s_the_esc_obj))
            lv_group_remove_obj(s_the_esc_obj);
    }
    s_the_cnt = 0;
    s_the_esc_obj = NULL;
}

/* 停止治疗/曲线图全部定时器（页面退出/切页前调用） */
void The_StopTimer(void)
{
    for (int i = 0; i < STAGE_MAX; i++) {
        if (ThePage_CtxData[i]->timer_Treat) { lv_timer_del(ThePage_CtxData[i]->timer_Treat); ThePage_CtxData[i]->timer_Treat = NULL; }
        if (ThePage_CtxData[i]->timer_Next)  { lv_timer_del(ThePage_CtxData[i]->timer_Next);  ThePage_CtxData[i]->timer_Next  = NULL; }
        ThePage_CtxData[i]->status = TIMER_PAUSED;
    }
    for (int i = 0; i < STAGE_MAX; i++) {
        if (The_Chart_Data_List[i]) {
            if (The_Chart_Data_List[i]->Timer_Treat) { lv_timer_del(The_Chart_Data_List[i]->Timer_Treat); The_Chart_Data_List[i]->Timer_Treat = NULL; }
            if (The_Chart_Data_List[i]->Timer_Back)  { lv_timer_del(The_Chart_Data_List[i]->Timer_Back);  The_Chart_Data_List[i]->Timer_Back  = NULL; }
        }
    }
    emg_force_stop_acq();
}

/* 统一退出：停定时器 + 移出 group + 回主菜单（删除路径调用后须提前 return，坑 B4-3） */
void The_BackToMenu(void)
{
    The_StopTimer();              /* 先删定时器 + 停采集(stop_push)，保证下面停波走确认事务 */
    therapy_stop();               /* [我们] 阶段C：停治疗引擎（停波 + 会话复位）。
                                   * 安全关键：此前被注释导致键盘ESC"已完成"直接回菜单时
                                   * 底层 STM32 仍持续输出（STM32 无超时/看门狗自保），必须恢复 */
    Child_ThePage_LeaveGroup();
    Goto_MenuPage();
}

/* 曲线图页：注册页面容器承接 ESC（参照 Ass 页 s_page_esc_obj 模式） */
static void The_RegisterEscObj(lv_obj_t* cont, Chart_Data_t* data);
static void ThePage_ChartEsc_cb(lv_event_t* e);

static void The_RegisterEscObj(lv_obj_t* cont, Chart_Data_t* data)
{
    s_the_esc_obj = cont;
    lv_group_t *g = app_keypad_get_group();
    if (g) { lv_group_add_obj(g, cont); lv_group_focus_obj(cont); }
    lv_obj_add_event_cb(cont, ThePage_ChartEsc_cb, LV_EVENT_KEY, data);
}

/* 曲线图页 ESC：停曲线定时器 + 回四宫格（删除路径调用后须提前 return） */
static void ThePage_ChartEsc_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        Chart_Data_t* data = (Chart_Data_t*)lv_event_get_user_data(e);
        if (data) {
            if (data->Timer_Treat) { lv_timer_del(data->Timer_Treat); data->Timer_Treat = NULL; }
            if (data->Timer_Back)  { lv_timer_del(data->Timer_Back);  data->Timer_Back  = NULL; }
        }
        The_StopTimer();
        Child_ThePage_LeaveGroup();
        PelFlo_ThePage_Load(MenPage);
    }
    // emg_force_stop_acq();
}

/* 确认框键盘：ESC=否（关闭弹窗，页面对象仍在 group → 焦点自然回落到页面） */
static void The_Page5MsgBox_Key_cb(lv_event_t* e)
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

/* Page5 返回确认：显示 Back_Window + 是/否 进 group（默认聚焦“是”），ESC=否 */
void The_Page5_ShowConfirm(Treat_Timer_Ctx_t* ctx)
{
    Back_Window(&ctx->Window, "是否确认返回？");
    lv_group_t *g = app_keypad_get_group();
    if (g && lv_obj_is_valid(ctx->Window.Yes_Btn)) {
        lv_group_add_obj(g, ctx->Window.Yes_Btn);
        lv_group_add_obj(g, ctx->Window.No_Btn);
        lv_group_focus_obj(ctx->Window.Yes_Btn);
    }
    lv_obj_add_event_cb(ctx->Window.Yes_Btn, Back_Btn_Event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(ctx->Window.No_Btn,  Back_Btn_Event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(ctx->Window.Yes_Btn, The_Page5MsgBox_Key_cb, LV_EVENT_KEY, ctx);
    lv_obj_add_event_cb(ctx->Window.No_Btn,  The_Page5MsgBox_Key_cb, LV_EVENT_KEY, ctx);
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

/* 各页 KEY 导航回调（[我们] B5-C，通用守则全应用） */
static void ThePage_Page1_Key_cb(lv_event_t* e);
static void ThePage_Page2_Key_cb(lv_event_t* e);
static void ThePage_Page3_Key_cb(lv_event_t* e);
static void ThePage_Page4_Key_cb(lv_event_t* e);
static void ThePage_Page5_Key_cb(lv_event_t* e);

/* ============================================================================
 * 数据实例（移植自 PC Child_ThePage.c；PC 死代码 g_Treat_Points/g_Treat_Point_cnt 已删）
 * ==========================================================================*/

/* 治疗功能介绍结构体 */
Instr_Data_t    The_Instr_data1 = {
    .Obj_Hig = 220,
    .Obj_Wid = 252,
    .Title = "盆底肌治疗",
    .Text = "适用于轻、中度压力性尿失禁,以压力性尿失禁为主的混合性尿失禁;I、11度阴道前后壁膨出("
             "膀胱脱垂、直肠脱垂);I、Ⅱ度子宫脱垂。同时可缓解慢性盆腔疼痛,辅助治疗慢性盆腔炎、阴道痉挛。",
    .Title_Color = FONT_RED_COLOR,
};
static Stage_Desc_t Stage_Tbl[STAGE_MAX] = {
    [STAGE_1] = {"阶段一", 8},
    [STAGE_2] = {"阶段二", 5},
    [STAGE_3] = {"阶段三", 5},
    [STAGE_4] = {"阶段四", 7},
};

Select_Data_t   The_Select_data1 = {
    .High_Obj = 900,         /* 根据按钮数量决定设置容器的高度 */
    .Sta_Table = Stage_Tbl,  /* [我们] PC 写 &Stage_Tbl（指针到数组）→ GCC -Werror incompatible-pointer-types，改数组名（自动衰减） */
    .Stage_Num = 4,
};
TreIns_Data_t   The_Treins_data1 = {
    .Obj_Hig = 120,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "历史",
    .Label_Btn2 = "开始治疗",
    .Title = "阶段一·",
    .Text = "阶段一共计8次治疗,单次时长30分钟。本阶段采用电刺激疗法,"
            "治疗过程中您无需主动发力,被动配合即可。",
};
TreIns_Data_t   The_Treins_data2 = {
    .Obj_Hig = 170,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "历史",
    .Label_Btn2 = "开始治疗",
    .Title = "阶段二·",
    .Text = "阶段二共计5次治疗,单次时长30分钟。本阶段采用电刺激 + 条件性电刺激疗法,"
            "电刺激治疗过程中您无需主动发力,被动配合即可,条件性电刺激需要您主动参与,依"
            "照图示和语音指引将曲线保持在提示范围内,若偏离范围会触发电刺激。",
};
TreIns_Data_t   The_Treins_data3 = {
    .Obj_Hig = 200,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "历史",
    .Label_Btn2 = "开始治疗",
    .Title = "阶段三·",
    .Text = "阶段三共计5次治疗,单次时长30分钟。本阶段采用电刺激+条件性电刺激+生物反馈疗法,"
            "电刺激治疗过程中您无需主动发力,被动配合即可,条件性电刺激需要您主动参与,依照图"
            "示和语音指引将曲线保持在提示范围内,若偏离范围会触发电刺激,生物反馈需要您主动参"
            "与,依照图示和语音指引将曲线保持在提示范围内。",
};
TreIns_Data_t   The_Treins_data4 = {
    .Obj_Hig = 170,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "历史",
    .Label_Btn2 = "开始治疗",
    .Title = "阶段四·",
    .Text = "阶段四共计7次治疗,单次时长30分钟。本阶段采用电刺激 + 生物反馈疗法,"
            "电刺激治疗过程中您无需主动发力,被动配合即可,生物反馈需要您主动参"
            "与,依照图示和语音指引将曲线保持在提示范围内。",
};

/* 阶段疗程说明结构体数组 */
TreIns_Data_t* The_Treins_list[STAGE_MAX] = {
   &The_Treins_data1,
   &The_Treins_data2,
   &The_Treins_data3,
   &The_Treins_data4,
};
Param_Data_t The_Param_Data1 = {
    .Value_Intens1 = 1,
    .Value_Intens2 = 1,
    .Value_Freq = 40,
    .Value_Pulse = 400,
    .Slder_Color = FONT_RED_COLOR ,
    .Text_Set1 = "设置完成，下一步",
    .Text_Set2 = "设置完成，进入治疗",
    .Style_KnobColor = &style_RedKnob,
};

Treat_Timer_Ctx_t ThePage_CtxData1 = {
    .total_sec = 30 * 60 * 1000,   /* [我们] 阶段C：处方3疗程1 单次 30 分钟 */
    .remain_sec = 30 * 60 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 1,
};
Treat_Timer_Ctx_t ThePage_CtxData2 = {
    .total_sec = 30 * 60 * 1000,   /* [我们] 阶段C：单次 30 分钟 */
    .remain_sec = 30 * 60 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 2,
};
Treat_Timer_Ctx_t ThePage_CtxData3 = {
    .total_sec = 30 * 60 * 1000,   /* [我们] 阶段C：单次 30 分钟 */
    .remain_sec = 30 * 60 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 3,
};
/* 治疗参数结构体数组 */
Treat_Timer_Ctx_t* ThePage_CtxData[STAGE_MAX] = {
    &ThePage_CtxData1,
    &ThePage_CtxData2,
    &ThePage_CtxData3,
    &ThePage_CtxData1,
};
Chart_Data_t The_Chart_Data1 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t The_Chart_Data2 = {
    .Value_MaxEMG = 50.8,
    .Value_InsEMG = 50.8,
    .Value_TimeLeft = 250,
};
Chart_Data_t* The_Chart_Data_List[STAGE_MAX] = {
    NULL,
    &The_Chart_Data1,
    &The_Chart_Data2,
    &The_Chart_Data2,
};
static char title_buf[9];
/**
 * @brief 盆底治疗子页面创建入口
 * @param page_id
 */
void Child_ThePage_Load(ChiThePage_ID_t page_id)
{
    The_StopTimer();
    Child_ThePage_LeaveGroup();        /* [我们] 先移出 group 再 Page_Clean（坑 6.8） */
    Page_Clean();
    uint8_t idx = Get_CurStage_Idx();
    switch (page_id) {
    case ChiThePage_Instr:
        ThePage1_widget(&The_Instr_data1, g_Ui.page_container);
        break;
    case ChiThePage_StaSel:
        ThePage2_widget(&The_Select_data1, g_Ui.page_container);
        break;
    case ChiThePage_TreIns:
        uint8_t num = Get_CurTime_Idx();
        lv_snprintf(title_buf, sizeof(title_buf), "治疗%d", num);
        The_Treins_list[idx]->Title_time = title_buf;
        ThePage3_widget(The_Treins_list[idx], g_Ui.page_container);
        break;
    case ChiThePage_ParSet:
        uint8_t schemes_idx = Get_CurTreat_Idx();
        The_Param_Data1.Value_Intens1 = 0;
        The_Param_Data1.Value_Intens2 = 0;            
        The_Param_Data1.stage_stim = therapy_cur_stage_stim(&g_rx_all[RX_3]->schemes[schemes_idx]);
        The_Param_Data1.Value_Freq = therapy_cur_freq(&g_rx_all[RX_3]->schemes[schemes_idx],1);
        The_Param_Data1.Value_Pulse = therapy_cur_pw(&g_rx_all[RX_3]->schemes[schemes_idx],1);
        ThePage4_widget(&The_Param_Data1, g_Ui.page_container);
        audio_play_request(VOICE_ID_MAX_INTENSITY_CN,1500);              //语音播放：调节强度
        break;
    case ChiThePage_Treat:
        uint8_t schemes_idx2 = Get_CurTreat_Idx();
        ThePage_CtxData1.treat_data.Value_Freq = therapy_cur_freq(&g_rx_all[RX_3]->schemes[schemes_idx2],1);
        ThePage_CtxData1.treat_data.Value_Pulse = therapy_cur_pw(&g_rx_all[RX_3]->schemes[schemes_idx2],1);
        ThePage_CtxData1.treat_data.Intens_Value = The_Param_Data1.Value_Intens1;
        ThePage_CtxData1.total_sec = therapy_get_stim_time(&g_rx_all[RX_3]->schemes[schemes_idx2]);
        ThePage5_widget(&ThePage_CtxData1, g_Ui.page_container);
        break;
    case ChiThePage_Chart1: //治疗阶段二、三、四
        emg_force_reset();                              /* 清零滤波器/包络状态 */
        emg_force_start_acq(500, 1);                   // 启动采集 (500SPS, CH1)
        ThePage6_widget(&The_Chart_Data1, g_Ui.page_container);
        Chart_Init_Point(The_Chart_Data1.Chart);
        if (idx == STAGE_4) therapy_init_chart_instr(&The_Chart_Data1);
        else therapy_init_chart(&The_Chart_Data1);
        break;
    case ChiThePage_Chart2: //治疗阶段三
        emg_force_reset();                              /* 清零滤波器/包络状态 */
        emg_force_start_acq(500, 1);                   // 启动采集 (500SPS, CH1)
        ThePage7_widget(&The_Chart_Data2, g_Ui.page_container);
        Chart_Init_Point(The_Chart_Data2.Chart);
        break;
    case ChiThePage_Chart3: //治疗阶段三
        ThePage8_widget(The_Chart_Data_List[idx], g_Ui.page_container);
        break;
    default:
        break;
    }

}

/**
 * @brief 治疗页面（四宫格「治疗」格入口）
 * @param  none
 */
void Child_ThePage_ui(void)
{

    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(page_cont);
    

    ThePage1_widget(&The_Instr_data1, page_cont);
   
}

/* ============================================================================
 * [我们] 各页 KEY 导航回调（通用守则：ESC 返回 / 2D 导航 / 删对象路径不用 stop_processing）
 * ==========================================================================*/

/* Page1 说明页：ESC 回四宫格；ENTER 由 Child_Page_StartBtn_Event_cb 处理 */
static void ThePage_Page1_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        PelFlo_ThePage_Load(MenPage);   /* 删对象路径：后续不再访问任何对象（坑 B4-3） */
    }
}

/* 疗程网格：查找当前按钮索引 */
static uint8_t The_FindStage(Select_Data_t* data, lv_obj_t* btn)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (data->sta[i].btn == btn) return i;
    }
    return data->Btn_MaxSum;
}

/* 疗程网格：全部格设未选中样式 */
static void The_StageUnSel(Select_Data_t* data)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (lv_obj_is_valid(data->sta[i].obj)) lv_obj_remove_style(data->sta[i].obj, &style_gradient, LV_PART_MAIN);
        if (lv_obj_is_valid(data->sta[i].img)) lv_obj_set_style_image_recolor(data->sta[i].img, lv_color_hex(FONT_GRAY_COLOR), 0);
    }
}

/* Page2 疗程网格：FOCUSED 高亮 + KEY 2D 导航 + ESC 回 Page1（3 格/行，Flex 换行） */
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
    if (key == LV_KEY_ESC) { Child_ThePage_Load(ChiThePage_Instr); return; }   /* 删对象路径，提前 return */

    uint8_t num = The_FindStage(data, obj);
    if (num >= data->Btn_MaxSum) return;

    const uint8_t per_row = 3;                    /* 480 宽 / (128+25) → 每行 3 格 */
    uint8_t target = 0xFF;
    if (key == LV_KEY_LEFT && num % per_row != 0)                      target = num - 1;
    else if (key == LV_KEY_RIGHT && num % per_row != per_row - 1)      target = num + 1;
    else if (key == LV_KEY_UP && num >= per_row)                       target = num - per_row;
    else if (key == LV_KEY_DOWN && num + per_row < data->Btn_MaxSum)   target = num + per_row;

    if (target != 0xFF && lv_obj_is_valid(data->sta[target].btn)) {
        lv_group_focus_obj(data->sta[target].btn);
        lv_event_stop_processing(e);   /* 纯导航分支非删对象，stop 无害 */
    }
}

/* Page3 治疗说明：ESC 回 Page2，LEFT/RIGHT 切 历史/开始 */
static void ThePage_Page3_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_ThePage_Load(ChiThePage_StaSel); return; }   /* 删对象路径 */

    if (key == LV_KEY_LEFT && obj == data->Start_Btn) lv_group_focus_obj(data->History_Btn);
    else if (key == LV_KEY_RIGHT && obj == data->History_Btn) lv_group_focus_obj(data->Start_Btn);
}

/* Page4 参数设置：滑条不进 group（B4-2）；减/加/设置 进 group；
 * 左右=调档位、上下=线性切按钮(减→加→设置)、ENTER=动作、ESC 回 Page3 */
static void ThePage_Page4_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_ThePage_Load(ChiThePage_TreIns); return; }   /* 删对象路径 */

    // /* 左右 = 调档位（滑条值 ±1） */
    // if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
    //     int32_t v = lv_slider_get_value(data->Slider);
    //     v += (key == LV_KEY_RIGHT) ? 1 : -1;
    //     if (v < 0) v = 0;
    //     if (v > 90) v = 90;
    //     lv_slider_set_value(data->Slider, v, LV_ANIM_OFF);
    //     /* [我们] LVGL9.5 lv_slider_set_value(ANIM_OFF) 不发 VALUE_CHANGED（lv_bar.c 确认），
    //      * 手动更新强度值 + 标签（同 Child_Slider_Event_cb VALUE_CHANGED 分支） */
    //     if (data->Value_Stage == 1)      data->Value_Intens1 = v;
    //     else if (data->Value_Stage == 2) data->Value_Intens2 = v;
    //     if (lv_obj_is_valid(data->Label_Intens)) lv_label_set_text_fmt(data->Label_Intens, "%d", (int)v);
    //     lv_event_stop_processing(e);   /* 调值分支非删对象，stop 无害（坑 B3-3 只禁删对象路径） */
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

/* Page5 治疗中：LEFT/RIGHT 切焦点（同 PosReh_Page5），ENTER 由 ChildPage_P5Btn_Event_cb 处理，ESC=返回确认 */
static void ThePage_Page5_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) {
        if (ctx->remain_sec == 0) {
            The_BackToMenu();      /* 删对象路径，后续不再访问 */
        }
        else {
            /* [我们] 先删 timer 再弹确认（确认“是”→Back_Btn_Event_cb→Goto_MenuPage 不删 timer，
             * 定时器会继续访问已删对象 → use-after-free；ESC 取消则治疗已暂停需重新开始）
             * 注：B6-A 发现并修复，与保养格一致 */
            // if (ctx->timer_Treat) { lv_timer_del(ctx->timer_Treat); ctx->timer_Treat = NULL; }
            // ctx->status = TIMER_PAUSED;
            therapy_pause();      /* [我们] 阶段C：ESC 确认前先暂停引擎（停波），确认“是”后 Back_Btn_Event_cb 再 stop */
            The_Page5_ShowConfirm(ctx);
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
 * 9 子页 widget（移植自 PC Child_ThePage.c；含 [我们] 按键导航注册）
 * ==========================================================================*/

/**
 * @brief 曲线图3（阶段三结束时弹窗询问返回）
 */
static void ThePage8_widget(Chart_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);

    Chart_widget(cont, param);
    lv_obj_add_event_cb(param->Chart, Chart_Draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, param);
    if (param->Timer_Treat){
        lv_timer_del(param->Timer_Treat);
        param->Timer_Treat = NULL;
    }
    param->tick = 0;
    param->Timer_Treat = lv_timer_create(Chart3_Stage3_Add_data, 10, param);
    The_RegisterEscObj(cont, param);        /* [我们] 承接 ESC */
}

/**
 * @brief 曲线图2（阶段三结束后自动进入曲线图3）
 */
static void ThePage7_widget(Chart_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_style_pad_all(cont, 0, 0);

    Therapy_Guide_Init(cont);      //初始化引导波形层
    Chart_widget(cont, param);
    lv_obj_add_event_cb(param->Chart, Chart_Draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, param);
    if (param->Timer_Treat){
        lv_timer_del(param->Timer_Treat);
        param->Timer_Treat = NULL;
    }
    param->tick = 0;
    param->Timer_Treat = lv_timer_create(Chart3_Stage3_Add_data, 50, param);
    therapy_init_chart_instr(param);
    The_RegisterEscObj(cont, param);        /* [我们] 承接 ESC */
}

/**
 * @brief 曲线图1（阶段二/三/四；阶段三用 Chart_Stage3_Add_data，其它用 Chart_Stage2_4_Add_data）
 */
static void ThePage6_widget(Chart_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_style_pad_all(cont, 0, 0);

    Therapy_Guide_LineInit(cont);      //初始化引导波形层
    Chart_widget(cont, param);
    lv_obj_add_event_cb(param->Chart, Chart_Draw_event_cb, LV_EVENT_DRAW_TASK_ADDED, param);
    if (param->Timer_Treat){
        lv_timer_del(param->Timer_Treat);
        param->Timer_Treat = NULL;
    }
    param->tick = 0;   /* [我们] 每次进 chart 页重置 tick（ESC 退出再进不残留） */
    if (Get_CurStage_Idx() == STAGE_3) {
        param->Timer_Treat = lv_timer_create(Chart_Stage3_Add_data, 50, param);
    }
    else {
        param->Timer_Treat = lv_timer_create(Chart_Stage2_4_Add_data, 50, param);
    }
    The_RegisterEscObj(cont, param);        /* [我们] 承接 ESC */
}
/**
 * @brief 电刺激治疗页（暂停/开始/返回 各自注册 CLICKED）
 */
static void ThePage5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont)
{
   
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cont, 0, 0);
    /*创建治疗页面 暂停、开始、返回、是/否按钮点击事件单独添加
      回调函数其他控件的聚焦点击事件都统一在菜单事件回调函数里*/
    Treat_Widget(cont, param);

    lv_obj_add_event_cb(param->treat_data.Pause_Btn, ChildPage_P5Btn_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, ChildPage_P5Btn_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn, ChildPage_P5Btn_Event_cb, LV_EVENT_CLICKED, param);

    /* ===== [我们] 按键导航：5 按钮进 group + 默认焦点“开始” + KEY 导航 ===== */
    The_GroupRegister(param->treat_data.Plu_Btn);
    The_GroupRegister(param->treat_data.Min_Btn);
    The_GroupRegister(param->treat_data.Pause_Btn);
    The_GroupRegister(param->treat_data.Start_Btn);
    The_GroupRegister(param->treat_data.Back_Btn);
    lv_group_focus_obj(param->treat_data.Start_Btn);
    lv_obj_add_event_cb(param->treat_data.Plu_Btn,   ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Min_Btn,   ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Pause_Btn, ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn,  ThePage_Page5_Key_cb, LV_EVENT_KEY, param);
}

/**
 * @brief 电刺激参数设置页（设置按钮单独注册 CLICKED）
 */
static void ThePage4_widget(Param_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_style_pad_all(cont, 0, 0);
    /*绘制参数设置页面 设置按钮点击事件单独添加回调函数
      其他控件的聚焦点击事件都统一在菜单事件回调函数里*/
    ParSet_Widget(page_cont, param);
    lv_obj_add_event_cb(param->Set_Btn, ChildPage_P4SetBtn_Event_cb, LV_EVENT_CLICKED, param);    //将加按钮添加聚焦事件回调

    /* ===== [我们] 按键导航：滑条不进 group（B4-2）；减/加/设置 进 group + KEY 导航 ===== */
    The_GroupRegister(param->Min_Btn);
    The_GroupRegister(param->Plu_Btn);
    The_GroupRegister(param->Set_Btn);
    lv_group_focus_obj(param->Set_Btn);
    lv_obj_add_event_cb(param->Min_Btn, ThePage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Plu_Btn, ThePage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Set_Btn, ThePage_Page4_Key_cb, LV_EVENT_KEY, param);
}

/**
 * @brief 阶段治疗说明页（历史/开始 单独注册 CLICKED）
 */
static void ThePage3_widget(TreIns_Data_t* param, lv_obj_t* page_cont)
{

    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                 //启用滑动条
    lv_obj_set_style_pad_all(cont, 0, 0);

    /* 页面滚动设置*/
    The_TreIns_Scroll.Obj = cont;
    The_TreIns_Scroll.Origin_Y = 210;
    The_TreIns_Scroll.END_Y = 0;
    param->Scroll = &The_TreIns_Scroll;

    TreIns_Widget(cont, param);
    lv_obj_add_event_cb(param->History_Btn, ChildPage_TreIns_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->Start_Btn, ChildPage_TreIns_Event_cb, LV_EVENT_CLICKED, param);

    /* ===== [我们] 按键导航：历史/开始 进 group + 默认焦点“开始” + KEY 导航 ===== */
    The_GroupRegister(param->History_Btn);
    The_GroupRegister(param->Start_Btn);
    lv_group_focus_obj(param->Start_Btn);
    lv_obj_add_event_cb(param->History_Btn, ThePage_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Start_Btn,   ThePage_Page3_Key_cb, LV_EVENT_KEY, param);
     /* 新增：绑定按键上下滚动页面 */
    lv_obj_add_event_cb(param->History_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
    lv_obj_add_event_cb(param->Start_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
}

/**
 * @brief 阶段疗程选择页（网格按钮逐个注册）
 */
static void ThePage2_widget(Select_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                 //启用滑动条
    lv_obj_set_style_pad_all(cont, 0, 0);

    Select_Widget2(cont, param);
    for (uint8_t i = 0; i < param->Btn_MaxSum; i++) {
        lv_obj_add_event_cb(param->sta[i].btn, ChildPage_StageBtn_Event_cb, LV_EVENT_ALL, param); //将按钮添加获得焦点事件
        /* ===== [我们] 按键导航：网格按钮进 group + FOCUSED 高亮 + KEY 2D 导航 =====
         * 注意：此时 Btn_MaxSum 已由 Select_Widget2 设置，sta[] 全部有效，
         * 空 group 首个 add 自动聚焦触发 FOCUSED 访问 sta 安全（坑 B4-1 已规避） */
        The_GroupRegister(param->sta[i].btn);
        lv_obj_add_event_cb(param->sta[i].btn, ThePage_Page2_Key_cb, LV_EVENT_ALL, param);
    }
    if (param->Btn_MaxSum > 0) lv_group_focus_obj(param->sta[0].btn);
}

/**
 * @brief 疗程说明页（选择疗程按钮 注册 CLICKED）
 */
static void ThePage1_widget(Instr_Data_t* param, lv_obj_t* page_cont)
{
  
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_style_pad_all(cont, 0, 0);
    Instr_Widget(cont, param);

    lv_obj_add_event_cb(param->Btn, Child_Page_StartBtn_Event_cb, LV_EVENT_ALL, param);

    /* ===== [我们] 按键导航：选择疗程按钮 进 group + 默认焦点 + KEY 导航（ESC 回四宫格） ===== */
    The_GroupRegister(param->Btn);
    lv_group_focus_obj(param->Btn);
    lv_obj_add_event_cb(param->Btn, ThePage_Page1_Key_cb, LV_EVENT_KEY, param);
}
