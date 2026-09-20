#include "PelFlo_TheChild_event.h"
#include "Child_ThePage.h"
#include "Child_MaiPage.h"   /* [我们] B6-A 保养格 */
#include "Chlid_DysPage.h"   /* [我们] B6-B 障碍格 */
#include "Chlid_KgePage.h"   /* [我们] B6-C 凯格尔格 */
#include "PelFlo_The_ui.h"
#include "app_therapy.h"   /* [我们] 阶段C：治疗控制（处方3疗程1 下发 + 30min 会话） */
#include "menu_ui.h"
#include "menu_ui_event_cb.h"
#include "basic.h"
#include "esp_log.h"
#include <string.h>        /* [我们] strcmp */
#include "voice.h"
#include "emg_force.h"     /* 阶段 3.3: 实时力度包络 */
#include "emg_dsp.h"       /* EMG_DSP_RAW16_LSB_UV 换算 */
#include "app_keypad.h"

static const char *TAG = "TheChild_evt";
#define FILTER_WIN 7  // 滑动窗口大小（3~7 效果较好）
/* [我们] B5-C 前置（0R §9.4）：
 * 已删 B5-B 已实现的重复符号：Chart_Btn_Event_cb / Chart_Draw_event_cb /
 *   Chart3_Stage3_Add_data / Chart_Stage2_4_Add_data / Myadd_faded_area
 *   （均已在 menu_ui_event_cb.c，防链接重复符号）
 * 已删死代码：Set_UnSel_Style / Get_Stage_BtnNum（PC 注释残留，无实现体引用）
 * 本文件保留：Chart_Stage3_Add_data / Chart2_Stage3_Add_data / timer_next_cb /
 *   治疗双 timer Trest_Widget_timer_cb / ChildPage_* / Child_Page_StartBtn_Event_cb /
 *   Get_StageBtn / Set_StageBtn_UnSel（switch 全部简化为 ThePage-only）
 */

/* [我们] static 前向声明（保留函数用） */
static uint8_t Get_StageBtn(Select_Data_t* data, lv_obj_t* btn);
static void Set_StageBtn_UnSel(Select_Data_t* data);
static void timer_next_cb(lv_timer_t* t);
static void Trest_Widget_timer_cb(lv_timer_t* t);


static int32_t filter_buf[FILTER_WIN] = { 0 };
static uint8_t filter_idx = 0;
/**
 * @author zqt
 * @brief 电刺激治疗页定时器回调函数（新版双 timer：结束 → 阶段1「很棒哦~」，阶段2-4「马上进入下一节」→ 曲线图）
 * @param e
 */
