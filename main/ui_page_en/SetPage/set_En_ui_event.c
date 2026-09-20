/**
  ******************************************************************************
  * @文件名称   set_En_ui_event.c
  * @文件描述   英文设置页事件回调（两套 UI，阶段 E2；复制中文 SetPage 事件直译）
  *            导航/清理逻辑与中文一致（坑 6.8/B2-4/B3-3 守则全部保留）；
  *            参数存储（音量/亮度/语言）与中文共用 app_params / ui_switch_language。
  ******************************************************************************
  */

#include "set_En_ui_event.h"
#include "set_En_ui.h"
#include "basic.h"
#include "Frame.h"
#include "app_params.h"      /* [我们] 参数存储（NVS） */
#include "voice.h"           /* [我们] 语音控制（音量实时预览） */
#include "app_heartbeat.h"  /* [我们] 心跳（RTC 对时） */
#include "app_modbus.h"      /* [我们] 提交返回码 mb_err_t */
#include "esp_log.h"         /* [我们] 对时失败日志 */
#include "app_ota.h"
#include "app_wifi.h"
#include "esp_system.h"
#include "app_keypad.h"

static const char *TAG = "SetPageEn";

extern Frame_Widget_t Frame_widget;
extern Set_En_Widget_t Set_En_Widget;

static void Set_En_SliVol_UnSel_Style(Set_En_Widget_t* data);
static void Set_En_SliBri_UnSel_Style(Set_En_Widget_t* data);
static void Set_En_SelLang_UnSel_Style(Set_En_Widget_t* data);
static void Set_En_Time_UnSel_Style(Set_En_Widget_t* data);
static void Set_En_Date_UnSel_Style(Set_En_Widget_t* data);
static void ota_bar_refresh_en(lv_obj_t *bar, lv_obj_t *pct, int val, bool fail);
static void ota_ver_refresh_en(lv_obj_t *lbl, const ota_ui_state_t *st, ota_target_t t);
static void ota_btn_set_en(lv_obj_t *btn, lv_obj_t *lbl, const char *txt, bool en);
static void ota_focus_back_if_enabled_en(OTA_Widget_EN_t *data);
void OTA_En_Page_Enter(OTA_Widget_EN_t *w);

static void OTA_En_Page_Leave(OTA_Widget_EN_t *w)
{
    if (w->Timer) { lv_timer_del(w->Timer); w->Timer = NULL; }
    lv_group_t *g = app_keypad_get_group();
    if (g)
    {
        if (lv_obj_is_valid(w->Btn_Sure)) { lv_group_remove_obj(w->Btn_Sure); }
        if (lv_obj_is_valid(w->Btn_Back)) { lv_group_remove_obj(w->Btn_Back); }
    }
    w->Btn_Sure = NULL;
    w->Btn_Back = NULL;
    app_wifi_disconnect();
    Set_En_BackToList();
}


