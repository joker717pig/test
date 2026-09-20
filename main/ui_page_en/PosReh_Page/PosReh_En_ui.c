/**
  ******************************************************************************
  * @文件名称   PosReh_En_ui.c
  * @文件描述   英文产后康复页实现（对应中文 ui_page_cn/PosReh_Page/PosReh_Page_ui.c）。
  *            独立数据实例（英文文本）+ 独立页面函数；共享控件用英文版
  *            shared_En_ui.c（TreIns_Widget 无硬编码中文，直接复用中文版）。
  *            页面管理结构 / 当前功能状态复用中文版（extern），NVS 参数存储共享。
  ******************************************************************************
  */

#include "PosReh_En_ui.h"
#include "PosReh_En_ui_event.h"
#include "shared_En_ui.h"
#include "menu_ui.h"
#include "menu_ui_event_cb.h"
#include "basic.h"
#include "app_keypad.h"   /* keypad group 注册 */
#include "app_params.h"   /* 参数存储（NVS） */
#include "esp_log.h"
#include <string.h>       /* memset */
#include "app_therapy.h"  /*接入处方*/
#include "therapy_pn.h"
#include "voice.h"

static const char *TAG = "PosRehEn";

static Scroll_Data_t PosReh_En_TreIns_Scroll;

/* static 前向声明（页面创建函数定义在文件后部） */
static void PosReh_En_Page1_widget(PosReh_Widget_t* widget, lv_obj_t* page_cont);
static void PosReh_En_Page2_widget(PosReh_Widget_t* widget, lv_obj_t* page_cont);
static void PosReh_En_Page3_widget(TreIns_Data_t* param, lv_obj_t* page_cont);
static void PosReh_En_Page4_widget(Param_Data_t* param, lv_obj_t* page_cont);
static void PosReh_En_Page5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont);

/* 页面管理结构 extern（定义在中文 PosReh_Page_ui.c，全局唯一） */
extern lv_my_ui_page_date_t lv_my_ui_pagePosReh_t;
extern lv_my_child_page_date_t lv_my_PosRehPage1_t;
extern lv_my_child_page_date_t lv_my_PosRehPage2_t;
extern lv_my_child_page_date_t lv_my_PosRehPage3_t;
extern lv_my_child_page_date_t lv_my_PosRehPage4_t;
extern lv_my_child_page_date_t lv_my_PosRehPage5_t;
extern lv_my_child_page_date_t lv_my_PosRehPage6_t;

PosReh_Widget_t PosReh_En_Widget;

/*========================== 疗程说明数据（英文） ==========================*/
TreIns_Data_t   PosReh_En_Treins_data1 = {
    .Obj_Hig = 150,
    .Obj_Wid = 404,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_YELLOW_COLOR,
    .Label_Btn1 = "Single",
    .Label_Btn2 = "10 Session",
    .Title = "Diastasis Recti",
    .Title_time = "",
    .Text = "Triggers passive abdominal muscle contraction to "
            "repair separated rectus abdominis and restore normal "
            "abdominal shape. 30 mins per session.\n"
            "Mode: Electrical Stimulation -- no voluntary "
            "contraction needed, just relax passively.",
    
};
TreIns_Data_t   PosReh_En_Treins_data2 = {
    .Obj_Hig = 150,
    .Obj_Wid = 404,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_YELLOW_COLOR,
    .Label_Btn1 = "Single",
    .Label_Btn2 = "10 Session",
    .Title = "Uterine Involution",
    .Title_time = "",
    .Text = "Helps stimulate uterine contractions and promote "
            "postpartum uterine recovery. 30 mins per session.\n"
            "Mode: Electrical Stimulation -- no voluntary "
            "contraction needed, just relax passively.",
    
};
TreIns_Data_t   PosReh_En_Treins_data3 = {
    .Obj_Hig = 150,
    .Obj_Wid = 404,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_YELLOW_COLOR,
    .Label_Btn1 = "Single",
    .Label_Btn2 = "10 Session",
    .Title = "Promote lactation",
    .Title_time = "",
    .Text = "Boost milk secretion. 30 mins per session.\n"
            "Electrical stimulation mode: no voluntary "
            "contraction required, just relax passively.",
    
};
TreIns_Data_t* PosReh_En_Treins_list[Func_SUM] = {
   &PosReh_En_Treins_data1,
   &PosReh_En_Treins_data2,
   &PosReh_En_Treins_data3,
};

