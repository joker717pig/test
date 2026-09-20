#include "menu_ui_event_cb.h"
#include "menu_ui.h"
#include "basic.h"
#include "app_keypad.h"   /* [我们] Menu_LeaveGroup 用 */
#include "app_therapy.h"  /* [我们] 阶段C：治疗控制（强度实时下发 / 确认返回停引擎） */
#include "esp_log.h"      /* [我们] 日志 */
#include "voice.h"
#include "emg_force.h"     /* 阶段 3.3: 实时力度包络 */
#include "emg_dsp.h"       /* EMG_DSP_RAW16_LSB_UV 换算 */

static const char *TAG = "menu_evt";
#define FILTER_WIN 7  // 滑动窗口大小（3~7 效果较好）
extern lv_my_ui_page_date_t* g_page_list[APP_SUM];

static int32_t filter_buf[FILTER_WIN] = { 0 };
static uint8_t filter_idx = 0;
/* [我们] 隐藏全部选中效果图片
 * lv_obj_is_valid 防悬垂：Page_Clean 删除时若 group refocus 触发 FOCUSED，seleck 图可能已删除 */
static void Menu_Set_UnSel(void)
{
    if (lv_obj_is_valid(Menu_Widget.Ass_seleckImg))
        lv_obj_add_flag(Menu_Widget.Ass_seleckImg, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(Menu_Widget.The_seleckImg))
        lv_obj_add_flag(Menu_Widget.The_seleckImg, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(Menu_Widget.PostRec_seleckImg))
        lv_obj_add_flag(Menu_Widget.PostRec_seleckImg, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(Menu_Widget.Set_seleckImg))
        lv_obj_add_flag(Menu_Widget.Set_seleckImg, LV_OBJ_FLAG_HIDDEN);
}

/* [我们] 切页前把 4 个菜单按钮移出 keypad group
 * 根因（2026-08-11 task_wdt 死锁，0H 坑 6 补充）：Page_Clean 删除“焦点按钮”时
 * lv_group_refocus 会向剩余按钮发 LV_EVENT_FOCUSED → 本回调操作已被删除/悬垂的
 * seleck 图 → lv_obj_invalidate 遍历损坏对象树 → 死循环 → task_wdt。
 * 先移除（对象仍有效）再切页，删除阶段不再触发 FOCUSED。 */
static void Menu_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g == NULL) return;
    /* [我们] lv_obj_is_valid 防悬垂：返回菜单后旧按钮指针已失效，与坑 6.8 同族 */
    if (lv_obj_is_valid(Menu_Widget.PelFlo_Ass_Btn)) lv_group_remove_obj(Menu_Widget.PelFlo_Ass_Btn);
    if (lv_obj_is_valid(Menu_Widget.PelFlo_The_Btn)) lv_group_remove_obj(Menu_Widget.PelFlo_The_Btn);
    if (lv_obj_is_valid(Menu_Widget.PostRec_Btn))    lv_group_remove_obj(Menu_Widget.PostRec_Btn);
    if (lv_obj_is_valid(Menu_Widget.Set_Btn))        lv_group_remove_obj(Menu_Widget.Set_Btn);
}

/**
 * @author zqt
 * @brief 菜单切换按钮事件回调函数
 *        [我们] 新增 LV_EVENT_FOCUSED（选中高亮）+ LV_EVENT_KEY（2D 导航）
 */
