/**
  ******************************************************************************
  * @文件名称   PosReh_En_ui_event.c
  * @文件描述   英文产后康复页事件回调实现（对应中文 PosReh_Page_ui_event.c）。
  *            逻辑与中文一致，仅文本翻译为英文、跳转改英文函数；
  *            返回确认弹窗用英文共享控件 Warn_En_Window。
  ******************************************************************************
  */

#include "PosReh_En_ui_event.h"
#include "PosReh_En_ui.h"
#include "shared_En_ui.h"
#include "basic.h"
#include "menu_ui.h"
#include "menu_ui_event_cb.h"   /* [我们] B5-A Back_Btn_Event_cb */
#include "app_keypad.h"   /* keypad group 注册 */
#include "app_params.h"   /* 参数存储（NVS） */
#include "esp_log.h"
#include <string.h>       /* strcmp */
#include "app_therapy.h"
#include "therapy_pn.h"
#include "voice.h"

/* [我们] 页面全局实例（定义在 PosReh_En_ui.c） */
extern PosReh_Widget_t PosReh_En_Widget;

/* [我们] Page5 返回确认（定义在文件尾部，先声明供 PosReh_En_P5Btn_Event_cb 调用） */
static void PosReh_En_Page5_ShowConfirm(Treat_Timer_Ctx_t* ctx);

/* 英文产后修复页面统一使用英文“暂停”提示，暂停和返回停止共用此入口。 */
static void PosReh_En_PlayPauseVoice(void)
{
    (void)audio_play_request(VOICE_ID_PAUSE_EN, 1500);
}

/**
 * @brief 电刺激治疗页定时器回调（英文文本）
 */
/*static void Trest_En_Widget_timer_cb(lv_timer_t* t)
{
    Treat_Timer_Ctx_t* ctx = lv_timer_get_user_data(t);

    // 如果不是运行状态，直接返回（防止暂停后还跑）
    if (ctx->status != TIMER_RUNNING) return;

    // 计算距离上次更新过了多少毫秒
    uint32_t elapsed = lv_tick_elaps(ctx->last_tick);
    if (elapsed >= 10) { // 每10毫秒更新一次（对应显示的两位毫秒）
        ctx->remain_sec -= elapsed;
        ctx->last_tick = lv_tick_get();
        // 分解时间：分 / 秒 / 百分之一秒（两位数毫秒）
        [我们] -Werror=format：int32_t=long，%d → (int) */
     /*   lv_label_set_text_fmt(ctx->treat_data.Label_RemTime, "%02d:%02d:%02d",
            (int)(ctx->remain_sec / 60000),
            (int)((ctx->remain_sec % 60000) / 1000),
            (int)((ctx->remain_sec % 1000) / 10));
        // 更新显示
        if (ctx->remain_sec <= 0) {
            ctx->remain_sec = 0;
            ctx->status = TIMER_END;

            lv_timer_del(ctx->timer_Treat);
            ctx->timer_Treat = NULL;

            //  结束时：两个按钮都隐藏掉 将返回按钮放大居中
            lv_label_set_text(ctx->treat_data.Label_RemTime, "Done");
            lv_label_set_text(ctx->treat_data.Label_Hint, "Great job!");
            audio_play_request(VOICE_ID_DONE_EN, 1500);
            lv_obj_add_flag(ctx->treat_data.Start_Btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ctx->treat_data.Pause_Btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(ctx->treat_data.Back_Btn, 150, 220);
            lv_obj_set_size(ctx->treat_data.Back_Btn, 162, 40);
        }
    }
}*/

/**
 * @brief 电刺激治疗页定时器回调（英文文本）- 由 app_therapy 引擎驱动倒计时
 */