/*========================== 电刺激参数设置数据（英文） ==========================*/
Param_Data_t PosReh_En_Param_Data1 = {
    .Value_Freq = 4,
    .Value_Pulse = 500,
    .Slder_Color = FONT_YELLOW_COLOR, 
    .Text_Set1 = "Setup Complete, Next",
    .Text_Set2 = "Setup Complete, Start",
    .Style_KnobColor = &style_YellowKnob,
};
Param_Data_t PosReh_En_Param_Data2 = {
    .Value_Freq = 4,
    .Value_Pulse = 500,
    .Slder_Color = FONT_YELLOW_COLOR,
    .Text_Set1 = "Setup Complete, Next",
    .Text_Set2 = "Setup Complete, Start",
    .Style_KnobColor = &style_YellowKnob,
};
Param_Data_t PosReh_En_Param_Data3 = {
    .Value_Freq = 4,
    .Value_Pulse = 500,
    .Slder_Color = FONT_YELLOW_COLOR,
    .Text_Set1 = "Setup Complete, Next",
    .Text_Set2 = "Setup Complete, Start",
    .Style_KnobColor = &style_YellowKnob,
};
Param_Data_t* PosReh_En_Param_list[Func_SUM] = {
    &PosReh_En_Param_Data1,
    &PosReh_En_Param_Data2,
    &PosReh_En_Param_Data3,
};

/*========================== 电刺激治疗上下文（英文，复用中文页面管理结构） ==========================*/
Treat_Timer_Ctx_t PosReh_En_CtxData1 = {
    .total_sec = 10 * 1000,
    .remain_sec = 5 * 1000,
    .status = TIMER_IDLE,
    .treat_data = {
        .Color = FONT_YELLOW_COLOR,
        .Intens_Value = 0,
    },
    
};
Treat_Timer_Ctx_t PosReh_En_CtxData2 = {
    .total_sec = 10 * 1000,
    .remain_sec = 7 * 1000,
    .status = TIMER_IDLE,
    .treat_data = {
        .Color = FONT_YELLOW_COLOR,
        .Intens_Value = 0,
    },
    
};
Treat_Timer_Ctx_t PosReh_En_CtxData3 = {
    .total_sec = 10 * 1000,
    .remain_sec = 9 * 1000,
    .status = TIMER_IDLE,
    .treat_data = {
        .Color = FONT_YELLOW_COLOR,
        .Intens_Value = 0,
    },
    
};
Treat_Timer_Ctx_t* PosReh_En_CtxData[Func_SUM] = {
    &PosReh_En_CtxData1,
    &PosReh_En_CtxData2,
    &PosReh_En_CtxData3,
};

static bool s_treat_params_en_loaded = false;

/**
 * @brief 把 NVS 已存治疗参数应用到英文数据实例（只应用一次，首次进入英文产后页时）
 */
void PosReh_En_Param_ApplySaved(void)
{
    if (s_treat_params_en_loaded) return;
    s_treat_params_en_loaded = true;

    for (int i = 0; i < Func_SUM && i < APP_PARAMS_TREAT_NUM; i++) {
        treat_param_t t;
        if (app_params_get_treat(i, &t) != ESP_OK) continue;
        Param_Data_t *p = PosReh_En_Param_list[i];
        if (p == NULL) continue;
        p->Value_Pulse   = (uint32_t)t.pulse;
        p->Value_Freq    = (uint32_t)t.freq;
        p->Value_Intens1 = t.intens1;
        p->Value_Intens2 = t.intens2;
        p->Value_Stage   = t.stage;
    }
    ESP_LOGI(TAG, "treat params applied from NVS (EN)");
}

/**
 * @brief 保存 3 个功能的治疗参数（英文数据实例）到 NVS
 */
void PosReh_En_Param_SaveAll(void)
{
    for (int i = 0; i < Func_SUM && i < APP_PARAMS_TREAT_NUM; i++) {
        Param_Data_t *p = PosReh_En_Param_list[i];
        if (p == NULL) continue;
        treat_param_t t = {
            .intens1 = p->Value_Intens1,
            .intens2 = p->Value_Intens2,
            .freq    = (int32_t)p->Value_Freq,
            .pulse   = (int32_t)p->Value_Pulse,
            .stage   = p->Value_Stage,
        };
        app_params_set_treat(i, &t);
        app_params_save_treat(i);
    }
}

