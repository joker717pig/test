#include "PosReh_Page_ui.h"
#include "PosReh_Page_ui_event.h"
#include "menu_ui.h"
#include "basic.h"
#include "lang.h"
#include "app_keypad.h"   /* [我们] keypad group 注册 */
#include "app_params.h"   /* [我们] 参数存储（NVS） */
#include "esp_log.h"      /* [我们] 日志 */
#include <string.h>       /* [我们] memset */
#include "app_therapy.h"
#include "therapy_pn.h"
#include "esp_log.h"         /* [我们] stub 日志 */
#include "voice.h"

static const char *TAG = "PosReh";    /* [我们] 日志 TAG */


static void PosReh_Page1_widget(PosReh_Widget_t* widget, lv_obj_t* page_cont);
static void PosReh_Page2_widget(PosReh_Widget_t* widget, lv_obj_t* page_cont);
static void PosReh_Page5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont);
static void PosReh_Page4_widget(Param_Data_t* param, lv_obj_t* page_cont);
static void PosReh_Page3_widget(TreIns_Data_t* param, lv_obj_t* page_cont);

PosReh_Widget_t PosReh_Widget;
TreIns_Data_t   PosReh_Treins_data;       //阶段治疗说明结构体变量
Param_Data_t    PosReh_Param_data;        //电刺激参数设置结构体变量

PosReh_Func_Name_t g_Cur_Func = DiaRecti;     //初始化当前治疗功能为 腹直肌分离
Scroll_Data_t PosReh_TreIns_Scroll;           //评估页滚动页面数据
char* TreIns_Title[Func_SUM] = {
    "腹直肌分离","子宫复旧","催乳"
};

char* TreIns_Text[Func_SUM] = {
    "刺激腹部肌肉被动收缩,修复分离肌群,重塑腹部正常状态。单次时长30分钟。"
    "该次训练采用电刺激疗法,电刺激治疗过程中您无需主动发力,被动配合即可。",
    "帮助子宫收缩,促进产后子宫恢复。单次时长30分钟。该次训练采用电刺激疗法,"
    "电刺激治疗过程中您无需主动发力,被动配合即可。",
    "促进分泌乳汁。单次时长30分钟。该次训练采用电刺激疗法,电刺激治疗过程中您无需主动发力,被动配合即可。"
};
/*=========================="腹直肌分离","子宫复旧","催乳" 疗程说明数据初始化==========================*/
Treat_Data_t      PosReh_Treat_data;        //电刺激治疗结构体变量
Treat_Timer_Ctx_t PosReh_Ctx;             // 电刺激治疗初始化上下文
JumpToBackPage_t  PosReh_PageData;       //页面跳转结构变量

