#include "Chlid_DysPage.h"
#include "PelFlo_The_ui.h"
#include "PelFlo_TheChild_event.h"
#include "menu_ui.h"
#include "basic.h"
#include "app_keypad.h"         /* [我们] keypad group 注册 */
#include <string.h>             /* [我们] memset */
#include "app_therapy.h"   
#include "esp_log.h"         /* [我们] stub 日志 */
#include "voice.h"
static char *TAG = "Child_DysPage";
/* ============================================================================
 * [我们] B6-B 通用守则基础设施（坑 6.8 / B2-4 / B3-3 同族，复制治疗格 + Dys_ 前缀）
 *   - 障碍格 5 页无曲线页 → 无 s_esc_obj / 无 Chart 定时器，只做 P1~P5 Key 导航
 *   - 复用治疗格 The_Page5_ShowConfirm（操作 ctx->Window，不依赖具体格）
 * ==========================================================================*/

#define DYS_GROUP_MAX 32
static lv_obj_t* s_dys_objs[DYS_GROUP_MAX];   /* 本页已注册进 group 的对象 */
static int s_dys_cnt = 0;

/* [我们] 数据实例前向声明（定义见文件下部，Dys_StopTimer 先引用） */
extern Treat_Timer_Ctx_t DysPage_CtxData1;
Scroll_Data_t Dys_TreIns_Scroll;           //评估页滚动页面数据
/* 注册对象进 keypad group 并记录（切页时统一移出，防 refocus FOCUSED 悬垂） */
static void Dys_GroupRegister(lv_obj_t* obj)
{
    lv_group_t *g = app_keypad_get_group();
    if (g && obj) {
        lv_group_add_obj(g, obj);
        if (s_dys_cnt < DYS_GROUP_MAX)
            s_dys_objs[s_dys_cnt++] = obj;
    }
}

/* 把本页注册的对象移出 group（lv_obj_is_valid 防悬垂，坑 B2-4） */
void Child_DysPage_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        for (int i = 0; i < s_dys_cnt; i++) {
            if (s_dys_objs[i] && lv_obj_is_valid(s_dys_objs[i]))
                lv_group_remove_obj(s_dys_objs[i]);
        }
    }
    s_dys_cnt = 0;
}

/* 停止障碍格治疗定时器（页面退出/切页前调用；单例 CtxData，无曲线 timer） */
static void Dys_StopTimer(void)
{
    if (DysPage_CtxData1.timer_Treat) { lv_timer_del(DysPage_CtxData1.timer_Treat); DysPage_CtxData1.timer_Treat = NULL; }
    if (DysPage_CtxData1.timer_Next)  { lv_timer_del(DysPage_CtxData1.timer_Next);  DysPage_CtxData1.timer_Next  = NULL; }
    DysPage_CtxData1.status = TIMER_PAUSED;
}

/* 统一退出：停定时器 + 移出 group + 回主菜单（删除路径调用后须提前 return，坑 B4-3） */
void Dys_BackToMenu(void)
{
    Dys_StopTimer();
    Child_DysPage_LeaveGroup();
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
/* ============================================================================
 * 障碍页 5 子页 widget 前向声明
 * ==========================================================================*/
static void DysPage1_widget(Instr_Data_t* param, lv_obj_t* page_cont);
static void DysPage2_widget(Select_Data_t* param, lv_obj_t* page_cont);
static void DysPage3_widget(TreIns_Data_t* param, lv_obj_t* page_cont);
static void DysPage4_widget(Param_Data_t* param, lv_obj_t* page_cont);
static void DysPage5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont);

/* 各页 KEY 导航回调（[我们] B6-B，通用守则全应用） */
static void DysPage_Page1_Key_cb(lv_event_t* e);
static void DysPage_Page2_Key_cb(lv_event_t* e);
static void DysPage_Page3_Key_cb(lv_event_t* e);
static void DysPage_Page4_Key_cb(lv_event_t* e);
static void DysPage_Page5_Key_cb(lv_event_t* e);

/* ============================================================================
 * 数据实例（移植自 PC Chlid_DysPage.c；PC 死代码已删：
 *   7 个非 static 死全局 Dys_Instr_data/Dys_Select_data/Dys_Treins_data/Dys_Param_data/
 *   Dys_Treat_data/Dys_ctx/Dys_page（均未引用）+ MAX_TREAT_POINTS/g_Treat_Points/g_Treat_Point_cnt）
 * ==========================================================================*/