void Menu_button_event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Menu_Widget_t* user_data = (Menu_Widget_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {                       /* [我们] */
        Menu_Set_UnSel();
        if (obj == user_data->PelFlo_Ass_Btn && lv_obj_is_valid(user_data->Ass_seleckImg))
            lv_obj_remove_flag(user_data->Ass_seleckImg, LV_OBJ_FLAG_HIDDEN);
        else if (obj == user_data->PelFlo_The_Btn && lv_obj_is_valid(user_data->The_seleckImg))
            lv_obj_remove_flag(user_data->The_seleckImg, LV_OBJ_FLAG_HIDDEN);
        else if (obj == user_data->PostRec_Btn && lv_obj_is_valid(user_data->PostRec_seleckImg))
            lv_obj_remove_flag(user_data->PostRec_seleckImg, LV_OBJ_FLAG_HIDDEN);
        else if (obj == user_data->Set_Btn && lv_obj_is_valid(user_data->Set_seleckImg))
            lv_obj_remove_flag(user_data->Set_seleckImg, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_KEY) {                      /* [我们] 2D 导航 */
        uint32_t key = lv_event_get_key(e);
        lv_obj_t* target = NULL;
        if (obj == user_data->PelFlo_Ass_Btn) {           /* 评估 左上 */
            if (key == LV_KEY_RIGHT) target = user_data->PelFlo_The_Btn;
            else if (key == LV_KEY_DOWN) target = user_data->PostRec_Btn;
        }
        else if (obj == user_data->PelFlo_The_Btn) {      /* 治疗 右上 */
            if (key == LV_KEY_LEFT) target = user_data->PelFlo_Ass_Btn;
            else if (key == LV_KEY_DOWN) target = user_data->Set_Btn;
        }
        else if (obj == user_data->PostRec_Btn) {         /* 产后 左下 */
            if (key == LV_KEY_RIGHT) target = user_data->Set_Btn;
            else if (key == LV_KEY_UP) target = user_data->PelFlo_Ass_Btn;
        }
        else if (obj == user_data->Set_Btn) {             /* 设置 右下 */
            if (key == LV_KEY_LEFT) target = user_data->PostRec_Btn;
            else if (key == LV_KEY_UP) target = user_data->PelFlo_The_Btn;
        }
        if (target) {
            ESP_LOGI(TAG, "nav: LV_KEY=%lu", (unsigned long)key);
            lv_group_focus_obj(target);
        }
    }
    else if (code == LV_EVENT_CLICKED) {                  /* 同事原逻辑 */
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
        ESP_LOGI(TAG, "Menu button clicked");
        Menu_LeaveGroup();       /* [我们] 先移出 group，避免 Page_Clean 时 refocus 触发 FOCUSED 悬垂 */

        if (obj == user_data->PelFlo_Ass_Btn)          //盆底评估
        {
            Menu_Page_Load(PAGE_PELFLO_ASS);
        }
        else if (obj == user_data->PelFlo_The_Btn)     //盆底治疗
        {
            Menu_Page_Load(PAGE_PELFLO_THE);
        }
        else if (obj == user_data->PostRec_Btn)        //产后修复
        {
            Menu_Page_Load(PAGE_POSREH);
        }
        else if (obj == user_data->Set_Btn)            //设置
        {
            Menu_Page_Load(PAGE_SETTING);
        }
    }
}

/* ============================================================================
 * [我们] B4 PosReh 共享控件回调（同事 MenuPage 原装，移植自 EDA_EMG_LVGL_PC\MenuPage\menu_ui_event_cb.c）
 * 配套控件函数在 menu_ui.c；PosReh_Page_ui_event.c 另注册各自的 CLICKED 导航回调
 * ==========================================================================*/

static void Treat_Set_UnSel(Treat_Timer_Ctx_t* ctx);

/**
 * @brief 返回弹窗按钮事件回调（对齐 PC 新版 Back_Btn_Event_cb）
 *        [我们] ESP32 适配：确认 = 移出 group + 关弹窗 + Goto_MenuPage
 *        （定时器已在调用方 Back 分支提前删除，故此处无需再删）；
 *        坑 B3-3：删对象路径不用 lv_event_stop_processing
 */
void Back_Btn_Event_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    // Window_Data_t* ctx = (Window_Data_t*)lv_event_get_user_data(e);     // 用户传进来的数据
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);
    if (code == LV_EVENT_CLICKED) {
        if (btn == ctx->Window.Yes_Btn) {
            /* [我们] 先把是/否移出 group，再回主菜单（Menu_ui 首对象自动聚焦） */
            lv_group_t *g = app_keypad_get_group();
            if (g) {
                if (lv_obj_is_valid(ctx->Window.Yes_Btn)) lv_group_remove_obj(ctx->Window.Yes_Btn);
                if (lv_obj_is_valid(ctx->Window.No_Btn))  lv_group_remove_obj(ctx->Window.No_Btn);
            }
            lv_msgbox_close_async(ctx->Window.Msgbox);
            if(ctx->timer_Treat) {
                lv_timer_del(ctx->timer_Treat);
                ctx->timer_Treat = NULL;
            }
            therapy_stop();   /* [我们] 阶段C：确认返回 -> 停治疗引擎（停波 + 会话复位；未运行时无操作） */
            audio_play_request(VOICE_ID_FINISH_CN,1500);              //语音播放：结束
            Goto_MenuPage();
        }
        else if (btn == ctx->Window.No_Btn) { 
            lv_msgbox_close_async(ctx->Window.Msgbox);
            therapy_resume(); 
        }
    }
}

