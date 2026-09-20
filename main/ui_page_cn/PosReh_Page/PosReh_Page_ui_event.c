#include "PosReh_Page_ui_event.h"
#include "PosReh_Page_ui.h"
#include "basic.h"
#include "menu_ui.h"
#include "menu_ui_event_cb.h"   /* [我们] B5-A Back_Btn_Event_cb */
#include "app_keypad.h"   /* [我们] keypad group 注册 */
#include "app_params.h"   /* [我们] 参数存储（NVS） */
#include "esp_log.h"      /* [我们] 日志 */
#include <string.h>        /* [我们] strcmp */
#include "app_therapy.h" 
#include "voice.h"
#include "therapy_pn.h"

/* [我们] 页面全局实例（定义在 PosReh_Page_ui.c） */
extern PosReh_Widget_t PosReh_Widget;

/* [我们] Page5 返回确认（定义在文件尾部，先声明供 PosReh_P5Btn_Event_cb 调用） */
static void PosReh_Page5_ShowConfirm(Treat_Timer_Ctx_t* ctx);


/**
 * @author zqt
 * @brief 电刺激治疗页定时器回调函数
 * @param e
 */
static void Trest_Widget_timer_cb(lv_timer_t* t)
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
        /* [我们] 阶段C：当前步骤参数跟随（脉宽/频率随阶段切换显示；引擎当前步骤值） */
        if (therapy_active()) {
            if (ctx->treat_data.Label_Pulse)
                lv_label_set_text_fmt(ctx->treat_data.Label_Pulse, "%duS", (int)therapy_current_pw());
            if (ctx->treat_data.Label_Freq)
                lv_label_set_text_fmt(ctx->treat_data.Label_Freq, "%dHz", (int)therapy_current_freq());
            if (ctx->treat_data.Label_stage_stime)
                lv_label_set_text_fmt(ctx->treat_data.Label_stage_stime, "当前阶段:%d", (int)therapy_current_stage_stim());
        }
        if (ctx->status == TIMER_END) {
            audio_play_request(VOICE_ID_DONE_CN,1500);              //语音播放：暂停
            //  结束时：两个按钮都隐藏掉 将返回按钮放大居中
            lv_obj_add_flag(ctx->treat_data.Start_Btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ctx->treat_data.Pause_Btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(ctx->treat_data.Back_Btn, 150, 220);
            lv_obj_set_size(ctx->treat_data.Back_Btn, 162, 40);
            lv_label_set_text(ctx->treat_data.Label_RemTime, "已完成");
            lv_label_set_text(ctx->treat_data.Label_Hint, "很棒哦~");
        
            if (ctx->timer_Treat) { lv_timer_del(ctx->timer_Treat); ctx->timer_Treat = NULL; }
        }
        return;
    }
    // // 如果不是运行状态，直接返回（防止暂停后还跑）
    // if (ctx->status != TIMER_RUNNING) return;

    // // 计算距离上次更新过了多少毫秒
    // uint32_t elapsed = lv_tick_elaps(ctx->last_tick);
    // if (elapsed >= 10) { // 每10毫秒更新一次（对应显示的两位毫秒）
    //     ctx->remain_sec -= elapsed;
    //     ctx->last_tick = lv_tick_get();
    //     // 分解时间：分 / 秒 / 百分之一秒（两位数毫秒）
    //     /* [我们] -Werror=format：int32_t=long，%d → (int) */
    //     lv_label_set_text_fmt(ctx->treat_data.Label_RemTime, "%02d:%02d:%02d",
    //         (int)(ctx->remain_sec / 60000),
    //         (int)((ctx->remain_sec % 60000) / 1000),
    //         (int)((ctx->remain_sec % 1000) / 10));   //除以10得到两位数毫秒
    //     // 更新显示
    //     if (ctx->remain_sec <= 0) {
    //         ctx->remain_sec = 0;
    //         ctx->status = TIMER_END;

    //         lv_timer_del(ctx->timer_Treat);
    //         ctx->timer_Treat = NULL;

    //         //  结束时：两个按钮都隐藏掉 将返回按钮放大居中
    //         lv_label_set_text(ctx->treat_data.Label_RemTime, "已完成");
    //         lv_label_set_text(ctx->treat_data.Label_Hint, "很棒哦~");
    //         lv_obj_add_flag(ctx->treat_data.Start_Btn, LV_OBJ_FLAG_HIDDEN);
    //         lv_obj_add_flag(ctx->treat_data.Pause_Btn, LV_OBJ_FLAG_HIDDEN);
    //         lv_obj_set_pos(ctx->treat_data.Back_Btn, 150, 220);
    //         lv_obj_set_size(ctx->treat_data.Back_Btn, 162, 40);
    //     }
    // }
}
/**
 * @author zqt
 * @brief 康复 治疗页按钮(开始、暂停、返回、是/否)事件回调函数
 * @param e
 */