/* 治疗功能介绍结构体 */
Instr_Data_t    Dys_Instr_data1 = {
    .Obj_Hig = 220,
    
    .Obj_Wid = 252,
    .Title = "高张力盆底功能障碍治疗",
    .Text = "适用于高张力型盆底功能障碍干预,可改善盆底局部血液循环,"
            "有效舒缓盆底肌异常高张力,放松紧张肌群,缓解盆底区域酸痛"
            "不适,逐步恢复盆底肌正常生理状态与健康机能。",
    .Title_Color = FONT_RED_COLOR,
};
static Stage_Desc_t Stage_Tbl[1] = {
    [STAGE_1] = {"阶段一", 15},
};

Select_Data_t   Dys_Select_data1 = {
    .High_Obj = 520,         /* 根据按钮数量决定设置容器的高度 */
    .Pos_Y = -18,
    .Sta_Table = Stage_Tbl,  /* [我们] PC 写 &Stage_Tbl（指针到数组）→ GCC -Werror incompatible-pointer-types，改数组名（自动衰减） */
    .Stage_Num = 1,
};
TreIns_Data_t   Dys_Treins_data1 = {
    .Obj_Hig = 120,
    .Obj_Wid = 400,
    .src = WIRING_DIAG_ICON,
    .Title_Color = FONT_RED_COLOR,
    .Label_Btn1 = "历史",
    .Label_Btn2 = "开始治疗",
    .Title = "阶段一·",
    .Text = "阶段一共计15次治疗,单次时长30分钟。采用电刺激疗法,"
            "治疗过程中您无需主动发力,被动配合即可。",
};
/* 参数设置结构体 */
Param_Data_t Dys_Param_Data1 = {
    .Value_Intens1 = 1,
    .Value_Intens2 = 1,
    .Value_Freq = 40,
    .Value_Pulse = 600,
    .Slder_Color = FONT_RED_COLOR ,
    .Text_Set1 = "设置完成，下一步",
    .Text_Set2 = "设置完成，进入治疗",
    .Style_KnobColor = &style_RedKnob,
};

/* 治疗参数结构体（单例：障碍格仅阶段一） */
Treat_Timer_Ctx_t DysPage_CtxData1 = {
    .total_sec = 10 * 1000,
    .remain_sec = 2 * 1000,
    .status = TIMER_IDLE,
    .treat_data.Color = FONT_RED_COLOR,
    .treat_data.Intens_Value = 9,
};
static char title_buf[9];
/**
 * @brief 盆底功能障碍子页面创建入口
 * @param page_id
 */
void Child_DysPage_Load(ChiDysPage_ID_t page_id)
{
    Child_DysPage_LeaveGroup();        /* [我们] 先移出 group 再 Page_Clean（坑 6.8） */
    Page_Clean();
    switch (page_id) {
    case ChiDysPage_Instr:
        DysPage1_widget(&Dys_Instr_data1, g_Ui.page_container);
        break;
    case ChiDysPage_StaSel:
        DysPage2_widget(&Dys_Select_data1, g_Ui.page_container);
        break;
    case ChiDysPage_TreIns:
        uint8_t num = Get_CurTime_Idx();
        
        lv_snprintf(title_buf, sizeof(title_buf), "治疗%d", num);
        ESP_LOGI(TAG,"%s ", title_buf);
        Dys_Treins_data1.Title_time = title_buf;
        DysPage3_widget(&Dys_Treins_data1, g_Ui.page_container);
        break;
    case ChiDysPage_ParSet:
        uint8_t schemes_idx = Get_CurTreat_Idx();
        Dys_Param_Data1.Value_Intens1 = 0;
        Dys_Param_Data1.Value_Intens2 = 0;             
        Dys_Param_Data1.stage_stim = therapy_cur_stage_stim(&g_rx_all[RX_1]->schemes[schemes_idx]);
        Dys_Param_Data1.Value_Freq = therapy_cur_freq(&g_rx_all[RX_1]->schemes[schemes_idx],1);
        Dys_Param_Data1.Value_Pulse = therapy_cur_pw(&g_rx_all[RX_1]->schemes[schemes_idx],1);
        DysPage4_widget(&Dys_Param_Data1, g_Ui.page_container);
        audio_play_request(VOICE_ID_MAX_INTENSITY_CN,1500);              //语音播放：调节强度
        break;
    case ChiDysPage_Treat:
        uint8_t schemes_idx2 = Get_CurTreat_Idx();
        DysPage_CtxData1.treat_data.Value_Freq = therapy_cur_freq(&g_rx_all[RX_1]->schemes[schemes_idx2],1);
        DysPage_CtxData1.treat_data.Value_Pulse = therapy_cur_pw(&g_rx_all[RX_1]->schemes[schemes_idx2],1);
        DysPage_CtxData1.treat_data.Intens_Value = Dys_Param_Data1.Value_Intens1;
        DysPage_CtxData1.total_sec = therapy_get_stim_time(&g_rx_all[RX_1]->schemes[schemes_idx2]);
        DysPage5_widget(&DysPage_CtxData1, g_Ui.page_container);
        break;
    default:
        break;
    }
}