/**
 * @brief 阶段疗程说明页事件回调（FOCUSED 样式占位，CLICKED 由 PosReh_TreIns_Event_cb 处理）
 */
void TreIns_Widget_event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    TreIns_Data_t* data = (TreIns_Data_t*)lv_event_get_user_data(e);

    (void)obj; (void)data;
    if (code == LV_EVENT_FOCUSED) {
        /* 同事原实现为空分支；导航/点击由 PosReh 页事件处理 */
    }
}

/**
 * @brief 电刺激治疗页中控件设未选中样式
 */
static void Treat_Set_UnSel(Treat_Timer_Ctx_t* ctx)
{
    /* [我们] lv_obj_is_valid 防悬垂（坑 B2-4）：切页 refocus 时对象可能已删 */
    if (lv_obj_is_valid(ctx->treat_data.Start_src)) lv_obj_set_style_image_recolor(ctx->treat_data.Start_src, lv_color_hex(FONT_GRAY_COLOR), 0);
    if (lv_obj_is_valid(ctx->treat_data.Pause_src)) lv_obj_set_style_image_recolor(ctx->treat_data.Pause_src, lv_color_hex(FONT_GRAY_COLOR), 0);
    if (lv_obj_is_valid(ctx->treat_data.Back_src))  lv_obj_set_style_image_recolor(ctx->treat_data.Back_src, lv_color_hex(FONT_GRAY_COLOR), 0);
    if (lv_obj_is_valid(ctx->treat_data.Plu_src))   lv_obj_set_style_image_recolor(ctx->treat_data.Plu_src, lv_color_hex(FONT_GRAY_COLOR), 0);
    if (lv_obj_is_valid(ctx->treat_data.Min_src))   lv_obj_set_style_image_recolor(ctx->treat_data.Min_src, lv_color_hex(FONT_GRAY_COLOR), 0);

    if (lv_obj_is_valid(ctx->treat_data.Label_Back))  lv_obj_set_style_text_color(ctx->treat_data.Label_Back, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    if (lv_obj_is_valid(ctx->treat_data.Label_Pause)) lv_obj_set_style_text_color(ctx->treat_data.Label_Pause, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    if (lv_obj_is_valid(ctx->treat_data.Label_Start)) lv_obj_set_style_text_color(ctx->treat_data.Label_Start, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    if (lv_obj_is_valid(ctx->treat_data.Back_Btn))  lv_obj_set_style_bg_color(ctx->treat_data.Back_Btn, lv_color_hex(0xf6f6f6), LV_STATE_DEFAULT);
    if (lv_obj_is_valid(ctx->treat_data.Start_Btn)) lv_obj_set_style_bg_color(ctx->treat_data.Start_Btn, lv_color_hex(0xf6f6f6), LV_STATE_DEFAULT);
    if (lv_obj_is_valid(ctx->treat_data.Pause_Btn)) lv_obj_set_style_bg_color(ctx->treat_data.Pause_Btn, lv_color_hex(0xf6f6f6), LV_STATE_DEFAULT);
}

/**
 * @brief 电刺激治疗页事件回调（FOCUSED 样式 + 强度 +/- 点击）
 */
void Treat_Widget_Event_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target_obj(e);
    lv_event_code_t code = lv_event_get_code(e);
    Treat_Timer_Ctx_t* ctx = (Treat_Timer_Ctx_t*)lv_event_get_user_data(e);

    int8_t value = 0;
    if (code == LV_EVENT_FOCUSED) {
        Treat_Set_UnSel(ctx);                       //将全部控件设为未选中样式
        if (btn == ctx->treat_data.Start_Btn) {     //开始按钮
            lv_obj_set_style_image_recolor(ctx->treat_data.Start_src, lv_color_hex(FONT_WHITE_COLOR), 0); //照片重新重色
            lv_obj_set_style_text_color(ctx->treat_data.Label_Start, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(ctx->treat_data.Start_Btn, lv_color_hex(ctx->treat_data.Color), LV_STATE_DEFAULT);
        }
        else if (btn == ctx->treat_data.Pause_Btn) { //暂停按钮
            lv_obj_set_style_image_recolor(ctx->treat_data.Pause_src, lv_color_hex(FONT_WHITE_COLOR), 0); //照片重新重色
            lv_obj_set_style_text_color(ctx->treat_data.Label_Pause, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(ctx->treat_data.Pause_Btn, lv_color_hex(ctx->treat_data.Color), LV_STATE_DEFAULT);
        }
        else if (btn == ctx->treat_data.Back_Btn) { //返回按钮
            lv_obj_set_style_image_recolor(ctx->treat_data.Back_src, lv_color_hex(FONT_WHITE_COLOR), 0); //照片重新重色
            lv_obj_set_style_text_color(ctx->treat_data.Label_Back, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(ctx->treat_data.Back_Btn, lv_color_hex(ctx->treat_data.Color), LV_STATE_DEFAULT);
        }
        else if (btn == ctx->treat_data.Plu_Btn) {  //强度加按钮
            lv_obj_set_style_image_recolor(ctx->treat_data.Plu_src, lv_color_hex(ctx->treat_data.Color), 0); //照片重新重色
        }
        else if (btn == ctx->treat_data.Min_Btn) {  //强度减按钮
            lv_obj_set_style_image_recolor(ctx->treat_data.Min_src, lv_color_hex(ctx->treat_data.Color), 0); //照片重新重色
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        value = ctx->treat_data.Intens_Value;
        if (btn == ctx->treat_data.Plu_Btn) {
            value++;
            if (value >= 90)value = 90;
            ctx->treat_data.Intens_Value = value;
        }
        else if (btn == ctx->treat_data.Min_Btn) {
            value--;
            if (value <= 0)value = 0;
            ctx->treat_data.Intens_Value = value;
        }
        lv_label_set_text_fmt(ctx->treat_data.Label_Intens, "%d", value);   //更新显示强度值
        /* [我们] 阶段C：治疗中实时调当前相位强度（引擎按当前相位写对应 mA；未运行时无操作） */
        if (therapy_active()) therapy_set_intensity(value);
    }
}

/**
 * @brief 将加/减按钮图片设为未选中（参数设置页）
 */
static void SetPar_UnSel_Style(Param_Data_t* data)
{
    /* [我们] lv_obj_is_valid 防悬垂（坑 B2-4） */
    if (lv_obj_is_valid(data->Plu_src))  lv_obj_set_style_image_recolor(data->Plu_src, lv_color_hex(FONT_GRAY_COLOR), 0);
    if (lv_obj_is_valid(data->Min_src))  lv_obj_set_style_image_recolor(data->Min_src, lv_color_hex(FONT_GRAY_COLOR), 0);
    if (lv_obj_is_valid(data->Slider))   lv_obj_set_style_bg_color(data->Slider, lv_color_hex(FONT_GRAY_COLOR), LV_PART_KNOB);
    if (lv_obj_is_valid(data->Set_Btn))  lv_obj_set_style_bg_color(data->Set_Btn, lv_color_hex(0xffffff), LV_PART_MAIN);
    if (lv_obj_is_valid(data->Label_Set)) lv_obj_set_style_text_color(data->Label_Set, lv_color_hex(data->Slder_Color), LV_PART_MAIN);
}

/**
 * @brief 参数设置页事件回调（FOCUSED 样式 + 强度 +/- 点击 + 滑条 VALUE_CHANGED）
 */
void Child_Slider_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Param_Data_t* user_data = (Param_Data_t*)lv_event_get_user_data(e);
    int8_t value = 0;

    if (code == LV_EVENT_FOCUSED) {                         //对焦事件
        SetPar_UnSel_Style(user_data);
        if (obj == user_data->Min_Btn) {                    //减按钮
            lv_obj_set_style_image_recolor(user_data->Min_src, lv_color_hex(user_data->Slder_Color), 0); //照片重新重色
        }
        else if (obj == user_data->Plu_Btn) {               //加按钮
            lv_obj_set_style_image_recolor(user_data->Plu_src, lv_color_hex(user_data->Slder_Color), 0); //照片重新重色
        }
        else if (obj == user_data->Slider) {                //滑动条
            lv_obj_set_style_bg_color(user_data->Slider, lv_color_hex(user_data->Slder_Color), LV_PART_KNOB);
        }
        else if (obj == user_data->Set_Btn) {               //设置按钮
            lv_obj_set_style_bg_color(user_data->Set_Btn, lv_color_hex(user_data->Slder_Color), LV_PART_MAIN);
            lv_obj_set_style_text_color(user_data->Label_Set, lv_color_hex(0xffffff), LV_PART_MAIN);
        }
    }
    else if (code == LV_EVENT_CLICKED) {                 //加/减按钮点击事件
        if (user_data->Value_Stage == 1) {              //治疗阶段1
            value = user_data->Value_Intens1;
        }
        else if (user_data->Value_Stage == 2) {         //治疗阶段2
            value = user_data->Value_Intens2;
        }

        if (obj == user_data->Min_Btn) {                //减按钮
            value--;
            if (value <= 0) value = 0;
        }
        else if (obj == user_data->Plu_Btn) {           //加按钮
            value++;
            if (value >= 90) value = 90;
        }
        if (user_data->Value_Stage == 1) {              //治疗阶段1
            user_data->Value_Intens1 = value;
            /* [我们] -Werror=format：int32_t=long，%d → (int) */
            ESP_LOGI(TAG,"阶段1强度值:%d", (int)user_data->Value_Intens1);
        }
        else if (user_data->Value_Stage == 2) {         //治疗阶段2
            user_data->Value_Intens2 = value;
            ESP_LOGI(TAG,"阶段2强度值:%d", (int)user_data->Value_Intens2);
        }

        lv_label_set_text_fmt(user_data->Label_Intens, "%d", value);
        lv_slider_set_value(user_data->Slider, value, LV_ANIM_OFF);
        
        therapy_send_intensity(value,user_data->Value_Freq,user_data->Value_Pulse);
    }
    else if (code == LV_EVENT_VALUE_CHANGED) {             //滑动条值改变事件
        if (obj == user_data->Slider) {
            value = lv_slider_get_value(obj);
            if (user_data->Value_Stage == 1) {              //治疗阶段1
                user_data->Value_Intens1 = value;
                ESP_LOGI(TAG,"设置滑动条 阶段1强度值:%d", (int)user_data->Value_Intens1);
            }
            else if (user_data->Value_Stage == 2) {        //治疗阶段2
                user_data->Value_Intens2 = value;
                ESP_LOGI(TAG,"设置滑动条 阶段2强度值:%d", (int)user_data->Value_Intens2);
            }
            lv_label_set_text_fmt(user_data->Label_Intens, "%d", value);
            therapy_send_intensity(value,user_data->Value_Freq,user_data->Value_Pulse);
        }
    }
}

/**
 * @brief 阶段治疗页的 模糊容器位置反向补偿 作用：页面滑动时 位置保持不动
 */
void ThePage_Obj_scroll_cb(lv_event_t* e)
{
    lv_obj_t* page = lv_event_get_target(e);
    lv_obj_t* obj = lv_event_get_user_data(e);

    lv_coord_t sy = lv_obj_get_scroll_y(page);
    uint16_t pos = 0;
    pos = ThePage_Obj_origin_y + sy;
    if (pos >= 445) pos = 445;
    lv_obj_set_pos(obj, -8, pos);
}

/**
 * @brief 历史查看按钮位置反向补偿 作用：页面滑动时 按钮位置保持不动
 */
void ThePage_HisBtn_scroll_cb(lv_event_t* e)
{
    lv_obj_t* page = lv_event_get_target(e);
    lv_obj_t* btn = lv_event_get_user_data(e);

    lv_coord_t sy = lv_obj_get_scroll_y(page);
    uint16_t pos = 0;
    pos = ThePage_HisBtn_origin_y + sy;
    if (pos >= 420) pos = 420;
    lv_obj_set_pos(btn, 195, pos);
}

/**
 * @brief 开始评估按钮位置反向补偿 作用：页面滑动时 按钮位置保持不动
 */
void ThePage_StrBtn_scroll_cb(lv_event_t* e)
{
    lv_obj_t* page = lv_event_get_target(e);
    lv_obj_t* btn = lv_event_get_user_data(e);

    lv_coord_t sy = lv_obj_get_scroll_y(page);
    uint16_t pos = 0;
    pos = ThePage_StrBtn_origin_y + sy;
    if (pos >= 420) pos = 420;
    lv_obj_set_pos(btn, 324, pos);
}

/* ============================================================================
 * [我们] B5-B 治疗页图表事件（同事原装，移植自 EDA_EMG_LVGL_PC\PelFlo_ThePage\PelFlo_TheChild_event.c）
 *  - Myadd_faded_area / Chart_Draw_event_cb / Chart_Btn_Event_cb / Timer_BackToMenu_Event_cb
 *  - Chart3_Stage3_Add_data / Chart_Stage2_4_Add_data（硬编码 50/60.8 演示值）
 *  - ThePage2_Obj_scroll_cb（Select_Widget2 滚动补偿）
 *  ⚠️ 页面跳转相关（Chart_Stage3/Chart2_Stage3/timer_next/治疗双 timer）留 B5-C
 * ==========================================================================*/

/**
 * @brief 选择疗程页的 模糊容器位置反向补偿 作用：页面滑动时 位置保持不动
 */
void ThePage2_Obj_scroll_cb(lv_event_t* e)
{
    lv_obj_t* page = lv_event_get_target(e);
    lv_obj_t* obj = lv_event_get_user_data(e);

    lv_coord_t sy = lv_obj_get_scroll_y(page);
    uint16_t pos = 0;
    pos = ThePage_Obj_origin_y2 + sy;
    if (pos >= 300) {
        pos = 300;
    }
    lv_obj_set_pos(obj, -8, pos);
}

/**
 * @brief 曲线渐变填充（评估页有 static 副本，本文件 static 不冲突；
 *        ⚠️ LVGL9.5：lv_draw_triangle_dsc_t 用 grad，勿用 PC 的 bg_grad）
 */
static void Myadd_faded_area(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);

    lv_draw_task_t* draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t* base_dsc = lv_draw_task_get_draw_dsc(draw_task);

    const lv_chart_series_t* ser = lv_chart_get_series_next(obj, NULL);
    lv_color_t ser_color = lv_chart_get_series_color(obj, ser);

    lv_draw_line_dsc_t* line_dsc = lv_draw_task_get_draw_dsc(draw_task);

    /* 1. 获取关键点坐标 */
    float p1x = line_dsc->p1.x;
    float p1y = line_dsc->p1.y;
    float p2x = line_dsc->p2.x - 0.5f;
    float p2y = line_dsc->p2.y;

    /* Y=0 (X轴) 在屏幕上的像素位置 */
    float zero_y = coords.y2;

    /* 2. 准备绘图描述符 */
    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 0; /* 矩形无圆角 */
    rect_dsc.border_width = 0;
    rect_dsc.bg_grad.dir = LV_GRAD_DIR_VER;

    lv_draw_triangle_dsc_t tri_dsc;
    lv_draw_triangle_dsc_init(&tri_dsc);
    tri_dsc.grad.dir = LV_GRAD_DIR_VER;   /* [我们] LVGL9.5: bg_grad -> grad */

    /* 3. 计算渐变参数 */
    int32_t full_h = lv_obj_get_height(obj);

    /* 矩形部分：顶部透明度取决于线段的最低点，底部完全透明 */
    float rect_top_ratio = (LV_MIN(p1y, p2y) - coords.y1) / (float)full_h;
    uint8_t rect_top_opa = (uint8_t)((1.0f - rect_top_ratio) * 100);
    uint8_t rect_btm_opa = 0; /* X轴处完全透明 */

    /* 三角形部分：顶部透明度取决于线段的最高点，底部与矩形顶部对齐 */
    float tri_top_ratio = (LV_MAX(p1y, p2y) - coords.y1) / (float)full_h;
    uint8_t tri_top_opa = (uint8_t)((1.0f - tri_top_ratio) * 100);

    /* 4. 绘制矩形（覆盖折线正下方的垂直区域） */
    /* 矩形的左右边界就是线段端点的X坐标，上下边界是从X轴到线段最低点 */
    lv_area_t rect_area;
    rect_area.x1 = (int32_t)((p1x < p2x) ? p1x : p2x);
    rect_area.x2 = (int32_t)((p1x > p2x) ? p1x : p2x);
    rect_area.y1 = (int32_t)LV_MAX(p1y, p2y); /* 上边界：线段较低的一端 */
    rect_area.y2 = (int32_t)zero_y;           /* 下边界：X轴 */

    /* 只有当线段不是水平线时才画矩形，否则矩形高度为0会导致无效果 */
    if (rect_area.y1 < rect_area.y2) {
        rect_dsc.bg_grad.stops[0].color = ser_color;
        rect_dsc.bg_grad.stops[0].opa = rect_top_opa;
        rect_dsc.bg_grad.stops[0].frac = 0;
        rect_dsc.bg_grad.stops[1].color = ser_color;
        rect_dsc.bg_grad.stops[1].opa = rect_btm_opa;
        rect_dsc.bg_grad.stops[1].frac = 255;

        lv_draw_rect(base_dsc->layer, &rect_dsc, &rect_area);
    }

    /* 5. 绘制三角形（填补矩形上方与折线之间的空隙） */
    /* 这是一个标准的直角三角形，直角在下方 */
    tri_dsc.p[0].x = p1x; tri_dsc.p[0].y = p1y;
    tri_dsc.p[1].x = p2x + 0.5f; tri_dsc.p[1].y = p2y;
    /* 找到折线两个点中较低的那个（Y值较大的） */
    float lower_y = LV_MAX(p1y, p2y);
    /* 这个数字你可以调（2, 3, 5...），直到你看到填充出现在折线下方 */
    lower_y += 1.0f;

    if (p2y > p1y) {
        /* 下降：直角顶点在右下角（X取p2x，Y取我们修正过的lower_y） */
        tri_dsc.p[2].x = p1x;
        tri_dsc.p[2].y = lower_y;
    }
    else {
        /* 上升：直角顶点在左下角（X取p1x，Y取我们修正过的lower_y） */
        tri_dsc.p[2].x = p2x + 0.5f;
        tri_dsc.p[2].y = lower_y;
    }
    tri_dsc.grad.stops[0].color = ser_color;
    tri_dsc.grad.stops[0].opa = tri_top_opa;
    tri_dsc.grad.stops[0].frac = 0;
    tri_dsc.grad.stops[1].color = ser_color;
    /* 三角形的底边透明度应该与矩形的顶边透明度一致，才能无缝衔接 */
    tri_dsc.grad.stops[1].opa = rect_top_opa;
    tri_dsc.grad.stops[1].frac = 255;

    /* 只有当线段不是水平线时才画三角形 */
    if (p1y != p2y) {
        lv_draw_triangle(base_dsc->layer, &tri_dsc);
    }
}

/**
 * @brief 曲线图绘图事件（LV_EVENT_DRAW_TASK_ADDED → 渐变填充）
 */
void Chart_Draw_event_cb(lv_event_t* e)
{
    lv_draw_task_t* draw_task = lv_event_get_draw_task(e);
    lv_draw_dsc_base_t* base_dsc = lv_draw_task_get_draw_dsc(draw_task);
    lv_obj_t* obj = lv_event_get_user_data(e);
    (void)obj;

    /*是否为画线任务*/
    if (base_dsc->part == LV_PART_ITEMS && lv_draw_task_get_type(draw_task) == LV_DRAW_TASK_TYPE_LINE) {
        Myadd_faded_area(e);
    }
}

/**
 * @brief 返回菜单定时器跳转函数（曲线图完成后延时返回）
 */
static void Timer_BackToMenu_Event_cb(lv_timer_t* t)
{
    Chart_Data_t* data = lv_timer_get_user_data(t);

    static uint8_t count = 0;
    count++;
    if (count >= 30) {
        count = 0;
        lv_timer_del(data->Timer_Back);
        data->Timer_Back = NULL;
        Goto_MenuPage();
        return;
    }
}

/**
 * @brief 曲线图页弹窗按钮事件回调（是 → 延时返回菜单，否 → 关窗）
 */
void Chart_Btn_Event_cb(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Chart_Data_t* data = (Chart_Data_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_CLICKED) {
        if (data->Window.Yes_Btn == btn) {
            /* [我们] B5-C 先移出 group 再关窗+延时回菜单（坑 6.8 同族） */
            lv_group_t *g = app_keypad_get_group();
            if (g) {
                if (lv_obj_is_valid(data->Window.Yes_Btn)) lv_group_remove_obj(data->Window.Yes_Btn);
                if (lv_obj_is_valid(data->Window.No_Btn))  lv_group_remove_obj(data->Window.No_Btn);
            }
            lv_msgbox_close_async(data->Window.Msgbox);
            lv_label_set_text_fmt(data->Label_Finish, "已完成，正在返回...");
            // 创建返回菜单倒计时定时器
            if (data->Timer_Back == NULL) {
                data->Timer_Back = lv_timer_create(Timer_BackToMenu_Event_cb, 20, data);
            }
        }
        else if (data->Window.No_Btn == btn) {
            lv_msgbox_close_async(data->Window.Msgbox);
        }
    }
}

/* [我们] B5-C 曲线图确认框键盘：ESC=否（关闭弹窗，曲线图 cont 仍在 group → 焦点自然回落到 cont） */
static void Chart_MsgBox_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        Chart_Data_t* data = (Chart_Data_t*)lv_event_get_user_data(e);
        if (data->Window.Msgbox && lv_obj_is_valid(data->Window.Msgbox))
            lv_msgbox_close_async(data->Window.Msgbox);
    }
}

/* [我们] B5-C 曲线图确认框键盘化：是/否 进 group（默认聚焦“是”）+ ESC=否 */
static void Chart_ShowConfirm_Key(Chart_Data_t* data)
{
    lv_group_t *g = app_keypad_get_group();
    if (g && lv_obj_is_valid(data->Window.Yes_Btn)) {
        lv_group_add_obj(g, data->Window.Yes_Btn);
        lv_group_add_obj(g, data->Window.No_Btn);
        lv_group_focus_obj(data->Window.Yes_Btn);
    }
    lv_obj_add_event_cb(data->Window.Yes_Btn, Chart_MsgBox_Key_cb, LV_EVENT_KEY, data);
    lv_obj_add_event_cb(data->Window.No_Btn,  Chart_MsgBox_Key_cb, LV_EVENT_KEY, data);
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
 * @brief 阶段三曲线图3定时器添加数据回调（硬编码 50/60.8 演示值）
 */
void Chart3_Stage3_Add_data(lv_timer_t* t)
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
    } else if(data->status == TIMER_END) {
        g_guide_cont = NULL;
        // g_guide_cont_line = NULL;
        cur_repeat_cnt = 0;
        // data->Chart = NULL;
        data->tick = 0;
        emg_force_stop_acq();
        audio_play_request(VOICE_ID_DONE_CN,1500);              //语音播放：完成
        lv_timer_del(data->Timer_Treat);
        data->Timer_Treat = NULL;                               //定时治疗结束 弹窗询问
        Back_Window(&data->Window, "是否确认返回？");
        lv_obj_add_event_cb(data->Window.No_Btn, Chart_Btn_Event_cb, LV_EVENT_CLICKED, data);
        lv_obj_add_event_cb(data->Window.Yes_Btn, Chart_Btn_Event_cb, LV_EVENT_CLICKED, data);
        Chart_ShowConfirm_Key(data);                            /* [我们] B5-C 键盘化 */
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
 * @brief 阶段二/四曲线图定时器添加数据回调（硬编码 50/60.8 演示值）
 */
void Chart_Stage2_4_Add_data(lv_timer_t* t)
{
    Chart_Data_t* data = (Chart_Data_t*)lv_timer_get_user_data(t);
    lv_chart_series_t* ser = lv_chart_get_series_next(data->Chart, NULL);

    uint16_t p = lv_chart_get_point_count(data->Chart);
    uint16_t s = lv_chart_get_x_start_point(data->Chart, ser);
    int32_t* a = lv_chart_get_y_array(data->Chart, ser);
    if (ser == NULL) return;

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
        

    } else if(data->status == TIMER_END) {
        // g_guide_cont = NULL;
        g_guide_cont_line = NULL;
        cur_repeat_cnt = 0;
        // data->Chart = NULL;
        data->tick = 0;
        emg_force_stop_acq();
        lv_timer_del(data->Timer_Treat);
        data->Timer_Treat = NULL;                               //定时治疗结束 弹窗询问
        Back_Window(&data->Window, "是否确认返回？");
        lv_obj_add_event_cb(data->Window.No_Btn, Chart_Btn_Event_cb, LV_EVENT_CLICKED, data);
        lv_obj_add_event_cb(data->Window.Yes_Btn, Chart_Btn_Event_cb, LV_EVENT_CLICKED, data);
        Chart_ShowConfirm_Key(data);                            /* [我们] B5-C 键盘化 */
        audio_play_request(VOICE_ID_DONE_CN,1500);              //语音播放：完成
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