static void Trest_Widget_timer_cb(lv_timer_t* t)
{
    Treat_Timer_Ctx_t* ctx = lv_timer_get_user_data(t);

    /* [我们] 阶段C：ThePage 真实下发会话由 app_therapy 引擎驱动倒计时，
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
           
            //  结束时：两个按钮都隐藏掉 将返回按钮放大居中
            lv_obj_add_flag(ctx->treat_data.Start_Btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(ctx->treat_data.Pause_Btn, LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(ctx->treat_data.Back_Btn, 150, 220);
            lv_obj_set_size(ctx->treat_data.Back_Btn, 162, 40);
            lv_label_set_text(ctx->treat_data.Label_RemTime, "已完成");
            
            if (Get_CurStage_Idx() != STAGE_1) {
                therapy_pause();
                audio_play_request(VOICE_ID_NEXT_CN,1500);              //语音播放：下一步
                lv_label_set_text(ctx->treat_data.Label_Hint, "马上进入下一节");
                // 创建下一节倒计时定时器
                if (ctx->timer_Next == NULL) {
                    ctx->timer_Next = lv_timer_create(timer_next_cb, 20, ctx);
                }
            }
            else {
                lv_label_set_text(ctx->treat_data.Label_Hint, "很棒哦~");
                audio_play_request(VOICE_ID_DONE_CN,1500);              //语音播放：完成
            }
            if (ctx->timer_Treat) { lv_timer_del(ctx->timer_Treat); ctx->timer_Treat = NULL; }
        }
        return;
    }

}

/**
 * @author zqt
 * @brief 治疗页按钮(开始、暂停、返回)事件回调函数
 *        [我们] 返回：结束直接回菜单（The_BackToMenu 统一清理），未结束弹键盘化确认
 * @param e
 */
void ChildPage_P5Btn_Event_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);     // 用户传进来的数据

    if (code == LV_EVENT_CLICKED) {
        if (btn == ctx->treat_data.Start_Btn) {
            
            ctx->status = TIMER_RUNNING;        // 点击开始 进入运行状态
            // 创建定时器
            if (ctx->timer_Treat == NULL) {
                ctx->timer_Treat = lv_timer_create(Trest_Widget_timer_cb, 1, ctx);
            }
            lv_obj_set_style_text_color(ctx->treat_data.Label_RemTime, lv_color_hex(ctx->treat_data.Color), LV_PART_MAIN);
           
            /* [我们] 阶段C：ThePage 治疗控制 ——
             * 暂停中=恢复（续跑）；未开始=新会话（处方3疗程1）；已运行中=忽略（防重复点击重置时间） */
            int ts = therapy_status();
            if (ts == TIMER_PAUSED) {
                therapy_resume();   
                audio_play_request(VOICE_ID_START_CN,1500);      //语音播放：开始                          
            }
            else if (ts != TIMER_RUNNING) {
                
                uint8_t schemes_idx = Get_CurTreat_Idx();
                if (Get_CurPage_Idx() == ThePage) {
                    therapy_start(&g_rx_all[RX_3]->schemes[schemes_idx],
                                (int)The_Param_Data1.Value_Intens1,
                                (int)The_Param_Data1.Value_Intens2, ctx);
                } else if (Get_CurPage_Idx() == MaiPage) {
                    therapy_start(&g_rx_all[RX_2]->schemes[schemes_idx],
                                (int)Mai_Param_Data1.Value_Intens1,
                                (int)Mai_Param_Data1.Value_Intens2, ctx);
                }else if (Get_CurPage_Idx() == DysPage) {
                    therapy_start(&g_rx_all[RX_1]->schemes[schemes_idx],
                                (int)Dys_Param_Data1.Value_Intens1,
                                (int)Dys_Param_Data1.Value_Intens2, ctx);
                } else if (Get_CurPage_Idx() == KgePage) {
                    // therapy_start_kegel(&g_rx_all[RX_4]->schemes[schemes_idx]);
                }
               
            }else{
                ESP_LOGI(TAG, "当前治疗状态为运行中，忽略重复点击");
            }
           
        }
        else if (btn == ctx->treat_data.Pause_Btn) {
            ctx->status = TIMER_PAUSED;
            lv_obj_set_style_text_color(ctx->treat_data.Label_RemTime, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
            /* [我们] 阶段C：ThePage 暂停 -> 停波 + 引擎暂停 */
            // if (Get_CurPage_Idx() == ThePage) therapy_pause();
            therapy_pause();
            audio_play_request(VOICE_ID_PAUSE_CN,1500);              //语音播放：暂停
        }
        else if (btn == ctx->treat_data.Back_Btn) {     //返回按钮
            ctx->status = TIMER_PAUSED;
            /* [我们] 阶段C：ThePage 返回 -> 先暂停引擎（停波）；确认"是"→Back_Btn_Event_cb 再 stop */
            // if (Get_CurPage_Idx() == ThePage) therapy_pause();
            therapy_pause();
            if (ctx->remain_sec == 0) {                 //如果治疗结束 点击返回按钮直接返回到菜单页
                ctx->remain_sec = ctx->total_sec;
                /* [我们] B6-C 按当前格分派退出（停对应格 timer + 移对应格 group + 回菜单） */
                ThePage_ID_t page_idx = Get_CurPage_Idx();
                switch (page_idx) {
                case ThePage: The_BackToMenu(); break;
                case MaiPage: Mai_BackToMenu(); break;
                case DysPage: Dys_BackToMenu(); break;
                case KgePage: Kge_BackToMenu(); break;
                default: Goto_MenuPage(); break;
                }
                return;                                 /* 坑 B4-3：删对象路径后禁止再访问页面对象 */
            }
            else {                                      //如果治疗没有结束 弹窗询问
                The_Page5_ShowConfirm(ctx);             /* [我们] 键盘化确认弹窗（是/否进group + ESC） */
            }
        }
        /* 更新显示强度值（[我们] 阶段C：用当前 Intens_Value——Start 后引擎已设为步骤0 档） */
        lv_label_set_text_fmt(ctx->treat_data.Label_Intens, "%d", (int)ctx->treat_data.Intens_Value);
    }
}

/**
 * @author zqt
 * @brief 参数页设置按钮点击事件回调函数
 * @param e
 */