/**
 * @brief 功能障碍页面（四宫格「障碍」格入口）
 * @param  none
 */
void Child_DysPage_ui(void)
{
    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(page_cont);

    DysPage1_widget(&Dys_Instr_data1, page_cont);
}

/* ============================================================================
 * [我们] 各页 KEY 导航回调（通用守则：ESC 返回 / 2D 导航 / 删对象路径不用 stop_processing）
 * ==========================================================================*/

/* Page1 说明页：ESC 回四宫格；ENTER 由 Child_Page_StartBtn_Event_cb 处理 */
static void DysPage_Page1_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        PelFlo_ThePage_Load(MenPage);   /* 删对象路径：后续不再访问任何对象（坑 B4-3） */
    }
}

/* 疗程网格：查找当前按钮索引 */
static uint8_t Dys_FindStage(Select_Data_t* data, lv_obj_t* btn)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (data->sta[i].btn == btn) return i;
    }
    return data->Btn_MaxSum;
}

/* 疗程网格：全部格设未选中样式 */
static void Dys_StageUnSel(Select_Data_t* data)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; i++) {
        if (lv_obj_is_valid(data->sta[i].obj)) lv_obj_remove_style(data->sta[i].obj, &style_gradient, LV_PART_MAIN);
        if (lv_obj_is_valid(data->sta[i].img)) lv_obj_set_style_image_recolor(data->sta[i].img, lv_color_hex(FONT_GRAY_COLOR), 0);
    }
}

/* Page2 疗程网格：FOCUSED 高亮 + KEY 2D 导航 + ESC 回 Page1（3 格/行，Flex 换行） */
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
    if (key == LV_KEY_ESC) { Child_DysPage_Load(ChiDysPage_Instr); return; }   /* 删对象路径，提前 return */

    uint8_t num = Dys_FindStage(data, obj);
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
static void DysPage_Page3_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_DysPage_Load(ChiDysPage_StaSel); return; }   /* 删对象路径 */

    if (key == LV_KEY_LEFT && obj == data->Start_Btn) lv_group_focus_obj(data->History_Btn);
    else if (key == LV_KEY_RIGHT && obj == data->History_Btn) lv_group_focus_obj(data->Start_Btn);
}

/* Page4 参数设置：滑条不进 group（B4-2）；减/加/设置 进 group；
 * 左右=调档位、上下=线性切按钮(减→加→设置)、ENTER=动作、ESC 回 Page3 */
static void DysPage_Page4_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { Child_DysPage_Load(ChiDysPage_TreIns); return; }   /* 删对象路径 */

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

   /* else if (key == LV_KEY_LEFT) {
        if (obj == data->Plu_Btn) lv_group_focus_obj(data->Min_Btn);
    }
    else if (key == LV_KEY_RIGHT) {
        if (obj == data->Min_Btn) lv_group_focus_obj(data->Plu_Btn);
    } */
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