static void Trest_En_Widget_timer_cb(lv_timer_t* t)
{
    Treat_Timer_Ctx_t* ctx = lv_timer_get_user_data(t);
    /* 真实下发会话由 app_therapy 引擎驱动倒计时，
     * 本回调只渲染（不自减）；引擎相位切换时已更新 Intens_Value -> 强度标签跟随 */
    if (therapy_active() || ctx->status == TIMER_END) {
        lv_label_set_text_fmt(ctx->treat_data.Label_RemTime, "%02d:%02d:%02d",
            (int)(ctx->remain_sec / 60000),
            (int)((ctx->remain_sec % 60000) / 1000),
            (int)((ctx->remain_sec % 1000) / 10));
        /* 强度标签跟随当前相位（引擎已更新 Intens_Value） */
        if (ctx->treat_data.Label_Intens) {
            lv_label_set_text_fmt(ctx->treat_data.Label_Intens, "%d", (int)ctx->treat_data.Intens_Value);
        }
        /* 阶段C：当前步骤参数跟随（脉宽/频率随阶段切换显示；引擎当前步骤值） */
        if (therapy_active()) {
            if (ctx->treat_data.Label_Pulse)
                lv_label_set_text_fmt(ctx->treat_data.Label_Pulse, "%duS", (int)therapy_current_pw());
            if (ctx->treat_data.Label_Freq)
                lv_label_set_text_fmt(ctx->treat_data.Label_Freq, "%dHz", (int)therapy_current_freq());
            if (ctx->treat_data.Label_stage_stime)
                lv_label_set_text_fmt(ctx->treat_data.Label_stage_stime, "Current Phase:%d", (int)therapy_current_stage_stim());
        }
        if (ctx->status == TIMER_END) {
            audio_play_request(VOICE_ID_DONE_EN, 1500);   //语音播放：结束
            //  结束时：两个按钮都隐藏掉 将返回按钮放大居中
            lv_obj_add_flag(ctx->treat_data.Start_Btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ctx->treat_data.Pause_Btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(ctx->treat_data.Back_Btn, 150, 220);
            lv_obj_set_size(ctx->treat_data.Back_Btn, 162, 40);
            lv_label_set_text(ctx->treat_data.Label_RemTime, "Done");
            lv_label_set_text(ctx->treat_data.Label_Hint, "Great job!");
        
            if (ctx->timer_Treat) { lv_timer_del(ctx->timer_Treat); ctx->timer_Treat = NULL; }
        }
        return;
    }
}

/**
 * @brief 英文康复治疗页按钮（开始/暂停/返回）事件回调
 */
void PosReh_En_P5Btn_Event_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    uint8_t value = 0;

    if (code == LV_EVENT_CLICKED) {
        value = ctx->treat_data.Intens_Value;

        if (btn == ctx->treat_data.Start_Btn) {
            ctx->status = TIMER_RUNNING;
            ctx->last_tick = lv_tick_get();
            if (ctx->timer_Treat == NULL) {
                ctx->timer_Treat = lv_timer_create(Trest_En_Widget_timer_cb, 1, ctx);
            }
            lv_obj_set_style_text_color(ctx->treat_data.Label_RemTime, lv_color_hex(ctx->treat_data.Color), LV_PART_MAIN);
            int ts = therapy_status();
            if (ts == TIMER_PAUSED) {
                audio_play_request(VOICE_ID_START_EN, 1500);
                therapy_resume();                            
            } else if (ts != TIMER_RUNNING) {
                uint8_t func = PosReh_Get_CurFunc();
                Param_Data_t *params = PosReh_En_Param_list[func];
                therapy_start(&rx_pn.schemes[func],
                            (int)params->Value_Intens1,
                            (int)params->Value_Intens2, ctx);
            }
        }
        else if (btn == ctx->treat_data.Pause_Btn) {
            ctx->status = TIMER_PAUSED;
            lv_obj_set_style_text_color(ctx->treat_data.Label_RemTime, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
            therapy_pause(); 
            // PosReh_En_PlayPauseVoice();
            audio_play_request(VOICE_ID_PAUSE_EN, 1500);
        }
        else if (btn == ctx->treat_data.Back_Btn) {
            ctx->status = TIMER_PAUSED;
            therapy_pause(); 
            if (ctx->remain_sec == 0) {
                ctx->remain_sec = ctx->total_sec;

                //  结束时：两个按钮都隐藏掉 将返回按钮放大居中
                lv_label_set_text(ctx->treat_data.Label_RemTime, "00:00:00");
                lv_label_set_text(ctx->treat_data.Label_Hint, "Remaining Time");
                lv_obj_remove_flag(ctx->treat_data.Start_Btn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(ctx->treat_data.Pause_Btn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_pos(ctx->treat_data.Back_Btn, 290, 220);
                lv_obj_set_size(ctx->treat_data.Back_Btn, 100, 40);

                /* [我们] 坑 B4-2：BackToMenu 已 Page_Clean 删除本页对象，
                 * 必须提前 return，否则末尾访问已删 Label_Intens → use-after-free 崩溃 */
                PosReh_En_BackToMenu();
                return;
            }
            else {
                PosReh_En_Page5_ShowConfirm(ctx);
            }
        }

        lv_label_set_text_fmt(ctx->treat_data.Label_Intens, "%d", value);   //更新显示强度值
    }
}

/**
 * @brief 英文参数页设置按钮事件回调（阶段推进 / 进入治疗）
 */
void PosReh_En_P4SetBtn_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        if (data->Set_Btn == obj) {
            if (strcmp(lv_label_get_text(data->Label_Set), "Setup Complete, Next") == 0 ||
                strcmp(lv_label_get_text(data->Label_Set), "Setup Complete, Start") == 0) {
                data->Value_Stage = 1;
                therapy_send_intensity(0,data->Value_Freq,data->Value_Pulse);
                PosReh_En_Page_Load(PosRehPage5);
            }
        }
        /* [我们] 参数存储：确认/进入治疗时保存当前功能治疗参数 */
        if (data != NULL) {
            treat_param_t t = {
                .intens1 = data->Value_Intens1,
                .intens2 = data->Value_Intens2,
                .freq    = (int32_t)data->Value_Freq,
                .pulse   = (int32_t)data->Value_Pulse,
                .stage   = data->Value_Stage,
            };
            app_params_set_treat((int)PosReh_Get_CurFunc(), &t);
            app_params_save_treat((int)PosReh_Get_CurFunc());
        }
    }
}

/**
 * @brief 英文阶段疗程说明页事件回调（单次/10次 → 参数设置页）
 */
void PosReh_En_TreIns_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        if (data->History_Btn == obj) {     //单次治疗
            PosReh_En_Page_Load(PosRehPage4);
        }
        else if (data->Start_Btn == obj) {  //10次治疗
            PosReh_En_Page_Load(PosRehPage4);
        }
    }
}