void ChildPage_P4SetBtn_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    Param_Data_t* data = (Param_Data_t*)lv_event_get_user_data(e);     // 用户传进来的数据

    if (code == LV_EVENT_CLICKED) {
       
        if (data->Set_Btn == obj) {     
            if (strcmp(lv_label_get_text(data->Label_Set), "设置完成，下一步") == 0) {
                data->Value_Stage = 2;                                     //设置为阶段2
                data->Value_Intens2 = 0;
                lv_slider_set_value(data->Slider, 0, LV_ANIM_OFF);          //初始化滑动条值为0
                lv_label_set_text(data->Label_Intens, "0");
                lv_label_set_text(data->Label_Stage, "2/2");
                lv_label_set_text(data->Label_Set, "设置完成，进入治疗");
                uint8_t schemes_idx = Get_CurTreat_Idx();
                ThePage_ID_t page_idx = Get_CurPage_Idx();
                uint8_t rx_idx = 0;
                switch (page_idx) {
                case ThePage: rx_idx = RX_3;  break;
                case MaiPage: rx_idx = RX_2; break;
                case DysPage: rx_idx = RX_1;  break;
                case KgePage: rx_idx = RX_4; break;
                default: break;
                }
                data->Value_Pulse = therapy_cur_pw(&g_rx_all[rx_idx]->schemes[schemes_idx],2);
                data->Value_Freq = therapy_cur_freq(&g_rx_all[rx_idx]->schemes[schemes_idx],2);
                lv_label_set_text_fmt(data->Label_Freq, "%dHz",data->Value_Freq);
                lv_label_set_text_fmt(data->Label_Pulse, "%duS",data->Value_Pulse);
                therapy_send_intensity(0,data->Value_Freq,data->Value_Pulse);

                lv_group_t* g = app_keypad_get_group();
                if (g && lv_obj_is_valid(data->Plu_Btn)) {
                lv_group_focus_obj(data->Plu_Btn);
               }
            }
            else if (strcmp(lv_label_get_text(data->Label_Set), "设置完成，进入治疗") == 0) { // 跳转到下一页
                therapy_send_intensity(0,data->Value_Freq,data->Value_Pulse);
                /* [我们] B6-C 按当前格分派 */
                ThePage_ID_t page_idx = Get_CurPage_Idx();
                switch (page_idx) {
                case ThePage: Child_ThePage_Load(ChiThePage_Treat);  break;
                case MaiPage: Child_MaiPage_Load(ChiMaiPage_Treat);  break;
                case DysPage: Child_DysPage_Load(ChiDysPage_Treat);  break;
                case KgePage: Child_KgePage_Load(ChiKgePage_Treat);  break;
                default: break;
                }
            }
        }
    }
}

/**
 * @author zqt
 * @brief 阶段疗程说明页 开始治疗/查看历史 事件回调函数
 *        [我们] History_Btn = stub（0R §6 风险2：PC 空逻辑，真实记录留阶段 D）
 * @param e
 */
void ChildPage_TreIns_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);     // 用户传进来的数据

    if (code == LV_EVENT_CLICKED) {
        if (data->History_Btn == obj) {             //查看历史按钮
            ESP_LOGI(TAG, "History_Btn stub（真实记录列表留阶段 D）");
        }
        else if (data->Start_Btn == obj) {          //开始治疗按钮
            /* [我们] B6-C 按当前格分派 */
            ThePage_ID_t page_idx = Get_CurPage_Idx();
            switch (page_idx) {
            case ThePage: Child_ThePage_Load(ChiThePage_ParSet);  break;
            case MaiPage: Child_MaiPage_Load(ChiMaiPage_ParSet);  break;
            case DysPage: Child_DysPage_Load(ChiDysPage_ParSet);  break;
            case KgePage: {
                /* 凯格尔训练：跳过第4页(ParSet)和第5页(Treat)，直接启动并进入 Chart1 */
                uint8_t schemes_idx = Get_CurTreat_Idx();
                therapy_start_kegel(&g_rx_all[RX_4]->schemes[schemes_idx]);
                Child_KgePage_Load(ChiKgePage_Chart1);
                break;
            }
            default: break;
            }
        }
    }
}
/**
 * @author zqt
 * @brief 阶段疗程选择按钮事件回调函数
 * @param e
 */