/* =====  keypad group 管理 + 通用守则清理 ===== */
#define POSREH_EN_GROUP_MAX 12
static lv_obj_t* s_posreh_en_objs[POSREH_EN_GROUP_MAX];
static int s_posreh_en_cnt = 0;

static void PosReh_En_GroupRegister(lv_obj_t* obj)
{
    lv_group_t *g = app_keypad_get_group();
    if (g && obj) {
        lv_group_add_obj(g, obj);
        if (s_posreh_en_cnt < POSREH_EN_GROUP_MAX)
            s_posreh_en_objs[s_posreh_en_cnt++] = obj;
    }
}

void PosReh_En_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        for (int i = 0; i < s_posreh_en_cnt; i++) {
            if (s_posreh_en_objs[i] && lv_obj_is_valid(s_posreh_en_objs[i]))
                lv_group_remove_obj(s_posreh_en_objs[i]);
        }
    }
    s_posreh_en_cnt = 0;
}

void PosReh_En_StopTimer(void)
{
    for (int i = 0; i < Func_SUM; i++) {
        if (PosReh_En_CtxData[i]->timer_Treat) {
            lv_timer_del(PosReh_En_CtxData[i]->timer_Treat);
            PosReh_En_CtxData[i]->timer_Treat = NULL;
        }
        PosReh_En_CtxData[i]->status = TIMER_PAUSED;
    }
}

/* 统一退出：停定时器 + 移出 group + 保存参数 + 回主菜单（按语言自动分叉） */
void PosReh_En_BackToMenu(void)
{
    PosReh_En_StopTimer();
    PosReh_En_LeaveGroup();
    PosReh_En_Param_SaveAll();
    memset(&PosReh_En_Widget, 0, sizeof(PosReh_En_Widget));
    Goto_MenuPage();
}