static void OTA_Timer_cb(lv_timer_t* t)
{
    OTA_Widget_EN_t* data = (OTA_Widget_EN_t*)lv_timer_get_user_data(t);
    const ota_ui_state_t *st = app_ota_ui_get_state();
    if (!lv_obj_is_valid(data->Btn_Back)) { lv_timer_delete(t); return; }

    static const ota_target_t row_target[OTA_TARGET_COUNT] =
    {
        OTA_TARGET_STM32,
        OTA_TARGET_IMAGE,
        OTA_TARGET_ESP32
    };
    lv_obj_t *bars[OTA_TARGET_COUNT] =
    {
        data->Bar_STM32,
        data->Bar_Bin,
        data->Bar_ESP32
    };
    lv_obj_t *pcts[OTA_TARGET_COUNT] =
    {
        data->Label_STM32,
        data->Label_Bin,
        data->Label_ESP32
    };
    lv_obj_t *vers[OTA_TARGET_COUNT] =
    {
        data->Label_Ver_STM32,
        data->Label_Ver_Bin,
        data->Label_Ver_ESP32
    };

    for(int i = 0; i < OTA_TARGET_COUNT; i++)
    {
        ota_target_t target = row_target[i];
        ota_bar_refresh_en(bars[i], pcts[i], st->progress[target], st->fail[target]);
        ota_ver_refresh_en(vers[i], st, target);
    }

    switch (st->phase) {
    case OTA_UI_CHECKING:
        if (st->wifi_state == APP_WIFI_CONNECTED) {
            lv_label_set_text(data->Label_Net, "Network OK");
            lv_obj_set_style_text_color(data->Label_Net, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
        } else {
            lv_label_set_text(data->Label_Net, "Connecting...");
            lv_obj_set_style_text_color(data->Label_Net, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
        }
        break;
    case OTA_UI_WIFI_FAIL:
        lv_label_set_text(data->Label_Net, "Network failed");
        lv_obj_set_style_text_color(data->Label_Net, lv_color_hex(FONT_RED_COLOR), LV_PART_MAIN);
        break;
    default:
        lv_label_set_text(data->Label_Net, "Network OK");
        lv_obj_set_style_text_color(data->Label_Net, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
        break;
    }

    bool any_update = st->has_update[OTA_TARGET_STM32] ||
                      st->has_update[OTA_TARGET_IMAGE] ||
                      st->has_update[OTA_TARGET_ESP32];
    switch (st->phase) {
    case OTA_UI_IDLE:
    case OTA_UI_CHECKING:
        lv_label_set_text(data->Label_OTA, "Checking Update...");
        ota_btn_set_en(data->Btn_Sure, data->Label_Sure, "Checking...", false);
        ota_btn_set_en(data->Btn_Back, data->Label_Back, "Back", true);
        break;
    case OTA_UI_WIFI_FAIL:
        lv_label_set_text(data->Label_OTA, "Network failed, please retry");
        ota_btn_set_en(data->Btn_Sure, data->Label_Sure, "Retry", true);
        ota_btn_set_en(data->Btn_Back, data->Label_Back, "Back", true);
        break;
    case OTA_UI_MANIFEST_FAIL:
        lv_label_set_text(data->Label_OTA, "Get version failed, please retry");
        ota_btn_set_en(data->Btn_Sure, data->Label_Sure, "Retry", true);
        ota_btn_set_en(data->Btn_Back, data->Label_Back, "Back", true);
        break;
    case OTA_UI_READY:
        if (any_update) {
            lv_label_set_text(data->Label_OTA, "New version found. Update now?");
            ota_btn_set_en(data->Btn_Sure, data->Label_Sure, "Start Update", true);
        } else {
            lv_label_set_text(data->Label_OTA, "Already up to date");
            ota_btn_set_en(data->Btn_Sure, data->Label_Sure, "Up to date", false);
            /* Sure is disabled and focus must remain on Back: enforce the
             * focused appearance even when LVGL does not emit FOCUSED again. */
            lv_obj_set_style_text_color(data->Label_Back, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Back, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
            ota_focus_back_if_enabled_en(data);
        }
        ota_btn_set_en(data->Btn_Back, data->Label_Back, "Back", true);
        break;
    case OTA_UI_UPGRADING:
        lv_label_set_text(data->Label_OTA, "Updating, do not power off...");
        ota_btn_set_en(data->Btn_Sure, data->Label_Sure, "Updating...", false);
        ota_btn_set_en(data->Btn_Back, data->Label_Back, "Updating...", false);
        break;
    case OTA_UI_DONE:
        if (st->all_ok) {
            lv_label_set_text(data->Label_OTA, "Update complete. Restart to apply");
            ota_btn_set_en(data->Btn_Sure, data->Label_Sure, "Completed", false);
            ota_btn_set_en(data->Btn_Back, data->Label_Back, "Restart", true);
            lv_obj_set_style_text_color(data->Label_Back, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Back, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
            ota_focus_back_if_enabled_en(data);
        } else {
            lv_label_set_text(data->Label_OTA, "Some updates failed (red bars), retry");
            ota_btn_set_en(data->Btn_Sure, data->Label_Sure, "Retry", true);
            ota_btn_set_en(data->Btn_Back, data->Label_Back, "Back", true);
        }
        break;
    }
}


void OTA_En_Page_Enter(OTA_Widget_EN_t *w)
{
    app_ota_ui_reset();
    lv_label_set_text(w->Label_OTA, "Checking Update...");
    lv_label_set_text(w->Label_Net, "Connecting...");
    ota_btn_set_en(w->Btn_Sure, w->Label_Sure, "Checking...", false);
    ota_btn_set_en(w->Btn_Back, w->Label_Back, "Back", true);
    app_ota_request(OTA_CMD_BEGIN_CHECK);
    if(w->Timer == NULL) { w->Timer = lv_timer_create(OTA_Timer_cb, 150, w); }
}


static void ota_bar_refresh_en(lv_obj_t *bar, lv_obj_t *pct, int val, bool fail)
{
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    lv_bar_set_value(bar, val, LV_ANIM_OFF);
    lv_label_set_text_fmt(pct, "%d%%", val);
    lv_obj_set_style_bg_color(bar, lv_color_hex(fail ? FONT_RED_COLOR : FONT_BLUE_COLOR), LV_PART_INDICATOR);
}

static void ota_ver_refresh_en(lv_obj_t *lbl, const ota_ui_state_t *st, ota_target_t t)
{
    const char *loc = st->local_ver[t];
    const char *clo = st->cloud_ver[t];
    bool cloud_known = (st->phase >= OTA_UI_READY) && clo[0] != '\0';
    if (cloud_known)
    {
        if (st->has_update[t]) { lv_label_set_text_fmt(lbl, "v%s->v%s", loc, clo); }
        else { lv_label_set_text_fmt(lbl, "v%s OK", loc); }
    }
    else if (st->ver_known[t])
    {
        lv_label_set_text_fmt(lbl, "v%s", loc);
    }
    else
    {
        lv_label_set_text(lbl, "");
    }
}

static void ota_btn_set_en(lv_obj_t *btn, lv_obj_t *lbl, const char *txt, bool en)
{
    if (lv_obj_is_valid(lbl)) { lv_label_set_text(lbl, txt); }
    if (!lv_obj_is_valid(btn)) return;
    if (en)
    {
        lv_obj_clear_state(btn, LV_STATE_DISABLED);
    }
    else
    {
        lv_obj_add_state(btn, LV_STATE_DISABLED);
        if (lv_obj_is_valid(lbl)) {
            lv_obj_set_style_text_color(lbl, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        }
        lv_obj_set_style_bg_color(btn, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
    }
}

static void ota_focus_back_if_enabled_en(OTA_Widget_EN_t *data)
{
    if (!lv_obj_is_valid(data->Btn_Back))
        return;

    if (lv_obj_has_state(data->Btn_Back, LV_STATE_DISABLED))
        return;

    lv_group_t *g = app_keypad_get_group();

    if (g && lv_group_get_focused(g) != data->Btn_Back)
    {
        lv_group_focus_obj(data->Btn_Back);
    }
}


/**
 * @brief 滑动条事件回调（英文设置页）
 */
void Set_En_Slider_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Set_En_Widget_t* user_data = (Set_En_Widget_t*)lv_event_get_user_data(e);
    int8_t value = 0;

    if (code == LV_EVENT_FOCUSED) {
        if (obj == user_data->Slider_Volume && lv_obj_is_valid(user_data->Slider_Volume)) {
            Set_En_SliVol_UnSel_Style(user_data);
            lv_obj_add_style(user_data->Slider_Volume, &style_BlueKnob, LV_PART_KNOB);
        }
        else if (obj == user_data->Btn_VolBack && lv_obj_is_valid(user_data->Btn_VolBack)) {
            Set_En_SliVol_UnSel_Style(user_data);
            lv_obj_set_style_text_color(user_data->Label_VolBack, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(user_data->Btn_VolBack, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
        else if (obj == user_data->Slider_Bright && lv_obj_is_valid(user_data->Slider_Bright)) {
            Set_En_SliBri_UnSel_Style(user_data);
            lv_obj_add_style(user_data->Slider_Bright, &style_BlueKnob, LV_PART_KNOB);
        }
        else if (obj == user_data->Btn_BriBack && lv_obj_is_valid(user_data->Btn_BriBack)) {
            Set_En_SliBri_UnSel_Style(user_data);
            lv_obj_set_style_text_color(user_data->Label_BriBack, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(user_data->Btn_BriBack, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
    }
    else if (code == LV_EVENT_VALUE_CHANGED) {
        if (obj == user_data->Slider_Volume) {
            value = lv_slider_get_value(obj);
            user_data->Value_Volume = value;
            app_params_set_volume(value);   /* [我们] 参数存储：同步内存副本 */
            if (user_data->Label_VolValue) lv_label_set_text_fmt(user_data->Label_VolValue, "%d", (int)value);
            voice_preview((uint8_t)value);  /* [我们] 语音实时预览（拖动出声，内部 debounce） */
            LV_LOG_USER("EN volume: %d", (int)value);
        }
        else if (obj == user_data->Slider_Bright) {
            value = lv_slider_get_value(obj);
            user_data->Value_Bright = value;
            app_params_set_bright(value);   /* [我们] 参数存储：同步内存副本 */
            if (user_data->Label_BriValue) lv_label_set_text_fmt(user_data->Label_BriValue, "%d", (int)value);
            LV_LOG_USER("EN brightness: %d", (int)value);
        }
    }
    else if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ESC) { Set_En_BackToList(); return; }
        if (key == LV_KEY_ENTER) {
            if (obj == user_data->Slider_Volume && lv_obj_is_valid(user_data->Btn_VolBack)) {
                lv_group_focus_obj(user_data->Btn_VolBack);
                lv_event_stop_processing(e);
            }
            else if (obj == user_data->Slider_Bright && lv_obj_is_valid(user_data->Btn_BriBack)) {
                lv_group_focus_obj(user_data->Btn_BriBack);
                lv_event_stop_processing(e);
            }
        }
        else if (key == LV_KEY_UP || key == LV_KEY_DOWN) {
            if (obj == user_data->Btn_VolBack && lv_obj_is_valid(user_data->Slider_Volume)) {
                lv_group_focus_obj(user_data->Slider_Volume);
                lv_event_stop_processing(e);
            }
            else if (obj == user_data->Btn_BriBack && lv_obj_is_valid(user_data->Slider_Bright)) {
                lv_group_focus_obj(user_data->Slider_Bright);
                lv_event_stop_processing(e);
            }
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        if (user_data->Btn_VolBack == obj) {
            Set_En_BackToList();
        }
        else if (user_data->Btn_BriBack == obj) {
            Set_En_BackToList();
        }
    }
}

/**
 * @brief 设置完成 定时器跳转函数（英文）
 */
void Set_En_timer_cb(lv_timer_t* t)
{
    (void)t;
    static uint8_t count = 0;
    count++;
    if (count >= 30) {
        count = 0;
        Set_En_BackToMenu();
    }
}

/**
 * @brief 选择语言事件回调（英文）
 */
void Set_En_Select_Language_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    Set_En_Widget_t* data = (Set_En_Widget_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {
        if (lv_obj_is_valid(data->Btn_Page3Next) && lv_obj_is_valid(data->Knob_ChiSrc)) {
            Set_En_SelLang_UnSel_Style(data);
        }
        if (data->Btn_Page3Next == obj && lv_obj_is_valid(data->Label_P3Next)) {
            lv_obj_set_style_text_color(data->Label_P3Next, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Page3Next, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
        else if (data->Btn_China == obj && lv_obj_is_valid(data->Knob_ChiSrc)) {
            lv_image_set_src(data->Knob_ChiSrc, EN_KNOB_SEL_ICON);
        }
        else if (data->Btn_English == obj && lv_obj_is_valid(data->Knob_EngSrc)) {
            lv_image_set_src(data->Knob_EngSrc, EN_KNOB_SEL_ICON);
        }
    }
    else if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ESC) { Set_En_BackToList(); return; }
        if (key == LV_KEY_DOWN) {
            if (obj == data->Btn_China)          { lv_group_focus_obj(data->Btn_English);  lv_event_stop_processing(e); }
            else if (obj == data->Btn_English)   { lv_group_focus_obj(data->Btn_Page3Next); lv_event_stop_processing(e); }
        }
        else if (key == LV_KEY_UP) {
            if (obj == data->Btn_Page3Next)      { lv_group_focus_obj(data->Btn_English);  lv_event_stop_processing(e); }
            else if (obj == data->Btn_English)   { lv_group_focus_obj(data->Btn_China);    lv_event_stop_processing(e); }
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        if (data->Btn_Page3Next == obj) {
            Set_En_SelLang_UnSel_Style(data);
            Set_En_Page_Load(En_Page_Date);
        }
        else if (data->Btn_China == obj) {
            ui_switch_language(LANG_ZH);
        }
        else if (data->Btn_English == obj) {
            ui_switch_language(LANG_EN);
        }
    }
}

/**
 * @brief 时间设置事件回调（英文）
 */
void Set_En_Time_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    Set_En_Widget_t* data = (Set_En_Widget_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {
        Set_En_Time_UnSel_Style(data);
        if (data->Btn_Sure == obj && lv_obj_is_valid(data->Label_Sure)) {
            lv_obj_set_style_text_color(data->Label_Sure, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Sure, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
    }
    else if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ESC) { Set_En_BackToList(); return; }
        if (key == LV_KEY_UP && obj == data->Btn_Sure && lv_obj_is_valid(data->Roller_Min)) {
            lv_group_focus_obj(data->Roller_Min);
            lv_event_stop_processing(e);
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        if (data->Btn_Sure == obj) {
            Set_En_Time_UnSel_Style(data);
            /* [我们] 真实时间设置：把草稿(年月日时分)写入 STM32 RTC（FC10+FC06）
             * 成功后顶栏由心跳(app_heartbeat)同步的 C 时钟自动刷新（1s 内） */
            int ret = app_heartbeat_commit();
            if (ret != MB_OK) {
                ESP_LOGW(TAG, "time commit failed: %s", app_modbus_err_str(ret));
            }
            Set_En_Page_Load(En_Page_Finish);
            if (data->Timer == NULL) {
                data->Timer = lv_timer_create(Set_En_timer_cb, 100, NULL);
            }
        }
    }
}

/**
 * @brief 日期设置事件回调（英文）
 */
void Set_En_Date_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    Set_En_Widget_t* data = (Set_En_Widget_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {
        if (lv_obj_is_valid(data->Btn_Page4Next)) Set_En_Date_UnSel_Style(data);
        if (data->Btn_Page4Next == obj && lv_obj_is_valid(data->Label_P4Next)) {
            lv_obj_set_style_text_color(data->Label_P4Next, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Page4Next, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
        else if (data->Btn_DateBack == obj && lv_obj_is_valid(data->Label_DatBack)) {
            lv_obj_set_style_text_color(data->Label_DatBack, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_DateBack, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
    }
    else if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ESC) { Set_En_BackToList(); return; }
        if (key == LV_KEY_UP || key == LV_KEY_DOWN) {
            if (obj == data->Btn_Page4Next && lv_obj_is_valid(data->Btn_DateBack)) {
                lv_group_focus_obj(data->Btn_DateBack); lv_event_stop_processing(e);
            }
            else if (obj == data->Btn_DateBack && lv_obj_is_valid(data->Btn_Page4Next)) {
                lv_group_focus_obj(data->Btn_Page4Next); lv_event_stop_processing(e);
            }
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        if (data->Btn_Page4Next == obj) {
            Set_En_Page_Load(En_Page_Time);
        }
        else if (data->Btn_DateBack == obj) {
            Set_En_BackToList();
        }
    }
}

/**
 * @brief 设置菜单列表事件回调（英文）
 */
void Set_En_List_event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    Set_En_Widget_t* data = (Set_En_Widget_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ESC) { Set_En_BackToMenu(); return; }
        int idx = -1;
        for (int i = 0; i < En_Btn_SUM; i++) if (obj == data->Btn[i]) { idx = i; break; }
        if (idx >= 0) {
            if (key == LV_KEY_DOWN) {
                lv_group_focus_obj(data->Btn[(idx + 1) % En_Btn_SUM]);
                lv_event_stop_processing(e);
            }
            else if (key == LV_KEY_UP) {
                lv_group_focus_obj(data->Btn[(idx + En_Btn_SUM - 1) % En_Btn_SUM]);
                lv_event_stop_processing(e);
            }
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        if (data->Btn[En_Btn_Language] == obj) {
            Set_En_Page_Load(En_Page_Language);
        }
        else if (data->Btn[En_Btn_Volume] == obj) {
            Set_En_Page_Load(En_Page_Volume);
        }
        else if (data->Btn[En_Btn_Bright] == obj) {
            Set_En_Page_Load(En_Page_Bright);
        }
        else if (data->Btn[En_Btn_Date] == obj) {
            Set_En_Page_Load(En_Page_Date);
        }
        else if (data->Btn[En_Btn_OTA] == obj) {
            Set_En_Page_Load(En_Page_OTA);
        }
    }
}

/**
 * @brief OTA升级按钮点击事件回调
 */
void Set_En_OTABtn_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    OTA_Widget_EN_t* data = (OTA_Widget_EN_t*)lv_event_get_user_data(e);
    const ota_ui_state_t *st = app_ota_ui_get_state();

    if (code == LV_EVENT_FOCUSED)
    {
        if (obj == data->Btn_Sure)
        {
           // 确认按钮：蓝底白字
           lv_obj_set_style_text_color(data->Label_Sure, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
           lv_obj_set_style_bg_color(data->Btn_Sure, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
           // 返回按钮恢复：白底蓝字
           lv_obj_set_style_text_color(data->Label_Back, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
           lv_obj_set_style_bg_color(data->Btn_Back, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);

        }
        else if (obj == data->Btn_Back)
        {
             // 返回按钮：蓝底白字
            lv_obj_set_style_text_color( data->Label_Back, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color( data->Btn_Back,lv_color_hex(FONT_BLUE_COLOR),LV_STATE_DEFAULT);
             // 确认按钮恢复
            lv_obj_set_style_text_color(  data->Label_Sure, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color( data->Btn_Sure, lv_color_hex(FONT_WHITE_COLOR),LV_STATE_DEFAULT);
        }
    }
    else if (code == LV_EVENT_CLICKED)
    {
        if (obj == data->Btn_Sure)
        {
            if (st->phase == OTA_UI_READY) { app_ota_request(OTA_CMD_UPGRADE_ALL); }
            else { app_ota_ui_reset(); app_ota_request(OTA_CMD_BEGIN_CHECK); }
        }
       // else if (obj == data->Btn_Back)
       // {
       //     if (st->phase == OTA_UI_UPGRADING) return;
       //     if (st->phase == OTA_UI_DONE && st->all_ok) { esp_restart(); return; }
        //    Set_En_BackToList();
        else if (obj == data->Btn_Back)
        {
           if (st->phase == OTA_UI_UPGRADING) return;
           if (st->phase == OTA_UI_DONE && st->all_ok) { esp_restart(); return; }
           OTA_En_Page_Leave(data);
        }
    }
    else if (code == LV_EVENT_KEY)
    {
        uint32_t key = lv_event_get_key(e);
       // if (key == LV_KEY_ESC) { Set_En_BackToList(); return; }
       if (key == LV_KEY_ESC){ OTA_En_Page_Leave(data);return;}
        if (key == LV_KEY_DOWN && obj == data->Btn_Sure &&
            !lv_obj_has_state(data->Btn_Back, LV_STATE_DISABLED))
        {
            lv_group_focus_obj(data->Btn_Back);
            lv_event_stop_processing(e);
        }
        else if (key == LV_KEY_UP && obj == data->Btn_Back &&
                 !lv_obj_has_state(data->Btn_Sure, LV_STATE_DISABLED))
        {
            lv_group_focus_obj(data->Btn_Sure);
            lv_event_stop_processing(e);
        }
    }
}

/* ---- 未选中样式（英文设置页） ---- */
static void Set_En_SliVol_UnSel_Style(Set_En_Widget_t* data)
{
    lv_obj_remove_style(data->Slider_Volume, &style_BlueKnob, LV_PART_KNOB);
    lv_obj_set_style_text_color(data->Label_VolBack, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_VolBack, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
}
static void Set_En_SliBri_UnSel_Style(Set_En_Widget_t* data)
{
    lv_obj_remove_style(data->Slider_Bright, &style_BlueKnob, LV_PART_KNOB);
    lv_obj_set_style_text_color(data->Label_BriBack, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_BriBack, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
}
static void Set_En_SelLang_UnSel_Style(Set_En_Widget_t* data)
{
    lv_image_set_src(data->Knob_ChiSrc, EN_KNOB_UNSEL_ICON);
    lv_image_set_src(data->Knob_EngSrc, EN_KNOB_UNSEL_ICON);
    lv_obj_set_style_text_color(data->Label_P3Next, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_Page3Next, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
}
static void Set_En_Time_UnSel_Style(Set_En_Widget_t* data)
{
    lv_obj_set_style_text_color(data->Label_Sure, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_Sure, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
}
static void Set_En_Date_UnSel_Style(Set_En_Widget_t* data)
{
    lv_obj_set_style_text_color(data->Label_P4Next, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_Page4Next, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(data->Label_DatBack, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_DateBack, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
}

/* [我们] 解析滚轮选中串（纯数字）→ 数字 */
static int set_En_parse_num(const char *s)
{
    int v = 0, started = 0;
    if (s == NULL) return 0;
    for (; *s != '\0'; s++)
    {
        if (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); started = 1; }
        else if (started) break;
    }
    return started ? v : 0;
}

/**
 * @brief 时间/日期滚轮值捕获（英文）：[我们] 解析选中项 → 更新对时草稿
 */
void Set_En_Roller_event_cb(lv_event_t* e)
{
    lv_obj_t* obj;
    Set_En_Widget_t* data;
    char buf[16];
    app_rtc_t t;
    lv_event_code_t code = lv_event_get_code(e);

    /* [我们] 捕获时机（关键坑：本 LVGL 版滚轮按键滚动【不】发 VALUE_CHANGED，
     * 只在 release_handler（拖拽释放）里发）→ 必须补抓 LV_EVENT_KEY(滚动键)。
     * 事件顺序：原生类回调先执行（已滚动到新值），本回调随后读到新选中项。 */
    if (code == LV_EVENT_VALUE_CHANGED) {
        /* 触屏/拖拽路径 */
    } else if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key != LV_KEY_UP && key != LV_KEY_DOWN &&
            key != LV_KEY_LEFT && key != LV_KEY_RIGHT) return;
    } else {
        return;
    }

    obj  = lv_event_get_target(e);
    data = (Set_En_Widget_t*)lv_event_get_user_data(e);
    if (data == NULL) return;

    app_heartbeat_get_pending(&t);

    if (obj == data->Roller_Year) {
        lv_roller_get_selected_str(data->Roller_Year, buf, sizeof(buf));
        t.year = set_En_parse_num(buf);
    } else if (obj == data->Roller_Month) {
        lv_roller_get_selected_str(data->Roller_Month, buf, sizeof(buf));
        t.month = set_En_parse_num(buf);
    } else if (obj == data->Roller_Day) {
        lv_roller_get_selected_str(data->Roller_Day, buf, sizeof(buf));
        t.day = set_En_parse_num(buf);
    } else if (obj == data->Roller_Hour) {
        lv_roller_get_selected_str(data->Roller_Hour, buf, sizeof(buf));
        t.hour = set_En_parse_num(buf);
    } else if (obj == data->Roller_Min) {
        lv_roller_get_selected_str(data->Roller_Min, buf, sizeof(buf));
        t.minute = set_En_parse_num(buf);
    } else {
        return;
    }

    app_heartbeat_set_pending(&t);
}

/* ===== [我们] 滚轮按键：ENTER 前进焦点 / ESC 返回 ===== */
void Set_En_Roller_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);

    if (key == LV_KEY_ESC) { Set_En_BackToList(); return; }
    if (key != LV_KEY_ENTER) return;

    lv_obj_t* target = NULL;
    if (obj == Set_En_Widget.Roller_Year)       target = Set_En_Widget.Roller_Month;
    else if (obj == Set_En_Widget.Roller_Month) target = Set_En_Widget.Roller_Day;
    else if (obj == Set_En_Widget.Roller_Day)   target = Set_En_Widget.Btn_Page4Next;
    else if (obj == Set_En_Widget.Roller_Hour)  target = Set_En_Widget.Roller_Min;
    else if (obj == Set_En_Widget.Roller_Min)   target = Set_En_Widget.Btn_Sure;
    if (target && lv_obj_is_valid(target)) {
        lv_group_focus_obj(target);
        lv_event_stop_processing(e);
    }
}

/* [我们] 无按钮页面的 ESC 处理（设置完成页，3s 定时器也会自动返回） */
void Set_En_Page_Esc_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        Set_En_BackToMenu();
    }
}