void PosReh_P5Btn_Event_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);     // 用户传进来的数据

    uint8_t value = 0;

    if (code == LV_EVENT_CLICKED) {
        value = ctx->treat_data.Intens_Value;

        if (btn == ctx->treat_data.Start_Btn) {
            ctx->status = TIMER_RUNNING;        // 点击开始 -> 进入运行状态
            ctx->last_tick = lv_tick_get();     // 重置基准时间
            // 创建定时器（每10ms刷新一次UI，保证秒数切换流畅）
            if (ctx->timer_Treat == NULL) {
                ctx->timer_Treat = lv_timer_create(Trest_Widget_timer_cb, 1, ctx);
            }
            lv_obj_set_style_text_color(ctx->treat_data.Label_RemTime, lv_color_hex(ctx->treat_data.Color), LV_PART_MAIN);
            
            /*  暂停中=恢复（续跑）；未开始=新会话；已运行中=忽略（防重复点击重置时间） */
            int ts = therapy_status();
            if (ts == TIMER_PAUSED) {
                audio_play_request(VOICE_ID_START_CN,1500);      //语音播放：开始 
                therapy_resume();                            
            } else if (ts != TIMER_RUNNING) {
                uint8_t func = PosReh_Get_CurFunc();
                therapy_start(&rx_pn.schemes[func],
                            (int)PosReh_Param_Data1.Value_Intens1,
                            (int)PosReh_Param_Data1.Value_Intens2, ctx);
            }
        }
        else if (btn == ctx->treat_data.Pause_Btn) {
            ctx->status = TIMER_PAUSED;
            lv_obj_set_style_text_color(ctx->treat_data.Label_RemTime, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
            therapy_pause();
            audio_play_request(VOICE_ID_PAUSE_CN,1500);              //语音播放：暂停
        }
        else if (btn == ctx->treat_data.Back_Btn) {
            ctx->status = TIMER_PAUSED;
            therapy_pause();
            if (ctx->remain_sec == 0) {
                ctx->remain_sec = ctx->total_sec;
             
                //  结束时：两个按钮都隐藏掉 将返回按钮放大居中
                lv_label_set_text(ctx->treat_data.Label_RemTime, "00:00:00");
                lv_label_set_text(ctx->treat_data.Label_Hint, "剩余时间");
                lv_obj_remove_flag(ctx->treat_data.Start_Btn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_remove_flag(ctx->treat_data.Pause_Btn, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_pos(ctx->treat_data.Back_Btn, 290, 220);
                lv_obj_set_size(ctx->treat_data.Back_Btn, 100, 40);

                /* [我们] 坑 B4-2：BackToMenu 已 Page_Clean 删除本页对象，
                 * 必须提前 return，否则末尾访问已删 Label_Intens → use-after-free 崩溃 */
                PosReh_BackToMenu();
                return;
            }
            else {
                PosReh_Page5_ShowConfirm(ctx);
            }
        }

        lv_label_set_text_fmt(ctx->treat_data.Label_Intens, "%d", value);   //更新显示强度值
    }
}

/**
 * @author zqt
 * @brief 康复 参数页设置按钮事件回调函数
 * @param e
 */
void PosReh_P4SetBtn_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);     // 用户传进来的数据

    if (code == LV_EVENT_CLICKED) {
        if (data->Set_Btn == obj) {     //擦看历史按钮
            if (strcmp(lv_label_get_text(data->Label_Set), "设置完成，下一步") == 0) {
                therapy_send_intensity(0,data->Value_Freq,data->Value_Pulse);
                PosReh_Page_Load(PosRehPage5);
            }
            else if (strcmp(lv_label_get_text(data->Label_Set), "设置完成，进入治疗") == 0) { // 跳转到下一页
                PosReh_Page_Load(PosRehPage5);
                therapy_send_intensity(0,data->Value_Freq,data->Value_Pulse);
            }
        }
        /* [我们] 参数存储：确认/进入治疗时保存当前功能治疗参数（强度/频率/脉宽/阶段） */
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
 * @author zqt
 * @brief 阶段疗程说明页事件回调函数
 * @param e
 */
void PosReh_TreIns_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);     // 用户传进来的数据

    if (code == LV_EVENT_CLICKED) {
        /* [我们] 单次治疗也能进入（同事原 History_Btn 空分支） */
        if (data->History_Btn == obj) {     //单次治疗
            PosReh_Page_Load(PosRehPage4);
        }
        else if (data->Start_Btn == obj) {  //10次治疗
            PosReh_Page_Load(PosRehPage4);
        }
    }
}