static void page_scroll_key_cb(lv_event_t* e)
{
    Scroll_Data_t* scroll = (Scroll_Data_t*)lv_event_get_user_data(e);
    uint32_t key = lv_event_get_key(e);
    int16_t step = 35;   

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
 * @brief 英文产后康复子页面创建入口
 */
void PosReh_En_Page_Load(PosReh_PageID_t page_id)
{
    PosReh_En_LeaveGroup();     /* 先移出 group 再 Page_Clean（坑 6.8） */
    Page_Clean();
    memset(&PosReh_En_Widget, 0, sizeof(PosReh_En_Widget));
    PosReh_En_Param_ApplySaved();   /* 首次进入应用 NVS 已存治疗参数 */
    uint8_t func = PosReh_Get_CurFunc();
    switch (page_id) {
    case PosRehPage1:
        PosReh_En_Page1_widget(&PosReh_En_Widget, g_Ui.page_container);
        break;
    case PosRehPage2:
        PosReh_En_Page2_widget(&PosReh_En_Widget, g_Ui.page_container);
        break;
    case PosRehPage3:
        PosReh_En_Page3_widget(PosReh_En_Treins_list[func], g_Ui.page_container);
        break;
    case PosRehPage4:
        PosReh_En_Param_list[func]->Value_Intens1 = 0;
        PosReh_En_Param_list[func]->stage_stim = therapy_cur_stage_stim(&rx_pn.schemes[func]);
        PosReh_En_Param_list[func]->Value_Freq = therapy_cur_freq(&rx_pn.schemes[func], 1);
        PosReh_En_Param_list[func]->Value_Pulse = therapy_cur_pw(&rx_pn.schemes[func], 1);
        PosReh_En_Param_list[func]->Value_Stage = 1;
        PosReh_En_Page4_widget(PosReh_En_Param_list[func], g_Ui.page_container);
        audio_play_request(VOICE_ID_MAX_INTENSITY_EN,1500);              //语音播放：调节强度
        break;
    case PosRehPage5:
        PosReh_En_CtxData[func]->total_sec = rx_pn.schemes[func].stage_time[0];
        PosReh_En_CtxData[func]->remain_sec = PosReh_En_CtxData[func]->total_sec;
        PosReh_En_CtxData[func]->treat_data.Value_Freq = therapy_cur_freq(&rx_pn.schemes[func], 1);
        PosReh_En_CtxData[func]->treat_data.Value_Pulse = therapy_cur_pw(&rx_pn.schemes[func], 1);
        PosReh_En_CtxData[func]->treat_data.Intens_Value = PosReh_En_Param_list[func]->Value_Intens1;
        PosReh_En_Page5_widget(PosReh_En_CtxData[func], g_Ui.page_container);
        break;
    case PosRehPage6:
        break;
    default:
        break;
    }
}

/**
 * @brief 英文产后康复第5页 电刺激治疗
 */
static void PosReh_En_Page5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    Treat_En_Widget(cont, param);   /* 英文共享控件 */
    /* 暂停/开始/返回 点击事件放在本文件（同中文） */
    lv_obj_add_event_cb(param->treat_data.Pause_Btn, PosReh_En_P5Btn_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, PosReh_En_P5Btn_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn,  PosReh_En_P5Btn_Event_cb, LV_EVENT_CLICKED, param);

    /* keypad：Plu/Min/Pause/Start/Back 进 group + KEY 导航 + 默认聚焦"开始" */
    PosReh_En_GroupRegister(param->treat_data.Plu_Btn);
    PosReh_En_GroupRegister(param->treat_data.Min_Btn);
    PosReh_En_GroupRegister(param->treat_data.Pause_Btn);
    PosReh_En_GroupRegister(param->treat_data.Start_Btn);
    PosReh_En_GroupRegister(param->treat_data.Back_Btn);
    lv_obj_add_event_cb(param->treat_data.Plu_Btn,   PosReh_En_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Min_Btn,   PosReh_En_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Pause_Btn, PosReh_En_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, PosReh_En_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn,  PosReh_En_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_group_focus_obj(param->treat_data.Start_Btn);
}

/**
 * @brief 英文产后康复第4页 电刺激参数设置
 */
static void PosReh_En_Page4_widget(Param_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    ParSet_En_Widget(cont, param);      /* 英文共享控件 */
    lv_obj_add_event_cb(param->Set_Btn, PosReh_En_P4SetBtn_Event_cb, LV_EVENT_CLICKED, param);

    /* keypad：减/加/设置 进 group（滑条不进，同中文） */
    PosReh_En_GroupRegister(param->Min_Btn);
    PosReh_En_GroupRegister(param->Plu_Btn);
    PosReh_En_GroupRegister(param->Set_Btn);
    lv_obj_add_event_cb(param->Min_Btn, PosReh_En_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Plu_Btn, PosReh_En_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Set_Btn, PosReh_En_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_group_focus_obj(param->Plu_Btn);
}

/**
 * @brief 英文产后康复第3页 疗程介绍页（复用中文 TreIns_Widget，文本来自英文数据）
 */
static void PosReh_En_Page3_widget(TreIns_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    /* 页面滚动设置 */
    PosReh_En_TreIns_Scroll.Obj = cont;
    PosReh_En_TreIns_Scroll.Origin_Y = 210;
    PosReh_En_TreIns_Scroll.END_Y = 0;
    param->Scroll = &PosReh_En_TreIns_Scroll;

    TreIns_En_Widget(cont, param);         /* 英文共享控件（字体/坐标对齐客户定稿） */
    lv_obj_add_event_cb(param->History_Btn, PosReh_En_TreIns_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->Start_Btn,   PosReh_En_TreIns_Event_cb, LV_EVENT_CLICKED, param);

    /* keypad：History/Start 进 group + KEY 导航 + 默认聚焦"开始" */
    PosReh_En_GroupRegister(param->History_Btn);
    PosReh_En_GroupRegister(param->Start_Btn);
    lv_obj_add_event_cb(param->History_Btn, PosReh_En_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Start_Btn,   PosReh_En_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_group_focus_obj(param->Start_Btn);
    lv_obj_add_event_cb(param->History_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
    lv_obj_add_event_cb(param->Start_Btn,  page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
}

/**
 * @brief 英文产后康复第2页 疗程选择
 */
static void PosReh_En_Page2_widget(PosReh_Widget_t* widget, lv_obj_t* page_cont)
{
    char* text_no[3] = {"01","02","03"};
    char* text_name[3] = { "Diastasis\nRecti","Uterine\nInvolution","Lactation\nPromotion" };
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);
    /*标题*/
    lv_obj_t* title = Creat_Label(cont, "Postnatal Rehabilitation", &lv_font_montserrat_26);
    lv_obj_align(title, LV_ALIGN_TOP_MID,0,20);
    lv_obj_set_style_text_color(title, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /*创建3个疗程选择控件*/
    for (int i = 0; i < Func_SUM; i++)
    {
        lv_obj_t* Obj = Create_Obj(cont, 110, 180, 0xffffff, 0);
        lv_obj_add_style(Obj, &style_gradient, LV_PART_MAIN);
        lv_obj_add_style(Obj, &style_shadow, LV_PART_MAIN);
        lv_obj_set_pos(Obj, 45+(i*135), 60);
        widget->SelTreat_Data[i].Obj_Func = Obj;
        lv_obj_t* Obj1 = Create_Obj(Obj, 100, 100, 0xffffff, 0);
        lv_obj_align(Obj1, LV_ALIGN_CENTER, 0, 20);
        lv_obj_set_style_radius(Obj1, 60, LV_PART_MAIN);
        lv_obj_add_style(Obj1, &style_gradient_gray, LV_PART_MAIN);
        /*选中显示控件*/
        lv_obj_t* Obj2 = Create_Obj(Obj1, 90, 90, 0xFFFffff, 1);
        lv_obj_align(Obj2, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_radius(Obj2, 45, LV_PART_MAIN);
        lv_obj_set_style_border_opa(Obj2, 100, LV_PART_MAIN);
        lv_obj_set_style_border_width(Obj2, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(Obj2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        widget->SelTreat_Data[i].Obj_Select = Obj2;
        lv_obj_t* btn = Create_Button(Obj2, 90, 90, FONT_GRAY_COLOR, 45, 1);
        lv_obj_center(btn);
        widget->SelTreat_Data[i].Btn_Func = btn;

        lv_obj_add_event_cb(btn, PosReh_En_SelTreatPage_Event_cb, LV_EVENT_ALL, widget);
        /* keypad：功能按钮进 group + KEY 导航 */
        PosReh_En_GroupRegister(btn);
        lv_obj_add_event_cb(btn, PosReh_En_Page2_Key_cb, LV_EVENT_KEY, widget);

        lv_obj_t* label = Creat_Label(Obj, text_no[i], &lv_font_montserrat_30);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 15);
        lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        widget->SelTreat_Data[i].Label_NO = label;
        lv_obj_t* label2 = Creat_Label(Obj, text_name[i], &lv_font_montserrat_14);
        lv_obj_set_width(label2, 90);
        lv_obj_set_style_text_align(label2, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_align(label2, LV_ALIGN_CENTER, 0, 23);
        lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        widget->SelTreat_Data[i].Label_Func = label2;
    }
    /* [我们] 默认聚焦当前功能 */
    lv_group_focus_obj(widget->SelTreat_Data[PosReh_Get_CurFunc()].Btn_Func);
}

/**
 * @brief 英文产后康复第1页 说明页
 */
static void PosReh_En_Page1_widget(PosReh_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    PosReh_Instr_Data_t insData = {
        .Title = "Description of Postnatal Rehabilitation",
        .Text = "Postpartum rehabilitation includes three core projects: "
               "lactation, uterine involution, and rectus abdominis muscle "
               "separation and repair.",
        .src = WOMEN_ICON,
    };
    widget->InsData = insData;

    /*说明框*/
    lv_obj_t* Obj = Create_Obj(cont, 256, 160, 0xffffff, 0);
    lv_obj_set_pos(Obj, 25, -4);
    lv_obj_add_style(Obj, &style_shadow, LV_PART_MAIN);
    lv_obj_add_style(Obj, &style_gradient, LV_PART_MAIN);
    /*标题*/
    lv_obj_t* title = Creat_Label(Obj, widget->InsData.Title, &lv_font_montserrat_16);
    lv_obj_set_pos(title, 15, 2);
    lv_obj_set_width(title, 250);
    lv_obj_set_style_text_color(title, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    /*正文*/
    lv_obj_t* label = Creat_Label(Obj, "•", &lv_font_montserrat_12);
    lv_obj_set_pos(label, 18, 43);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text = Creat_TextLabel(Obj, 225, 105, widget->InsData.Text, &lv_font_montserrat_12);
    lv_obj_set_pos(text, 25, 35);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(text, 4, 0);

    /*图片*/
    lv_obj_t* Img = Creat_Image(cont, widget->InsData.src);
    lv_obj_set_pos(Img, 291, 8);

    /*选择疗程按钮*/
    lv_obj_t* btn = Create_Button(cont, 110, 40, FONT_YELLOW_COLOR, 30, 0);
    lv_obj_set_pos(btn, 305, 200);
    lv_obj_set_style_shadow_opa(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    lv_obj_t* label0 = Creat_Label(btn, "Select", &lv_font_montserrat_14);
    lv_obj_center(label0);
    lv_obj_set_style_text_color(label0, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    widget->InsData.Btn_Select = btn;
    widget->InsData.Label_Select = label0;
    lv_obj_add_event_cb(btn, PosReh_En_InstrPage_Event_cb, LV_EVENT_ALL, widget);

    /*腹直肌分离治疗按钮*/
    lv_obj_t* btn1 = Create_Button(cont, 222, 21, FONT_WHITE_COLOR, 0, 0);
    lv_obj_align_to(btn1, Obj, LV_ALIGN_OUT_BOTTOM_MID,0,29);
    lv_obj_set_style_shadow_opa(btn1, 0, LV_PART_MAIN);
    lv_obj_t* label1 = Creat_Label(btn1, "Diastasis Recti                       8 Left", &lv_font_montserrat_12);
    lv_obj_center(label1);
    lv_obj_set_style_text_color(label1, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    widget->InsData.Btn_DiaRecti = btn1;
    widget->InsData.Label_DiaRecti = label1;
    lv_obj_add_event_cb(btn1, PosReh_En_InstrPage_Event_cb, LV_EVENT_ALL, widget);
    /*催乳治疗按钮*/
    lv_obj_t* btn2 = Create_Button(cont, 222, 21, FONT_WHITE_COLOR, 0, 0);
    lv_obj_align_to(btn2, btn1, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_style_shadow_opa(btn2, 0, LV_PART_MAIN);
    lv_obj_t* label2 = Creat_Label(btn2, "Uterine Involution               6 Left", &lv_font_montserrat_12);
    lv_obj_center(label2);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    widget->InsData.Btn_UterInvo = btn2;
    widget->InsData.Label_UterInvo = label2;
    lv_obj_add_event_cb(btn2, PosReh_En_InstrPage_Event_cb, LV_EVENT_ALL, widget);
    /*子宫复旧治疗按钮*/
    lv_obj_t* btn3 = Create_Button(cont, 222, 21, FONT_WHITE_COLOR, 0, 0);
    lv_obj_align_to(btn3, btn2, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_style_shadow_opa(btn3, 0, LV_PART_MAIN);
    lv_obj_t* label3 = Creat_Label(btn3, " Lactation Promotion         3 Left", &lv_font_montserrat_12);
    lv_obj_center(label3);
    lv_obj_set_style_text_color(label3, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    widget->InsData.Btn_LacPro = btn3;
    widget->InsData.Label_LacPro = label3;
    lv_obj_add_event_cb(btn3, PosReh_En_InstrPage_Event_cb, LV_EVENT_ALL, widget);

    /* keypad：4 按钮进 group + KEY 导航 + 默认聚焦"选择" */
    PosReh_En_GroupRegister(btn);
    PosReh_En_GroupRegister(btn1);
    PosReh_En_GroupRegister(btn2);
    PosReh_En_GroupRegister(btn3);
    lv_obj_add_event_cb(btn,  PosReh_En_Page1_Key_cb, LV_EVENT_KEY, widget);
    lv_obj_add_event_cb(btn1, PosReh_En_Page1_Key_cb, LV_EVENT_KEY, widget);
    lv_obj_add_event_cb(btn2, PosReh_En_Page1_Key_cb, LV_EVENT_KEY, widget);
    lv_obj_add_event_cb(btn3, PosReh_En_Page1_Key_cb, LV_EVENT_KEY, widget);
    lv_group_focus_obj(btn);
}