Treat_Timer_Ctx_t PosReh_CtxData1 = {
    .total_sec = 10 * 1000,
    .remain_sec = 5 * 1000,
    .status = TIMER_IDLE,
    .treat_data = {
        .Color = FONT_YELLOW_COLOR,
        .Intens_Value = 0,
    },
    /* [我们] B5-A 对齐 PC 新版：删 Main_Page/Page 跳转字段 */
};
Treat_Timer_Ctx_t PosReh_CtxData2 = {
    .total_sec = 10 * 1000,
    .remain_sec = 7 * 1000,
    .status = TIMER_IDLE,
    .treat_data = {
        .Color = FONT_YELLOW_COLOR,
        .Intens_Value = 0,
    },
    /* [我们] B5-A 对齐 PC 新版：删 Main_Page/Page 跳转字段 */
};
Treat_Timer_Ctx_t PosReh_CtxData3 = {
    .total_sec = 10 * 1000,
    .remain_sec = 9 * 1000,
    .status = TIMER_IDLE,
    .treat_data = {
        .Color = FONT_YELLOW_COLOR,
        .Intens_Value = 0,
    },
    /* [我们] B5-A 对齐 PC 新版：删 Main_Page/Page 跳转字段 */
};
Treat_Timer_Ctx_t* PosReh_CtxData[Func_SUM] = {
    &PosReh_CtxData1,
    &PosReh_CtxData2,
    &PosReh_CtxData3,
};
Param_Data_t PosReh_Param_Data1 = {
    .Value_Intens1 = 1,
    .Value_Intens2 = 0,
    .Value_Freq = 1,
    .Value_Pulse = 500,
    .Slder_Color = FONT_YELLOW_COLOR,
    /* [我们] B5-A 对齐 PC 新版：删 Page_Next 跳转字段 */
    .Text_Set1 = "设置完成，下一步",
    .Text_Set2 = "设置完成，进入治疗",
    .Style_KnobColor = &style_YellowKnob,
};
Param_Data_t PosReh_Param_Data2 = {
    .Value_Intens1 = 1,
    .Value_Intens2 = 0,
    .Value_Freq = 2,
    .Value_Pulse = 500,
    .Slder_Color = FONT_YELLOW_COLOR,
    /* [我们] B5-A 对齐 PC 新版：删 Page_Next 跳转字段 */
    .Text_Set1 = "设置完成，下一步",
    .Text_Set2 = "设置完成，进入治疗",
    .Style_KnobColor = &style_YellowKnob,
};
Param_Data_t PosReh_Param_Data3 = {
    .Value_Intens1 = 1,
    .Value_Intens2 = 0,
    .Value_Freq = 3,
    .Value_Pulse = 500,
    .Slder_Color = FONT_YELLOW_COLOR,
    /* [我们] B5-A 对齐 PC 新版：删 Page_Next 跳转字段 */
    .Text_Set1 = "设置完成，下一步",
    .Text_Set2 = "设置完成，进入治疗",
    .Style_KnobColor = &style_YellowKnob,
};
Param_Data_t* PosReh_Param_list[Func_SUM] = {
    &PosReh_Param_Data1,
    &PosReh_Param_Data2,
    &PosReh_Param_Data3,
};

/* ===== [我们] 参数存储：NVS 治疗参数 ↔ PosReh_Param_Data1/2/3 ===== */
static bool s_treat_params_loaded = false;

/**
 * @brief 把 NVS 已存治疗参数应用到 PosReh_Param_Data1/2/3（只应用一次）
 *        首次进入产后修复页时调用（PosReh_Page_Load），此时无进行中的编辑，安全。
 */
void PosReh_Param_ApplySaved(void)
{
    if (s_treat_params_loaded) return;
    s_treat_params_loaded = true;

    for (int i = 0; i < Func_SUM && i < APP_PARAMS_TREAT_NUM; i++) {
        treat_param_t t;
        if (app_params_get_treat(i, &t) != ESP_OK) continue;
        Param_Data_t *p = PosReh_Param_list[i];
        if (p == NULL) continue;
        p->Value_Pulse   = (uint32_t)t.pulse;
        p->Value_Freq    = (uint32_t)t.freq;
        p->Value_Intens1 = t.intens1;
        p->Value_Intens2 = t.intens2;
        p->Value_Stage   = t.stage;
    }
    ESP_LOGI(TAG, "treat params applied from NVS");
}

/**
 * @brief 保存 3 个功能的治疗参数到 NVS（内存副本 + 落盘）
 *        模块退出（PosReh_BackToMenu）时调用，此时各功能参数为最新值。
 */