/**
 * @brief 设置说明页控件样式
 * @param data
 */
static void Set_SelTreat_Style(PosReh_Widget_t* data,uint8_t flag, uint8_t temp)
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
        /* [我们] lv_obj_is_valid 防悬垂（坑 B2-4）：清除样式时可能页面已切换 */
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
 * @author zqt
 * @brief 产后康复疗程选择事件回调函数
 * @param e
 */
void SelTreatPage_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    PosReh_Widget_t* data = (PosReh_Widget_t*)lv_event_get_user_data(e);        // 用户传进来的数据

    /* [我们] 同事原用 LV_EVENT_VALUE_CHANGED（按钮不触发），改 FOCUSED 才能按键高亮 */
    if (code == LV_EVENT_FOCUSED) {                             //对焦事件
        Set_SelTreat_Style(data,0,0);                           //设置全部控件未选择样式
        if (lv_obj_is_valid(data->SelTreat_Data[DiaRecti].Btn_Func) && data->SelTreat_Data[DiaRecti].Btn_Func == obj) {    //腹直肌分离
            Set_SelTreat_Style(data, 1, DiaRecti);              //设置腹直肌分离选中样式
        }
        else if (lv_obj_is_valid(data->SelTreat_Data[LacPro].Btn_Func) && data->SelTreat_Data[LacPro].Btn_Func == obj) {
            Set_SelTreat_Style(data, 1, LacPro);                //设置子宫复旧选中样式
        }
        else if (lv_obj_is_valid(data->SelTreat_Data[UterInvo].Btn_Func) && data->SelTreat_Data[UterInvo].Btn_Func == obj) {
            Set_SelTreat_Style(data, 1, UterInvo);              //设置催乳选中样式
        }

    }
    else if (code == LV_EVENT_CLICKED) {                        //点击事件
        if (data->SelTreat_Data[DiaRecti].Btn_Func == obj) {        //腹直肌分离
            PosReh_Set_CurFunc(DiaRecti);
            therapy_init_scheme(&rx_pn.schemes[DiaRecti]);
            PosReh_Page_Load(PosRehPage3);
        }
        else if (data->SelTreat_Data[LacPro].Btn_Func == obj) {     //子宫复旧
            PosReh_Set_CurFunc(LacPro);
            therapy_init_scheme(&rx_pn.schemes[LacPro]);
            PosReh_Page_Load(PosRehPage3);

        }
        else if (data->SelTreat_Data[UterInvo].Btn_Func == obj) {   //催乳
            PosReh_Set_CurFunc(UterInvo);
            therapy_init_scheme(&rx_pn.schemes[UterInvo]);
            PosReh_Page_Load(PosRehPage3);
        }

    }
}

/**
 * @brief 设置说明页控件未选中样式
 * @param data
 */
static void Set_Instr_UnSel_Style(PosReh_Widget_t* data)
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
 * @author zqt
 * @brief 产后康复说明页事件回调函数
 * @param e
 */
void InstrPage_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    PosReh_Widget_t* data = (PosReh_Widget_t*)lv_event_get_user_data(e);        // 用户传进来的数据

    /* [我们] 同事原用 LV_EVENT_VALUE_CHANGED（按钮不触发），改 FOCUSED 才能按键高亮 */
    if (code == LV_EVENT_FOCUSED) {                         //对焦事件
        Set_Instr_UnSel_Style(data);
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
    else if (code == LV_EVENT_CLICKED) {                    //点击事件
        if (data->InsData.Btn_Select == obj) {
            PosReh_Page_Load(PosRehPage2);
       }

    }
}

/* ============================================================================
 * [我们] B4 按键导航回调（坑 6.8 / B2-4 / B3-3 通用守则）
 * ==========================================================================*/

/* Page5 返回确认：显示 Back_Window + 是/否 进 group（默认聚焦“是”），ESC=否 */
static void PosReh_Page5_ShowConfirm(Treat_Timer_Ctx_t* ctx)
{
    Back_Window(&ctx->Window, "是否确认返回？");
    lv_group_t *g = app_keypad_get_group();
    if (g && ctx->Window.Yes_Btn && lv_obj_is_valid(ctx->Window.Yes_Btn)) {
        lv_group_add_obj(g, ctx->Window.Yes_Btn);
        lv_group_add_obj(g, ctx->Window.No_Btn);
        lv_group_focus_obj(ctx->Window.Yes_Btn);
    }
    /* CLICKED：是→移出group+关窗+回菜单（Back_Btn_Event_cb），否→仅关窗 */
    lv_obj_add_event_cb(ctx->Window.Yes_Btn, Back_Btn_Event_cb, LV_EVENT_CLICKED, ctx);
    lv_obj_add_event_cb(ctx->Window.No_Btn,  Back_Btn_Event_cb, LV_EVENT_CLICKED, ctx);
    /* KEY：ESC=否（关闭弹窗，页面对象仍在 group → 焦点自然回落到页面） */
    lv_obj_add_event_cb(ctx->Window.Yes_Btn, PosReh_MsgBox_Key_cb, LV_EVENT_KEY, ctx);
    lv_obj_add_event_cb(ctx->Window.No_Btn,  PosReh_MsgBox_Key_cb, LV_EVENT_KEY, ctx);
}

