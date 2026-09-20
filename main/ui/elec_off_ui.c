/**
  ******************************************************************************
  * @文件名称   elec_off_ui.c
  * @文件描述   全局「电极脱落」提示弹窗实现（见 elec_off_ui.h 头注释）
  * @设计要点
  *   1. 弹窗挂 lv_layer_top()：所有页面都建在 g_Ui.page_container 里, 切页会
  *      lv_obj_clean(page_container); 挂顶层可跨页面存活, 不被切页清掉。
  *   2. 边沿触发：仅在状态 0→非0 的上升沿弹一次; 弹窗关闭后只要仍脱落就不重弹,
  *      直到状态回到 0(电极贴好)才重新武装。
  *   3. 纯提示单按钮：不停治疗/采集; ENTER/「确定」关闭, ESC 也可关闭。
  *   4. 弹窗显示期间 lv_group_focus_freeze(true)：物理方向键不再把焦点移到页面
  *      按钮, 避免治疗中按方向/确认误触页面控件(安全关键)。
  ******************************************************************************
  */

#include "lvgl.h"
#include "esp_log.h"
#include "elec_off_ui.h"
#include "basic.h"
#include "lang.h"
#include "app_keypad.h"
#include "app_modbus.h"
#include "voice.h"
#include "app_therapy.h"

static const char *TAG = "elec_off_ui";

#define ELEC_POLL_MS       300    /* 状态轮询周期 (ms): 非采集期心跳 1s 刷新, 采集期 500ms 推帧 */
#define ELEC_BOX_W         250
#define ELEC_BOX_H         132

static lv_obj_t *s_overlay = NULL;   /* 顶层遮罩(弹窗根), 非 NULL = 正在显示 */
static lv_obj_t *s_btn     = NULL;   /* 唯一「确定」按钮(group 焦点) */
static bool      s_prev_off = false; /* 上一次采样的脱落状态(做边沿) */

/* ---- 关闭弹窗: 解冻 group + 移出按钮 + 删顶层 ---- */
static void elec_popup_close(void)
{
    if (s_overlay == NULL)
        return;

    lv_group_t *g = app_keypad_get_group();
    if (g)
    {
        if (s_btn && lv_obj_is_valid(s_btn))
            lv_group_remove_obj(s_btn);
        lv_group_focus_freeze(g, false);   /* 恢复页面正常按键导航 */
    }

    lv_obj_t *ov = s_overlay;
    s_overlay = NULL;
    s_btn     = NULL;
    lv_obj_delete_async(ov);

    ESP_LOGI(TAG, "electrode-off popup closed");
}

/* 按钮 CLICKED(触摸/ENTER) → 关闭 */
static void elec_btn_clicked_cb(lv_event_t *e)
{
    (void)e;
    therapy_resume();       /* 恢复治疗 */
    elec_popup_close();
}

/* 按钮 KEY: ESC 也关闭(ENTER 由 button 默认转 CLICKED 处理) */
static void elec_btn_key_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY)
        return;
    if (lv_event_get_key(e) == LV_KEY_ESC) {
        therapy_resume();       /* 恢复治疗 */
        elec_popup_close();
    }
        
}

/* ---- 弹出提示(挂 lv_layer_top, 跨页面) ---- */
static void elec_popup_show(void)
{
    if (s_overlay != NULL)
        return;   /* 已在显示, 不重复弹 */

    bool en = (g_app_lang == LANG_EN);

    /* 顶层全屏遮罩: 拦截页面触摸, 但不做点击关闭(必须按确定, 防误触) */
    lv_obj_t *ov = lv_obj_create(lv_layer_top());
    lv_obj_set_size(ov, LV_PCT(100), LV_PCT(100));
    lv_obj_set_pos(ov, 0, 0);
    lv_obj_set_scrollbar_mode(ov, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(ov, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(ov, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(ov, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ov, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ov, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ov, LV_OPA_50, LV_PART_MAIN);
    s_overlay = ov;

    /* 居中消息框 */
    lv_obj_t *box = lv_obj_create(ov);
    lv_obj_set_size(box, ELEC_BOX_W, ELEC_BOX_H);
    lv_obj_center(box);
    lv_obj_set_scrollbar_mode(box, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(box, 8, LV_PART_MAIN);
    lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_color(box, lv_color_hex(0x5e5f62), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(box, 0, LV_PART_MAIN);

    /* 提示文字 */
    const char *txt = en ? "Electrode off\nPlease check electrodes"
                         : "电极脱落\n请检查电极是否贴好";
    const lv_font_t *font = en ? &lv_font_montserrat_14
                               : basic_widget.Chinise_Font_Elec;  /* 专用子集字库(含脱/落/检/查/贴) */
    lv_obj_t *label = Creat_Label(box, (char *)txt, font);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 24);

    /* 唯一「确定」按钮 */
    lv_obj_t *btn = Create_Button(box, 96, 36, FONT_BLUE_COLOR, 6, 0);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -16);
    lv_obj_t *blabel = Creat_Label(btn, (char *)(en ? "OK" : "确定"),
                                   en ? &lv_font_montserrat_14
                                      : basic_widget.Chinise_Font_Elec);
    lv_obj_set_style_text_color(blabel, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    lv_obj_center(blabel);
    lv_obj_add_event_cb(btn, elec_btn_clicked_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(btn, elec_btn_key_cb, LV_EVENT_KEY, NULL);
    s_btn = btn;

    /* 进 group + 聚焦 + 冻结: 弹窗期间方向键不切到页面按钮, ENTER 只作用于确定 */
    lv_group_t *g = app_keypad_get_group();
    if (g)
    {
        lv_group_add_obj(g, btn);
        lv_group_focus_obj(btn);
        lv_group_focus_freeze(g, true);
    }

    ESP_LOGI(TAG, "electrode-off popup shown");
}

/* ---- LVGL 周期监控: 读统一快照做边沿判断 ---- */
static void elec_monitor_cb(lv_timer_t *t)
{
    (void)t;

    uint16_t st = 0;
    if (!app_modbus_get_elec_status(&st))
        return;   /* 尚未取得过有效状态(开机/未通信), 不弹 */

    bool off = (st != 0);

    if (!off)
    {
        s_prev_off = false;   /* 电极已贴好: 重新武装, 允许下次脱落再弹 */
        return;
    }

    /* 脱落: 仅 0→非0 上升沿且当前无弹窗时弹一次 */
    if (!s_prev_off && s_overlay == NULL) {
        elec_popup_show();
        therapy_pause();                                  /* 暂停治疗 */
        audio_play_request(VOICE_ID_ELEC_OFF_CN, 1500);   /* 脱落语音提示 */

    }
        


    s_prev_off = true;
}

void elec_off_ui_init(void)
{
    s_overlay  = NULL;
    s_btn      = NULL;
    s_prev_off = false;
    lv_timer_create(elec_monitor_cb, ELEC_POLL_MS, NULL);
    ESP_LOGI(TAG, "electrode-off popup monitor init (%d ms)", ELEC_POLL_MS);
}