void PosReh_Param_SaveAll(void)
{
    for (int i = 0; i < Func_SUM && i < APP_PARAMS_TREAT_NUM; i++) {
        Param_Data_t *p = PosReh_Param_list[i];
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
/*=========================="腹直肌分离","子宫复旧","催乳" 疗程说明数据初始化==========================*/
TreIns_Data_t   PosReh_Treins_data1 = {
    .Obj_Hig = 150,
    .Obj_Wid = 404,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_YELLOW_COLOR,
    .Label_Btn1 = "单次治疗",
    .Label_Btn2 = "10次治疗",
    .Title = "腹直肌分离",.Title_time = "",
    .Text = "刺激腹部肌肉被动收缩,修复分离肌群,重塑腹部正常状态。单次时长30分钟。"
            "该次训练采用电刺激疗法,电刺激治疗过程中您无需主动发力,被动配合即可。",
    /* [我们] B5-A 对齐 PC 新版：删 Page_Start/Page_His 跳转字段 */
};
TreIns_Data_t   PosReh_Treins_data2 = {
    .Obj_Hig = 150,
    .Obj_Wid = 404,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_YELLOW_COLOR,
    .Label_Btn1 = "单次治疗",
    .Label_Btn2 = "10次治疗",
    .Title = "子宫复旧",.Title_time = "",
    .Text = "帮助子宫收缩,促进产后子宫恢复。单次时长30分钟。该次训练采用电刺激疗法,"
                   "电刺激治疗过程中您无需主动发力,被动配合即可。",
    /* [我们] B5-A 对齐 PC 新版：删 Page_Start/Page_His 跳转字段 */
};
TreIns_Data_t   PosReh_Treins_data3 = {
    .Obj_Hig = 150,
    .Obj_Wid = 404,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_YELLOW_COLOR,
    .Label_Btn1 = "单次治疗",
    .Label_Btn2 = "10次治疗",
    .Title = "催乳",.Title_time = "",
    .Text = "电刺激治疗过程中您无需主动发力,被动配合即可。"
                   "促进分泌乳汁。单次时长30分钟。该次训练采用电刺激疗法,电刺激治疗过程中您无需主动发力,被动配合即可。",
    /* [我们] B5-A 对齐 PC 新版：删 Page_Start/Page_His 跳转字段 */
};
TreIns_Data_t* PosReh_Treins_list[Func_SUM] = {
   &PosReh_Treins_data1,
   &PosReh_Treins_data2,
   &PosReh_Treins_data3,
};

/* ===== [我们] B4 keypad group 管理 + 通用守则清理（坑 6.8 / B2-4 / B3-3 同族） ===== */
#define POSREH_GROUP_MAX 12
static lv_obj_t* s_posreh_objs[POSREH_GROUP_MAX];
static int s_posreh_cnt = 0;

/* 注册对象进 keypad group 并记录（切页时统一移出，防 refocus FOCUSED 悬垂） */
static void PosReh_GroupRegister(lv_obj_t* obj)
{
    lv_group_t *g = app_keypad_get_group();
    if (g && obj) {
        lv_group_add_obj(g, obj);
        if (s_posreh_cnt < POSREH_GROUP_MAX)
            s_posreh_objs[s_posreh_cnt++] = obj;
    }
}

/* 把本页注册的对象移出 group（lv_obj_is_valid 防悬垂，坑 B2-4） */
void PosReh_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        for (int i = 0; i < s_posreh_cnt; i++) {
            if (s_posreh_objs[i] && lv_obj_is_valid(s_posreh_objs[i]))
                lv_group_remove_obj(s_posreh_objs[i]);
        }
    }
    s_posreh_cnt = 0;
}

/* 通用守则：定时器生命周期与页面一致（页面退出/切页前删除） */
void PosReh_StopTimer(void)
{
    for (int i = 0; i < Func_SUM; i++) {
        if (PosReh_CtxData[i]->timer_Treat) {
            lv_timer_del(PosReh_CtxData[i]->timer_Treat);
            PosReh_CtxData[i]->timer_Treat = NULL;
        }
        PosReh_CtxData[i]->status = TIMER_PAUSED;
    }
}