/* Page5 治疗中：LEFT/RIGHT 切焦点（同治疗格），ENTER 由 ChildPage_P5Btn_Event_cb 处理，ESC=返回确认 */
static void DysPage_Page5_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) {
        if (ctx->remain_sec == 0) {
            Dys_BackToMenu();              /* 删对象路径，后续不再访问 */
        }
        else {
            /* [我们] 先删 timer 再弹确认（坑 B6-A-1 同款：确认“是”→Back_Btn_Event_cb→Goto_MenuPage 不删 timer） */
            // if (ctx->timer_Treat) { lv_timer_del(ctx->timer_Treat); ctx->timer_Treat = NULL; }
            // ctx->status = TIMER_PAUSED;
            therapy_pause();
            The_Page5_ShowConfirm(ctx);    /* 复用治疗格键盘化确认 */
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
 * 5 子页 widget（移植自 PC Chlid_DysPage.c；含 [我们] 按键导航注册）
 * ==========================================================================*/

/**
 * @brief 电刺激治疗页（暂停/开始/返回 各自注册 CLICKED）
 */
static void DysPage5_widget(Treat_Timer_Ctx_t* param, lv_obj_t* page_cont)
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
    Dys_GroupRegister(param->treat_data.Plu_Btn);
    Dys_GroupRegister(param->treat_data.Min_Btn);
    Dys_GroupRegister(param->treat_data.Pause_Btn);
    Dys_GroupRegister(param->treat_data.Start_Btn);
    Dys_GroupRegister(param->treat_data.Back_Btn);
    lv_group_focus_obj(param->treat_data.Start_Btn);
    lv_obj_add_event_cb(param->treat_data.Plu_Btn,   DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Min_Btn,   DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Pause_Btn, DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Start_Btn, DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->treat_data.Back_Btn,  DysPage_Page5_Key_cb, LV_EVENT_KEY, param);
}

/**
 * @brief 电刺激参数设置页（设置按钮单独注册 CLICKED）
 */
static void DysPage4_widget(Param_Data_t* param, lv_obj_t* page_cont)
{

    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_style_pad_all(cont, 0, 0);
    /*绘制参数设置页面 设置按钮点击事件单独添加回调函数
      其他控件的聚焦点击事件都统一在菜单事件回调函数里*/
    ParSet_Widget(page_cont, param);
    lv_obj_add_event_cb(param->Set_Btn, ChildPage_P4SetBtn_Event_cb, LV_EVENT_CLICKED, param);

    /* ===== [我们] 按键导航：滑条不进 group（B4-2）；减/加/设置 进 group + KEY 导航 ===== */
    Dys_GroupRegister(param->Min_Btn);
    Dys_GroupRegister(param->Plu_Btn);
    Dys_GroupRegister(param->Set_Btn);
    lv_group_focus_obj(param->Plu_Btn);
    lv_obj_add_event_cb(param->Min_Btn, DysPage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Plu_Btn, DysPage_Page4_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Set_Btn, DysPage_Page4_Key_cb, LV_EVENT_KEY, param);
}

/**
 * @brief 阶段治疗说明页（历史/开始 单独注册 CLICKED）
 */
static void DysPage3_widget(TreIns_Data_t* param, lv_obj_t* page_cont)
{

    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_set_pos(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);                 //启用滑动条
    lv_obj_set_style_pad_all(cont, 0, 0);
    /* 页面滚动设置*/
    Dys_TreIns_Scroll.Obj = cont;
    Dys_TreIns_Scroll.Origin_Y = 210;
    Dys_TreIns_Scroll.END_Y = 0;
    param->Scroll = &Dys_TreIns_Scroll;

    TreIns_Widget(cont, param);
    lv_obj_add_event_cb(param->History_Btn, ChildPage_TreIns_Event_cb, LV_EVENT_CLICKED, param);
    lv_obj_add_event_cb(param->Start_Btn, ChildPage_TreIns_Event_cb, LV_EVENT_CLICKED, param);

    /* ===== [我们] 按键导航：历史/开始 进 group + 默认焦点“开始” + KEY 导航 ===== */
    Dys_GroupRegister(param->History_Btn);
    Dys_GroupRegister(param->Start_Btn);
    lv_group_focus_obj(param->Start_Btn);
    lv_obj_add_event_cb(param->History_Btn, DysPage_Page3_Key_cb, LV_EVENT_KEY, param);
    lv_obj_add_event_cb(param->Start_Btn,   DysPage_Page3_Key_cb, LV_EVENT_KEY, param);
    /* 新增：绑定按键上下滚动页面 */
    lv_obj_add_event_cb(param->History_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
    lv_obj_add_event_cb(param->Start_Btn, page_scroll_key_cb, LV_EVENT_KEY, param->Scroll);
}

/**
 * @brief 阶段疗程选择页（网格按钮逐个注册）
 */
static void DysPage2_widget(Select_Data_t* param, lv_obj_t* page_cont)
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
        Dys_GroupRegister(param->sta[i].btn);
        lv_obj_add_event_cb(param->sta[i].btn, DysPage_Page2_Key_cb, LV_EVENT_ALL, param);
    }
    if (param->Btn_MaxSum > 0) lv_group_focus_obj(param->sta[0].btn);
}

/**
 * @brief 疗程说明页（选择疗程按钮 注册 CLICKED）
 */
static void DysPage1_widget(Instr_Data_t* param, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    lv_obj_set_style_pad_all(cont, 0, 0);

    Instr_Widget(cont, param);

    lv_obj_add_event_cb(param->Btn, Child_Page_StartBtn_Event_cb, LV_EVENT_ALL, param);

    /* ===== [我们] 按键导航：选择疗程按钮 进 group + 默认焦点 + KEY 导航（ESC 回四宫格） ===== */
    Dys_GroupRegister(param->Btn);
    lv_group_focus_obj(param->Btn);
    lv_obj_add_event_cb(param->Btn, DysPage_Page1_Key_cb, LV_EVENT_KEY, param);
}