/* 确认框键盘：ESC=否（关闭弹窗，页面对象仍在 group → 焦点自然回落到页面） */
void PosReh_MsgBox_Key_cb(lv_event_t* e)
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

/* Page1 说明页：UP/DOWN 切 3 功能按钮，LEFT/RIGHT 进出“选择”，ESC 回菜单 */
void PosReh_Page1_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    PosReh_Widget_t* data = &PosReh_Widget;

    if (key == LV_KEY_ESC) { PosReh_BackToMenu(); return; }

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

/* Page2 功能选择：LEFT/RIGHT 切换 3 功能，ENTER 由 SelTreatPage_Event_cb 处理，ESC 回说明页 */
void PosReh_Page2_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    PosReh_Widget_t* data = &PosReh_Widget;

    if (key == LV_KEY_ESC) { PosReh_Page_Load(PosRehPage1); return; }

    if (key == LV_KEY_RIGHT) {
        if (obj == data->SelTreat_Data[DiaRecti].Btn_Func) lv_group_focus_obj(data->SelTreat_Data[LacPro].Btn_Func);
        else if (obj == data->SelTreat_Data[LacPro].Btn_Func) lv_group_focus_obj(data->SelTreat_Data[UterInvo].Btn_Func);
    }
    else if (key == LV_KEY_LEFT) {
        if (obj == data->SelTreat_Data[UterInvo].Btn_Func) lv_group_focus_obj(data->SelTreat_Data[LacPro].Btn_Func);
        else if (obj == data->SelTreat_Data[LacPro].Btn_Func) lv_group_focus_obj(data->SelTreat_Data[DiaRecti].Btn_Func);
    }
}

/* Page3 治疗说明：LEFT/RIGHT 切换 History/Start，ENTER 由 PosReh_TreIns_Event_cb 处理，ESC 回功能选择 */
void PosReh_Page3_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { PosReh_Page_Load(PosRehPage2); return; }

    if (key == LV_KEY_LEFT && obj == data->Start_Btn) lv_group_focus_obj(data->History_Btn);
    else if (key == LV_KEY_RIGHT && obj == data->History_Btn) lv_group_focus_obj(data->Start_Btn);
}

/* Page4 参数设置：[我们] 滑条不进 group（LVGL 滑条 4 方向都调值且 class 先执行，无法拦截上下）；
 * 减/加/设置 进 group：左右=调档位、上下=线性切按钮(减→加→设置)、ENTER=按钮动作、ESC 回 Page3 */
void PosReh_Page4_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) { PosReh_Page_Load(PosRehPage3); return; }

    // /* 左右 = 调档位（滑条值 ±1） */
    // if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
    //     int32_t v = lv_slider_get_value(data->Slider);
    //     v += (key == LV_KEY_RIGHT) ? 1 : -1;
    //     if (v < 0) v = 0;
    //     if (v > 90) v = 90;
    //     lv_slider_set_value(data->Slider, v, LV_ANIM_OFF);
    //     /* [我们] ⚠️ LVGL9.5 lv_slider_set_value(ANIM_OFF) 不发 VALUE_CHANGED（lv_bar.c 确认），
    //      * 手动更新强度值 + 标签（同 Child_Slider_Event_cb VALUE_CHANGED 分支） */
    //     if (data->Value_Stage == 1)      data->Value_Intens1 = v;
    //     else if (data->Value_Stage == 2) data->Value_Intens2 = v;
    //     if (lv_obj_is_valid(data->Label_Intens)) lv_label_set_text_fmt(data->Label_Intens, "%d", (int)v);
    //     lv_event_stop_processing(e);   /* [我们] 调值分支非删对象，stop 无害（坑 B3-3 只禁删对象路径） */
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

/* Page5 治疗中：LEFT/RIGHT 切焦点，ENTER 由 PosReh_P5Btn_Event_cb 处理，ESC=返回确认 */
void PosReh_Page5_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    if (key == LV_KEY_ESC) {
        if (ctx->remain_sec == 0) PosReh_BackToMenu();
        else {
            therapy_pause();
            PosReh_Page5_ShowConfirm(ctx);
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