void ChildPage_StageBtn_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    Select_Data_t* data = (Select_Data_t*)lv_event_get_user_data(e);        // 用户传进来的数据
   
    uint8_t num = 0;
    if (code == LV_EVENT_KEY) {                         //对焦事件
        num = Get_StageBtn(data, obj);
        LV_LOG_USER("num:%d", (int)num);
        if (num < data->Btn_MaxSum) {
            Set_StageBtn_UnSel(data);
            lv_obj_add_style(data->sta[num].obj, &style_gradient, LV_PART_MAIN);
            lv_obj_set_style_image_recolor(data->sta[num].img, lv_color_hex(FONT_RED_COLOR), 0); //照片重新重色
        }
    }
    else if (code == LV_EVENT_CLICKED) {                        //点击事件
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
        num = Get_StageBtn(data, obj);
        ESP_LOGI(TAG,"num:%d ", (int)num);
        if (num < data->Btn_MaxSum) {
            Set_CurTreat_Idx(num);
            Treat_Point_t* p = &data->Treat_Point[num];     //获取治疗阶段
            ESP_LOGI(TAG,"stage:[%d][%d] ", (int)p->stage, (int)p->time);
            Set_CurStage_Idx(p->stage);
            Set_CurTime_Idx(p->time);
            uint8_t rx_idx = 0;
            /* [我们] B6-C 按当前格分派 */
            ThePage_ID_t page_idx = Get_CurPage_Idx();
            switch (page_idx) {
            case ThePage: Child_ThePage_Load(ChiThePage_TreIns);rx_idx = RX_3; break;
            case MaiPage: Child_MaiPage_Load(ChiMaiPage_TreIns);rx_idx = RX_2;  break;
            case DysPage: Child_DysPage_Load(ChiDysPage_TreIns);rx_idx = RX_1;  break;
            case KgePage: Child_KgePage_Load(ChiKgePage_TreIns);rx_idx = RX_4;  break;
            default: break;
            }
            therapy_init_scheme(&g_rx_all[rx_idx]->schemes[num]);
        }
    }

}

/**
 * @author zqt
 * @brief 开始选择疗程按钮事件回调函数
 * @param e
 */
void Child_Page_StartBtn_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    Instr_Data_t* data = (Instr_Data_t*)lv_event_get_user_data(e);        // 用户传进来的数据
   
    if (code == LV_EVENT_FOCUSED) {                     //对焦事件
    }
    else if (code == LV_EVENT_CLICKED) {                //点击事件
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
        if (obj == data->Btn) {
            /* [我们] B6-C 按当前格分派 */
            ThePage_ID_t page_idx = Get_CurPage_Idx();
            switch (page_idx) {
            case ThePage: Child_ThePage_Load(ChiThePage_StaSel);  break;
            case MaiPage: Child_MaiPage_Load(ChiMaiPage_StaSel);  break;
            case DysPage: Child_DysPage_Load(ChiDysPage_StaSel);  break;
            case KgePage: Child_KgePage_Load(ChiKgePage_StaSel);  break;
            default: break;
            }
        }
    }
}