/* 统一退出函数：停定时器 + 移出 group + 保存参数 + 回主菜单 */
void PosReh_BackToMenu(void)
{
    PosReh_StopTimer();
    PosReh_LeaveGroup();
    PosReh_Param_SaveAll();              /* [我们] 参数存储：退出前保存全部治疗参数 */
    memset(&PosReh_Widget, 0, sizeof(PosReh_Widget));   /* [我们] B3 防御：清零 stale 指针 */
    Goto_MenuPage();
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
 * @brief 产后康复子页面创建入口
 * @param page_id
 */
void PosReh_Page_Load(PosReh_PageID_t page_id)
{
    PosReh_LeaveGroup();        /* [我们] 先移出 group 再 Page_Clean（坑 6.8） */
    Page_Clean();
    memset(&PosReh_Widget, 0, sizeof(PosReh_Widget));   /* [我们] B3 防御：清零 stale 指针 */
    PosReh_Param_ApplySaved();  /* [我们] 参数存储：首次进入时应用 NVS 已存治疗参数 */
    uint8_t func = PosReh_Get_CurFunc();
    switch (page_id) {
    case PosRehPage1:
        PosReh_Page1_widget(&PosReh_Widget, g_Ui.page_container);
        break;
    case PosRehPage2:
        PosReh_Page2_widget(&PosReh_Widget, g_Ui.page_container);
        break;
    case PosRehPage3:
        PosReh_Page3_widget(PosReh_Treins_list[func], g_Ui.page_container);
        break;
    case PosRehPage4:
        PosReh_Param_Data1.Value_Intens1 = 0;
        PosReh_Param_Data1.stage_stim = therapy_cur_stage_stim(&rx_pn.schemes[func]);
        PosReh_Param_Data1.Value_Freq = therapy_cur_freq(&rx_pn.schemes[func],1);
        PosReh_Param_Data1.Value_Pulse = therapy_cur_pw(&rx_pn.schemes[func],1);
        PosReh_Page4_widget(&PosReh_Param_Data1, g_Ui.page_container);
        audio_play_request(VOICE_ID_MAX_INTENSITY_CN,1500);              //语音播放：调节强度
        break;
    case PosRehPage5:
        PosReh_CtxData1.treat_data.Value_Freq = therapy_cur_freq(&rx_pn.schemes[func],1);
        PosReh_CtxData1.treat_data.Value_Pulse = therapy_cur_pw(&rx_pn.schemes[func],1);
        PosReh_CtxData1.treat_data.Intens_Value = PosReh_Param_Data1.Value_Intens1;
        PosReh_CtxData1.total_sec = therapy_get_stim_time(&rx_pn.schemes[func]);
        PosReh_Page5_widget(&PosReh_CtxData1, g_Ui.page_container);
        break;
    default:
        break;
    }
}

/**
 * @brief 产后康复页ui设计
 * @param  none
 */
void PosReh_Page_ui(void)
{
    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_BLUE_COLOR, 0);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(page_cont);
    lv_obj_clear_flag(page_cont, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    lv_obj_set_scrollbar_mode(page_cont, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/

    PosReh_Page1_widget(&PosReh_Widget, page_cont);

}
/**
 * @brief 获取当前康复治疗功能模式
 * @param  
 * @return 
 */
uint8_t PosReh_Get_CurFunc(void)
{
    return g_Cur_Func;
}
/**
 * @brief 设置当前康复治疗功能模式
 * @param func 
 */
void PosReh_Set_CurFunc(PosReh_Func_Name_t func)
{
    if (func < Func_SUM) {
        g_Cur_Func = func;
    }
}


/**
 * @brief 产后康复第5页 电刺激治疗
 * @param  none
 */
static void PosReh_Page5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont)
{
   
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cont, 0, 0);

    Treat_Widget(cont, param);
    //单独将暂停、开始、返回、是\否按钮点击事件 放在本文件，方便页面跳转
    lv_obj_add_event_cb(param->treat_data.Pause_Btn, PosReh_P5Btn_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, PosReh_P5Btn_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn, PosReh_P5Btn_Event_cb, LV_EVENT_CLICKED, param);

    /* [我们] keypad：Plu/Min/Pause/Start/Back 进 group + KEY 导航 + 默认聚焦“开始” */
    PosReh_GroupRegister(param->treat_data.Plu_Btn);
    PosReh_GroupRegister(param->treat_data.Min_Btn);
    PosReh_GroupRegister(param->treat_data.Pause_Btn);
    PosReh_GroupRegister(param->treat_data.Start_Btn);
    PosReh_GroupRegister(param->treat_data.Back_Btn);
    lv_obj_add_event_cb(param->treat_data.Plu_Btn,   PosReh_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Min_Btn,   PosReh_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Pause_Btn, PosReh_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, PosReh_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn,  PosReh_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_group_focus_obj(param->treat_data.Start_Btn);
}

/**
 * @brief 产后康复第4页 电刺激参数设置
 * @param  none
 */
static void PosReh_Page4_widget(Param_Data_t* param, lv_obj_t* page_cont)
{

    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(cont, 0, 0);

    ParSet_Widget(cont, param);      //绘制参数设置页面
    lv_obj_add_event_cb(param->Set_Btn, PosReh_P4SetBtn_Event_cb, LV_EVENT_CLICKED, param);    //将加按钮添加聚焦事件回调

    /* [我们] keypad：滑条不进 group（LVGL 滑条 4 方向都调值无法拦截）；
     * 减/加/设置 进 group：左右=调档位、上下=切按钮（Min->Plu->Set）、默认聚焦减 */
    PosReh_GroupRegister(param->Min_Btn);
    PosReh_GroupRegister(param->Plu_Btn);
    PosReh_GroupRegister(param->Set_Btn);
    lv_obj_add_event_cb(param->Min_Btn, PosReh_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Plu_Btn, PosReh_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Set_Btn, PosReh_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_group_focus_obj(param->Set_Btn);
}



/**
 * @brief 产后康复第3页 疗程介绍页
 * @param  none
 */
static void PosReh_Page3_widget(TreIns_Data_t* param, lv_obj_t* page_cont)
{
 
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                 //启用滑动条
    lv_obj_set_style_pad_all(cont, 0, 0);
    /* 页面滚动设置*/
    PosReh_TreIns_Scroll.Obj = cont;
    PosReh_TreIns_Scroll.Origin_Y = 210;
    PosReh_TreIns_Scroll.END_Y = 0;
    param->Scroll = &PosReh_TreIns_Scroll;

    TreIns_Widget(cont, param);
    lv_obj_add_event_cb(param->History_Btn, PosReh_TreIns_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->Start_Btn, PosReh_TreIns_Event_cb, LV_EVENT_CLICKED, param);

    /* [我们] keypad：History/Start 进 group + KEY 导航 + 默认聚焦“开始” */
    PosReh_GroupRegister(param->History_Btn);
    PosReh_GroupRegister(param->Start_Btn);
    lv_obj_add_event_cb(param->History_Btn, PosReh_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Start_Btn,   PosReh_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_group_focus_obj(param->Start_Btn);

    /* 新增：绑定按键上下滚动页面 */
    lv_obj_add_event_cb(param->History_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
    lv_obj_add_event_cb(param->Start_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
}

/**
 * @brief 产后康复第2页
 * @param  none
 */
static void PosReh_Page2_widget(PosReh_Widget_t* widget, lv_obj_t* page_cont)
{
    char* text_no[3] = {"01","02","03"};
    char* text_name[3] = { "腹直肌\n 分离","子宫\n复旧","催乳" };
  
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_style_pad_all(cont, 0, 0);
    /*标题*/
    lv_obj_t* title = Creat_Label(cont, "产后康复", basic_widget.Chinise_Font_Title);
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
        lv_obj_align(Obj1, LV_ALIGN_CENTER, -5, 20);
        lv_obj_set_style_radius(Obj1, 60, LV_PART_MAIN);
        lv_obj_add_style(Obj1, &style_gradient_gray, LV_PART_MAIN);
        //选中显示控件
        lv_obj_t* Obj2 = Create_Obj(Obj1, 90, 90, 0xFFFffff, 1);
        lv_obj_align(Obj2, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_radius(Obj2, 45, LV_PART_MAIN);
        lv_obj_set_style_border_opa(Obj2, 100, LV_PART_MAIN);
        lv_obj_set_style_border_width(Obj2, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(Obj2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        /*lv_obj_set_style_border_side(Obj2, LV_BORDER_SIDE_BOTTOM | LV_BORDER_SIDE_LEFT| LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);*/
        widget->SelTreat_Data[i].Obj_Select = Obj2;
        lv_obj_t* btn = Create_Button(Obj2, 90, 90, FONT_GRAY_COLOR, 45, 1);            //创建按钮
        lv_obj_center(btn);
        widget->SelTreat_Data[i].Btn_Func = btn;
       
        lv_obj_add_event_cb(btn, SelTreatPage_Event_cb, LV_EVENT_ALL, widget);
        /* [我们] keypad：功能按钮进 group + KEY 导航 */
        PosReh_GroupRegister(btn);
        lv_obj_add_event_cb(btn, PosReh_Page2_Key_cb, LV_EVENT_KEY, widget);

        lv_obj_t* label = Creat_Label(Obj, text_no[i], basic_widget.Chinese_Font_30);
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 15);
        lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        widget->SelTreat_Data[i].Label_NO = label;
        lv_obj_t* label2 = Creat_Label(Obj, text_name[i], basic_widget.Chinise_Font_Title);
        lv_obj_align(label2, LV_ALIGN_CENTER, -5, 23);
        lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        widget->SelTreat_Data[i].Label_Func = label2;
    }
    /* [我们] 默认聚焦当前功能 */
    lv_group_focus_obj(widget->SelTreat_Data[PosReh_Get_CurFunc()].Btn_Func);
}

/**
 * @brief 产后康复第1页
 * @param  none
 */
static void PosReh_Page1_widget(PosReh_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_style_pad_all(cont, 0, 0);

    PosReh_Instr_Data_t insData = {
        .Title = "产后康复说明",
        .Text = "产后康复涵盖催乳、子宫复旧、腹直肌分离修复三大核心项目。",
        .src = WOMEN_ICON,
    };
    widget->InsData = insData;

    /*说明框*/
    lv_obj_t* Obj = Create_Obj(cont, 252, 120, 0xffffff, 0);        //中文 252 150
    lv_obj_set_pos(Obj, 25, 5); 
    //lv_obj_set_height(Obj, LV_SIZE_CONTENT);
    /*添加样式*/
    lv_obj_add_style(Obj, &style_shadow, LV_PART_MAIN);      // 底层：阴影
    lv_obj_add_style(Obj, &style_gradient, LV_PART_MAIN);    // 中层：渐变
    /*标题*/
    lv_obj_t* title = Creat_Label(Obj, widget->InsData.Title, basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(title, 15, 10);
    lv_obj_set_style_text_color(title, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    /*正文*/
    lv_obj_t* label = Creat_Label(Obj, "·", basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(label, 15, 45);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_t* text = Creat_TextLabel(Obj, 210, 50, widget->InsData.Text, basic_widget.Chinise_Font_Text);
    lv_obj_set_pos(text, 25, 40);
    lv_obj_set_style_text_color(text, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_line_space(text, 10, 0);          // 行间距 10px

    /*图片*/
    lv_obj_t* Img = Creat_Image(cont, widget->InsData.src);
    lv_obj_set_pos(Img, 291, 19);

    /*选择疗程按钮*/
    lv_obj_t* btn = Create_Button(cont, 110, 40, FONT_YELLOW_COLOR, 30, 0);
    lv_obj_set_pos(btn, 305, 200);
    lv_obj_set_style_shadow_opa(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(btn, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    lv_obj_t* label0 = Creat_Label2(btn, POSREH_PAGE1_ID_SELECT, basic_widget.Chinise_Font_Btn);
    lv_obj_center(label0);
    lv_obj_set_style_text_color(label0, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    widget->InsData.Btn_Select = btn;
    widget->InsData.Label_Select = label0;
    lv_obj_add_event_cb(btn, InstrPage_Event_cb, LV_EVENT_ALL, widget);

    /*腹直肌分离治疗按钮*/
    lv_obj_t* btn1 = Create_Button(cont, 222, 21, FONT_WHITE_COLOR, 0, 0);
    /*lv_obj_set_pos(btn1, 30, 180);*/
    lv_obj_align_to(btn1, Obj, LV_ALIGN_OUT_BOTTOM_MID,0,20);
    lv_obj_set_style_shadow_opa(btn1, 0, LV_PART_MAIN);
    lv_obj_t* label1 = Creat_Label2(btn1, POSREH_PAGE1_ID_DIA, basic_widget.Chinise_Font_Text);
    lv_obj_center(label1);
    lv_obj_set_style_text_color(label1, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    widget->InsData.Btn_DiaRecti = btn1;
    widget->InsData.Label_DiaRecti = label1;
    lv_obj_add_event_cb(btn1, InstrPage_Event_cb, LV_EVENT_ALL, widget);
    /*催乳治疗按钮*/
    lv_obj_t* btn2 = Create_Button(cont, 222, 21, FONT_WHITE_COLOR, 0, 0);
  /*  lv_obj_set_pos(btn2, 30, 203);*/
    lv_obj_align_to(btn2, btn1, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_style_shadow_opa(btn2, 0, LV_PART_MAIN);
    lv_obj_t* label2 = Creat_Label2(btn2, POSREH_PAGE1_ID_UTE, basic_widget.Chinise_Font_Text);
    lv_obj_center(label2);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    widget->InsData.Btn_UterInvo = btn2;
    widget->InsData.Label_UterInvo = label2;
    lv_obj_add_event_cb(btn2, InstrPage_Event_cb, LV_EVENT_ALL, widget);
    /*子宫复旧治疗按钮*/
    lv_obj_t* btn3 = Create_Button(cont, 222, 21, FONT_WHITE_COLOR, 0, 0);
    /*lv_obj_set_pos(btn3, 30, 225);*/
    lv_obj_align_to(btn3, btn2, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_set_style_shadow_opa(btn3, 0, LV_PART_MAIN);
    lv_obj_t* label3 = Creat_Label2(btn3, POSREH_PAGE1_ID_LAC, basic_widget.Chinise_Font_Text);
    lv_obj_center(label3);
    lv_obj_set_style_text_color(label3, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    
    widget->InsData.Btn_LacPro = btn3;
    widget->InsData.Label_LacPro = label3;
    lv_obj_add_event_cb(btn3, InstrPage_Event_cb, LV_EVENT_ALL, widget);

    /* [我们] keypad：4 按钮进 group + KEY 导航 + 默认聚焦“选择” */
    PosReh_GroupRegister(btn);
    PosReh_GroupRegister(btn1);
    PosReh_GroupRegister(btn2);
    PosReh_GroupRegister(btn3);
    lv_obj_add_event_cb(btn,  PosReh_Page1_Key_cb, LV_EVENT_KEY, widget);
    lv_obj_add_event_cb(btn1, PosReh_Page1_Key_cb, LV_EVENT_KEY, widget);
    lv_obj_add_event_cb(btn2, PosReh_Page1_Key_cb, LV_EVENT_KEY, widget);
    lv_obj_add_event_cb(btn3, PosReh_Page1_Key_cb, LV_EVENT_KEY, widget);
    lv_group_focus_obj(btn);
}
// /**
//  * @brief 产后康复第2页
//  * @param  none
//  */
// void PosReh_Show_Page2_widget(void)
// {
//     /* 清空当前页面 */
//     lv_obj_clean(g_page_list[Menu_Page]->page_cont);
   
//     lv_obj_t* cont = Create_Obj(g_page_list[Menu_Page]->page_cont, 480, 290, FONT_GRAY_COLOR, 0);
//     lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
//     lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
//     lv_obj_set_pos(cont, -8, -8);
//     lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

//     PosReh_Page2_widget(&PosReh_Widget, cont);
   
// }

// /**
//  * @brief 产后康复第5页 先清空再重新绘制显示
//  * @param  none
//  */
// void PosReh_Show_Page5_widget(Treat_Timer_Ctx_t* param)
// {
//     /* 清空当前页面 */
//     lv_obj_clean(lv_my_PosRehPage5_t.page_cont);

//     lv_obj_t* cont = Create_Obj(lv_my_ui_pagePosReh_t.page_cont, 480, 290, FONT_GRAY_COLOR, 0);
//     lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
//     lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
//     lv_obj_set_pos(cont, -8, -8);
//     lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);                     // 隐藏界面
//     lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
//     lv_my_PosRehPage5_t.page_cont = cont;

//     Treat_Widget(lv_my_PosRehPage5_t.page_cont, param);
//     //单独将暂停、开始、返回、是\否按钮点击事件 放在本文件，方便页面跳转
//     lv_obj_add_event_cb(param->treat_data.Pause_Btn, PosReh_P5Btn_Event_cb, LV_EVENT_CLICKED, param);
//     lv_obj_add_event_cb(param->treat_data.Start_Btn, PosReh_P5Btn_Event_cb, LV_EVENT_CLICKED, param);
//     lv_obj_add_event_cb(param->treat_data.Back_Btn, PosReh_P5Btn_Event_cb, LV_EVENT_CLICKED, param);
// }

// /**
//  * @brief 产后康复第4页 先清空再重新绘制显示
//  * @param  none
//  */
// void PosReh_Show_Page4_widget(Param_Data_t* param)
// {
//     /* 清空当前页面 */
//     lv_obj_clean(lv_my_PosRehPage4_t.page_cont);

//     lv_obj_t* cont = Create_Obj(lv_my_ui_pagePosReh_t.page_cont, 480, 290, FONT_GRAY_COLOR, 0);
//     lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
//     lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
//     lv_obj_set_pos(cont, -8, -8);
//     lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);                     // 隐藏界面
//     lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
//     lv_my_PosRehPage4_t.page_cont = cont;

//     ParSet_Widget(lv_my_PosRehPage4_t.page_cont, param);      //绘制参数设置页面
//     lv_obj_add_event_cb(param->Set_Btn, PosReh_P4SetBtn_Event_cb, LV_EVENT_CLICKED, param);    //将加按钮添加聚焦事件回调
// }

// /**
//  * @brief 产后康复 页3 先清空再重新绘制显示
//  * @param param
//  */
// void PosReh_Show_Page3_widget(TreIns_Data_t* param)
// {
//     /* 清空当前页面 */
//     lv_obj_clean(lv_my_PosRehPage3_t.page_cont);

//     lv_obj_t* cont = Create_Obj(lv_my_ui_pagePosReh_t.page_cont, 480, 290, FONT_WHITE_COLOR, 1);
//     lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
//     lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
//     lv_obj_set_pos(cont, -8, -8);
//     lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);                     // 隐藏界面
//     lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                 //启用滑动条
//     lv_my_PosRehPage3_t.page_cont = cont;

//     TreIns_Widget(lv_my_PosRehPage3_t.page_cont, param);
//     lv_obj_add_event_cb(param->History_Btn, PosReh_TreIns_Event_cb, LV_EVENT_CLICKED, param);
//     lv_obj_add_event_cb(param->Start_Btn, PosReh_TreIns_Event_cb, LV_EVENT_CLICKED, param);
// }