/**
 * @brief 设置说明页控件样式（英文页用）
 */
static void PosReh_En_Set_SelTreat_Style(PosReh_Widget_t* data, uint8_t flag, uint8_t temp)
{
    if (flag) {
        /* [我们] 坑 B4-1：lv_group_add_obj 空 group 自动聚焦会同步触发 FOCUSED，
         * 而 Page2 循环内 Label_Func/Label_NO 在注册之后才赋值（NULL）→ 必须防护 */
        if (lv_obj_is_valid(data->SelTreat_Data[temp].Label_Func))
            lv_obj_set_style_text_color(data->SelTreat_Data[temp].Label_Func, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
        if (lv_obj_is_valid(data->SelTreat_Data[temp].Label_NO))
            lv_obj_set_style_text_color(data->SelTreat_Data[temp].Label_NO, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
        if (lv_obj_is_valid(data->SelTreat_Data[temp].Obj_Select))
            lv_obj_set_style_border_color(data->SelTreat_Data[temp].Obj_Select, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    }
    else {
        /* [我们] lv_obj_is_valid 防悬垂（坑 B2-4） */
        for (int8_t i = 0; i < 3; i++) {
            if (lv_obj_is_valid(data->SelTreat_Data[i].Label_Func))
                lv_obj_set_style_text_color(data->SelTreat_Data[i].Label_Func, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
            if (lv_obj_is_valid(data->SelTreat_Data[i].Label_NO))
                lv_obj_set_style_text_color(data->SelTreat_Data[i].Label_NO, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
            if (lv_obj_is_valid(data->SelTreat_Data[i].Obj_Select))
                lv_obj_set_style_border_color(data->SelTreat_Data[i].Obj_Select, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        }
    }
}

/**
 * @brief 英文产后康复疗程选择事件回调
 */
void PosReh_En_SelTreatPage_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    PosReh_Widget_t* data = (PosReh_Widget_t*)lv_event_get_user_data(e);

    /* [我们] 同事原用 LV_EVENT_VALUE_CHANGED（按钮不触发），改 FOCUSED 才能按键高亮 */
    if (code == LV_EVENT_FOCUSED) {
        PosReh_En_Set_SelTreat_Style(data,0,0);
        if (lv_obj_is_valid(data->SelTreat_Data[DiaRecti].Btn_Func) && data->SelTreat_Data[DiaRecti].Btn_Func == obj) {
            PosReh_En_Set_SelTreat_Style(data, 1, DiaRecti);
        }
        else if (lv_obj_is_valid(data->SelTreat_Data[LacPro].Btn_Func) && data->SelTreat_Data[LacPro].Btn_Func == obj) {
            PosReh_En_Set_SelTreat_Style(data, 1, LacPro);
        }
        else if (lv_obj_is_valid(data->SelTreat_Data[UterInvo].Btn_Func) && data->SelTreat_Data[UterInvo].Btn_Func == obj) {
            PosReh_En_Set_SelTreat_Style(data, 1, UterInvo);
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        if (data->SelTreat_Data[DiaRecti].Btn_Func == obj) {        //腹直肌分离
            PosReh_Set_CurFunc(DiaRecti);
            PosReh_En_Page_Load(PosRehPage3);
            therapy_init_scheme(&rx_pn.schemes[DiaRecti]);
        }
        else if (data->SelTreat_Data[LacPro].Btn_Func == obj) {     //子宫复旧
            PosReh_Set_CurFunc(LacPro);
            PosReh_En_Page_Load(PosRehPage3);
            therapy_init_scheme(&rx_pn.schemes[LacPro]);
        }
        else if (data->SelTreat_Data[UterInvo].Btn_Func == obj) {   //催乳
            PosReh_Set_CurFunc(UterInvo);
            PosReh_En_Page_Load(PosRehPage3);
            therapy_init_scheme(&rx_pn.schemes[UterInvo]);
        }
    }
}

/**
 * @brief 设置说明页控件未选中样式（英文页用）
 */
static void PosReh_En_Set_Instr_UnSel_Style(PosReh_Widget_t* data)
{
    /* [我们] lv_obj_is_valid 防悬垂（坑 B2-4） */
    if (lv_obj_is_valid(data->InsData.Label_DiaRecti))
        lv_obj_set_style_text_color(data->InsData.Label_DiaRecti, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    if (lv_obj_is_valid(data->InsData.Btn_DiaRecti))
        lv_obj_set_style_bg_color(data->InsData.Btn_DiaRecti, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);

    if (lv_obj_is_valid(data->InsData.Label_UterInvo))
        lv_obj_set_style_text_color(data->InsData.Label_UterInvo, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    if (lv_obj_is_valid(data->InsData.Btn_UterInvo))
        lv_obj_set_style_bg_color(data->InsData.Btn_UterInvo, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);

    if (lv_obj_is_valid(data->InsData.Label_LacPro))
        lv_obj_set_style_text_color(data->InsData.Label_LacPro, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    if (lv_obj_is_valid(data->InsData.Btn_LacPro))
        lv_obj_set_style_bg_color(data->InsData.Btn_LacPro, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);

    if (lv_obj_is_valid(data->InsData.Label_Select))
        lv_obj_set_style_text_color(data->InsData.Label_Select, lv_color_hex(FONT_YELLOW_COLOR), LV_PART_MAIN);
    if (lv_obj_is_valid(data->InsData.Btn_Select))
        lv_obj_set_style_bg_color(data->InsData.Btn_Select, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
}

/**
 * @brief 英文产后康复说明页事件回调
 */
void PosReh_En_InstrPage_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    PosReh_Widget_t* data = (PosReh_Widget_t*)lv_event_get_user_data(e);

    /* [我们] 同事原用 LV_EVENT_VALUE_CHANGED（按钮不触发），改 FOCUSED 才能按键高亮 */
    if (code == LV_EVENT_FOCUSED) {
        PosReh_En_Set_Instr_UnSel_Style(data);
        if (lv_obj_is_valid(data->InsData.Btn_DiaRecti) && data->InsData.Btn_DiaRecti == obj) {
            lv_obj_set_style_text_color(data->InsData.Label_DiaRecti, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->InsData.Btn_DiaRecti, lv_color_hex(FONT_YELLOW_COLOR), LV_STATE_DEFAULT);
        }
        else if (lv_obj_is_valid(data->InsData.Btn_LacPro) && data->InsData.Btn_LacPro == obj) {
            lv_obj_set_style_text_color(data->InsData.Label_LacPro, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->InsData.Btn_LacPro, lv_color_hex(FONT_YELLOW_COLOR), LV_STATE_DEFAULT);
        }
        else if (lv_obj_is_valid(data->InsData.Btn_UterInvo) && data->InsData.Btn_UterInvo == obj) {
            lv_obj_set_style_text_color(data->InsData.Label_UterInvo, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->InsData.Btn_UterInvo, lv_color_hex(FONT_YELLOW_COLOR), LV_STATE_DEFAULT);
        }
        else if (lv_obj_is_valid(data->InsData.Btn_Select) && data->InsData.Btn_Select == obj) {
            lv_obj_set_style_text_color(data->InsData.Label_Select, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->InsData.Btn_Select, lv_color_hex(FONT_YELLOW_COLOR), LV_STATE_DEFAULT);
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        if (data->InsData.Btn_Select == obj) {
            PosReh_En_Page_Load(PosRehPage2);
        }
    }
}

/* ============================================================================
 * [我们] B4 按键导航回调（英文页）
 * ==========================================================================*/

/* Page5 返回确认：显示 Back_En_Window + Yes/No 进 group（默认聚焦 Yes），ESC=No */
static void PosReh_En_Page5_ShowConfirm(Treat_Timer_Ctx_t* ctx)
{
    Back_En_Window(&ctx->Window, "Exit treatment?");
    lv_group_t *g = app_keypad_get_group();
    if (g && ctx->Window.Yes_Btn && lv_obj_is_valid(ctx->Window.Yes_Btn)) {
        lv_group_add_obj(g, ctx->Window.Yes_Btn);
        lv_group_add_obj(g, ctx->Window.No_Btn);
        lv_group_focus_obj(ctx->Window.Yes_Btn);
    }
    /* CLICKED：是→移出group+关窗+回菜单（Back_Btn_Event_cb），否→仅关窗 */
    lv_obj_add_event_cb(ctx->Window.Yes_Btn, Back_Btn_Event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(ctx->Window.No_Btn,  Back_Btn_Event_cb, LV_EVENT_CLICKED, ctx);
    /* KEY：ESC=No（关闭弹窗，页面对象仍在 group → 焦点自然回落到页面） */
    lv_obj_add_event_cb(ctx->Window.Yes_Btn, PosReh_En_MsgBox_Key_cb, LV_EVENT_KEY, ctx);
    lv_obj_add_event_cb(ctx->Window.No_Btn,  PosReh_En_MsgBox_Key_cb, LV_EVENT_KEY, ctx);
}

/* 确认框键盘：ESC=否（关闭弹窗，页面对象仍在 group → 焦点自然回落到页面） */
void PosReh_En_MsgBox_Key_cb(lv_event_t* e)
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

/* Page1 说明页：UP/DOWN 切 3 功能按钮，LEFT/RIGHT 进出"选择"，ESC 回菜单 */
void PosReh_En_Page1_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    PosReh_Widget_t* data = &PosReh_En_Widget;

    if (key == LV_KEY_ESC) { PosReh_En_BackToMenu(); return; }

    if (obj == data->InsData.Btn_Select) {
        if (key == LV_KEY_LEFT && lv_obj_is_valid(data->InsData.Btn_DiaRecti))
            lv_group_focus_obj(data->InsData.Btn_DiaRecti);
    }
    else if (obj == data->InsData.Btn_DiaRecti) {
        if (key == LV_KEY_DOWN && lv_obj_is_valid(data->InsData.Btn_UterInvo)) lv_group_focus_obj(data->InsData.Btn_UterInvo);
        else if (key == LV_KEY_RIGHT && lv_obj_is_valid(data->InsData.Btn_Select)) lv_group_focus_obj(data->InsData.Btn_Select);
    }
    else if (obj == data->InsData.Btn_UterInvo) {
        if (key == LV_KEY_UP && lv_obj_is_valid(data->InsData.Btn_DiaRecti)) lv_group_focus_obj(data->InsData.Btn_DiaRecti);
        else if (key == LV_KEY_DOWN && lv_obj_is_valid(data->InsData.Btn_LacPro)) lv_group_focus_obj(data->InsData.Btn_LacPro);
        else if (key == LV_KEY_RIGHT && lv_obj_is_valid(data->InsData.Btn_Select)) lv_group_focus_obj(data->InsData.Btn_Select);
    }
    else if (obj == data->InsData.Btn_LacPro) {
        if (key == LV_KEY_UP && lv_obj_is_valid(data->InsData.Btn_UterInvo)) lv_group_focus_obj(data->InsData.Btn_UterInvo);
        else if (key == LV_KEY_RIGHT && lv_obj_is_valid(data->InsData.Btn_Select)) lv_group_focus_obj(data->InsData.Btn_Select);
    }
}

/* Page2 功能选择：LEFT/RIGHT 切换 3 功能，ENTER 由 PosReh_En_SelTreatPage_Event_cb 处理，ESC 回说明页 */
void PosReh_En_Page2_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    PosReh_Widget_t* data = &PosReh_En_Widget;

    if (key == LV_KEY_ESC) { PosReh_En_Page_Load(PosRehPage1); return; }

    if (key == LV_KEY_RIGHT) {
        if (obj == data->SelTreat_Data[DiaRecti].Btn_Func) lv_group_focus_obj(data->SelTreat_Data[LacPro].Btn_Func);
        else if (obj == data->SelTreat_Data[LacPro].Btn_Func) lv_group_focus_obj(data->SelTreat_Data[UterInvo].Btn_Func);
    }
    else if (key == LV_KEY_LEFT) {
        if (obj == data->SelTreat_Data[UterInvo].Btn_Func) lv_group_focus_obj(data->SelTreat_Data[LacPro].Btn_Func);
        else if (obj == data->SelTreat_Data[LacPro].Btn_Func) lv_group_focus_obj(data->SelTreat_Data[DiaRecti].Btn_Func);
    }
}

/* Page3 治疗说明：LEFT/RIGHT 切换 History/Start，ENTER 由 PosReh_En_TreIns_Event_cb 处理，ESC 回功能选择 */
void PosReh_En_Page3_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { PosReh_En_Page_Load(PosRehPage2); return; }

    if (key == LV_KEY_LEFT && obj == data->Start_Btn) lv_group_focus_obj(data->History_Btn);
    else if (key == LV_KEY_RIGHT && obj == data->History_Btn) lv_group_focus_obj(data->Start_Btn);
}

/* Page4 参数设置：[我们] 滑条不进 group；减/加/设置 进 group：
 * 左右=调档位、上下=线性切按钮(减→加→设置)、ENTER=按钮动作、ESC 回 Page3 */
void PosReh_En_Page4_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { PosReh_En_Page_Load(PosRehPage3); return; }

    // /* 左右 = 调档位（滑条值 ±1） */
    // if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
    //     int32_t v = lv_slider_get_value(data->Slider);
    //     v += (key == LV_KEY_RIGHT) ? 1 : -1;
    //     if (v < 0) v = 0;
    //     if (v > 10) v = 10;
    //     lv_slider_set_value(data->Slider, v, LV_ANIM_OFF);
    //     /* [我们] ⚠️ LVGL9.5 lv_slider_set_value(ANIM_OFF) 不发 VALUE_CHANGED（lv_bar.c 确认），
    //      * 手动更新强度值 + 标签 */
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

/* Page5 治疗中：LEFT/RIGHT 切焦点，ENTER 由 PosReh_En_P5Btn_Event_cb 处理，ESC=返回确认 */
void PosReh_En_Page5_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) {
        if (ctx->remain_sec == 0) PosReh_En_BackToMenu();
        else{
            therapy_pause();
            PosReh_En_Page5_ShowConfirm(ctx);
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