/**
 * @brief 下一节倒计时定时器跳转函数
 *        [我们] ThePage-only（PC switch Mai/Dys/Kge 未移植，各后续会话）
 * @param t
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
        /* [我们] B6-C 按当前格分派（PC 原 switch 前 3 处无 Dys 分支） */
        ThePage_ID_t page_idx = Get_CurPage_Idx();
        switch (page_idx) {
        case ThePage: Child_ThePage_Load(ChiThePage_Chart1);  break;
        case MaiPage: Child_MaiPage_Load(ChiMaiPage_Chart1);  break;
        case KgePage: Child_KgePage_Load(ChiKgePage_Chart1);  break;
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
 * @author zqt
 * @brief 阶段三曲线图1定时器添加数据回调函数（y=300 弹提示窗，y=400 关，500 后进曲线图2）
 * @param t
 */
void Chart_Stage3_Add_data(lv_timer_t* t)
{
    Chart_Data_t* data = (Chart_Data_t*)lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(data->Chart, NULL);

    uint16_t p = lv_chart_get_point_count(data->Chart);
    uint16_t s = lv_chart_get_x_start_point(data->Chart, ser);
    int32_t* a = lv_chart_get_y_array(data->Chart, ser);

    data->tick++;
    if(data->status == TIMER_RUNNING) {
        /* 实时力度包络 (raw16 LSB → µV) */
        int32_t env_uv = (int32_t)(emg_force_get_env() * EMG_DSP_RAW16_LSB_UV);
        int32_t smoothed = smooth_value(env_uv);
        therapy_emg_feedback(smoothed);
        if(smoothed >= 100) smoothed = 100;
        if(smoothed <= 0) smoothed = 0;
        lv_label_set_text_fmt(data->Label_InsEMG, "%dμV", (int)smoothed);  //瞬时肌电位 需根据实际值显示
        lv_label_set_text_fmt(data->Label_TimeLeft, "%02d分%02d秒",
                                        (int)(data->Value_TimeLeft / 60000),
                                        (int)((data->Value_TimeLeft % 60000) / 1000)); 
        lv_chart_set_next_value(data->Chart, ser, smoothed);               //需根据实际值填入数据
        int32_t chart_point_count = lv_chart_get_point_count(data->Chart);
        if(data->tick >= chart_point_count) {
            data->tick = 0;
            cur_repeat_cnt++;
            if (cur_repeat_cnt < chart_repeat) {
                lv_chart_set_all_value(data->Chart, ser, LV_CHART_POINT_NONE);  /* 清空旧数据 */
                lv_chart_set_x_start_point(data->Chart, ser, 0);          /* 从最左开始画 */  
            }
            ESP_LOGI(TAG, "当前次数：%d,重复次数：%d,chart point=%d\r\n", cur_repeat_cnt,chart_repeat,chart_point_count);
        }
        if (smoothed < 0) {
            Warn_Window(&data->Window);             //定时器模拟触发提示窗口
        } else if (smoothed > 0) {
            if (data->Window.Msgbox) {
                lv_msgbox_close_async(data->Window.Msgbox);         //主动关闭窗口
            }
        }
    } else if(data->status == TIMER_END) {
        // g_guide_cont = NULL;
        g_guide_cont_line = NULL;
        cur_repeat_cnt = 0;
        // data->Chart = NULL;
        data->tick = 0;
        emg_force_stop_acq();
        lv_timer_del(data->Timer_Treat);
        data->Timer_Treat = NULL;                               
        audio_play_request(VOICE_ID_NEXT_CN,1500);              //语音播放：下一步
        /* [我们] B6-C 按当前格分派 */
        ThePage_ID_t page_idx = Get_CurPage_Idx();
        switch (page_idx) {
        case ThePage: Child_ThePage_Load(ChiThePage_Chart2);  break;
        case MaiPage: Child_MaiPage_Load(ChiMaiPage_Chart2);  break;
        case KgePage: Child_KgePage_Load(ChiKgePage_Chart2);  break;
        default: break;
        }
        /* 清空滑动均值窗口，防跨阶段混入旧值 */
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
 * @author zqt
 * @brief 阶段三曲线图2定时器添加数据回调函数（500 后进曲线图3）
 * @param t
 */
void Chart2_Stage3_Add_data(lv_timer_t* t)
{
    static int32_t y = 0;
    Chart_Data_t* data = (Chart_Data_t*)lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(data->Chart, NULL);

    uint16_t p = lv_chart_get_point_count(data->Chart);
    uint16_t s = lv_chart_get_x_start_point(data->Chart, ser);
    int32_t* a = lv_chart_get_y_array(data->Chart, ser);


    if (y >= 500) {
        y = 0;
        lv_timer_del(data->Timer_Treat);
        data->Timer_Treat = NULL;                               //定时治疗结束 进入下一节治疗
        /* [我们] B6-C 按当前格分派 */
        ThePage_ID_t page_idx = Get_CurPage_Idx();
        switch (page_idx) {
        case ThePage: Child_ThePage_Load(ChiThePage_Chart3);  break;
        case MaiPage: Child_MaiPage_Load(ChiMaiPage_Chart3);  break;
        case KgePage: Child_KgePage_Load(ChiKgePage_Chart3);  break;
        default: break;
        }
        return;
    }
    lv_label_set_text_fmt(data->Label_InsEMG, "%.1fμV", 60.8); //需根据实际值显示
    lv_chart_set_next_value(data->Chart, ser, 50);              //需根据实际值填入数据
    y++;
    
    a[(s + 1) % p] = LV_CHART_POINT_NONE;
    a[(s + 2) % p] = LV_CHART_POINT_NONE;
    a[(s + 3) % p] = LV_CHART_POINT_NONE;
    a[(s + 4) % p] = LV_CHART_POINT_NONE;
    a[(s + 5) % p] = LV_CHART_POINT_NONE;
}


/**
 * @brief 获取疗程 在用
 * @param btn
 * @return
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
 * @author zqt
 * @brief 将全部疗程照片设未选中 在用
 * @param e
 */
static void Set_StageBtn_UnSel(Select_Data_t* data)
{
    for (uint8_t i = 0; i < data->Btn_MaxSum; ++i) {

        lv_obj_remove_style(data->sta[i].obj, &style_gradient, LV_PART_MAIN);
        lv_obj_set_style_image_recolor(data->sta[i].img, lv_color_hex(FONT_GRAY_COLOR), 0); //照片重新重色
    }
}
