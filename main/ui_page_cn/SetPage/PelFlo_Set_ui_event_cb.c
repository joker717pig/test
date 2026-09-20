#include "PelFlo_Set_ui_event_cb.h"
#include "PelFlo_Set_ui.h"
#include "basic.h"
#include "Frame.h"
#include "menu_ui.h"      /* [我们] include 扁平化（menu_ui.h 在 INCLUDE_DIRS） */
#include "app_params.h"   /* [我们] 参数存储（NVS） */
#include "voice.h"        /* [我们] 语音控制（音量实时预览） */
#include "app_heartbeat.h"  /* [我们] 心跳（RTC 对时） */
#include "app_modbus.h"   /* [我们] 提交返回码 mb_err_t */
#include "app_ota.h"      /* [我们] OTA 全局状态轮询 + 命令 */
#include "app_wifi.h"     /* [我们] 离开 OTA 页关 WiFi */
#include "app_keypad.h"   /* [我们] keypad group 移出 */
#include "esp_log.h"      /* [我们] 对时失败日志 */
#include "esp_system.h"   /* [我们] esp_restart() 升级完成重启 */

static const char *TAG = "SetPage";

/* 前向声明（OTA_Page_Enter 内创建轮询定时器用到） */
static void OTA_Timer_cb(lv_timer_t* t);

/* 进 OTA 页：复位状态 → 投 BEGIN_CHECK（ota 任务读版本+连WiFi+拉清单）→ 启动轮询 */
void OTA_Page_Enter(OTA_Widget_t *w)
{
    app_ota_ui_reset();
    app_ota_request(OTA_CMD_BEGIN_CHECK);
    if (w->Timer == NULL) {
        w->Timer = lv_timer_create(OTA_Timer_cb, 150, w);
    }
}

extern lv_my_ui_page_date_t lv_my_ui_pageSet_t;
extern lv_my_child_page_date_t* gChild_SetPage_list;
extern lv_my_ui_page_date_t* g_page_list[APP_SUM];


extern Frame_Widget_t Frame_widget;
extern  Set_Widget_t Set_Widget;

static void Set_SliVol_UnSel_Style(Set_Widget_t* data);
static void Set_SliBri_UnSel_Style(Set_Widget_t* data);
static void Set_SelLang_UnSel_Style(Set_Widget_t* data);
static void Set_Time_UnSel_Style(Set_Widget_t* data);
static void Set_Date_UnSel_Style(Set_Widget_t* data);


/* ===== [我们] OTA 页进/出 + 真实状态轮询（阶段 D 步骤 2/5） ===== */

/* 离开 OTA 页：停轮询 + 移出 group（防悬垂）+ 关 WiFi + 返回设置列表 */
static void OTA_Page_Leave(OTA_Widget_t *w)
{
    if (w->Timer) { lv_timer_del(w->Timer); w->Timer = NULL; }
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        if (lv_obj_is_valid(w->Btn_Sure)) lv_group_remove_obj(w->Btn_Sure);
        if (lv_obj_is_valid(w->Btn_Back)) lv_group_remove_obj(w->Btn_Back);
    }
    app_wifi_disconnect();
    Set_BackToList();
}

/* 刷新单条进度条（值 + 条内百分比 + 失败标红） */
static void ota_bar_refresh(lv_obj_t *bar, lv_obj_t *pct, int val, bool fail)
{
    if (val < 0) val = 0;
    if (val > 100) val = 100;
    lv_bar_set_value(bar, val, LV_ANIM_OFF);
    lv_label_set_text_fmt(pct, "%d%%", val);
    lv_obj_set_style_bg_color(bar, lv_color_hex(fail ? FONT_RED_COLOR : FONT_BLUE_COLOR),
                              LV_PART_INDICATOR);
}

/* 刷新单行版本号：检查中显示本地 vX.Y；拿到云端后 有更新 vX.Y→vA.B / 无更新 vX.Y ✓ */
static void ota_ver_refresh(lv_obj_t *lbl, const ota_ui_state_t *st, ota_target_t t)
{
    const char *loc = st->local_ver[t];
    const char *clo = st->cloud_ver[t];
    bool cloud_known = (st->phase >= OTA_UI_READY) && clo[0] != '\0';
    if (cloud_known) {
        if (st->has_update[t])
            lv_label_set_text_fmt(lbl, "v%s→v%s", loc, clo);
        else
            lv_label_set_text_fmt(lbl, "v%s √", loc);   /* SimHei 无 ✓，借 √ 作对勾 */
    } else if (st->ver_known[t]) {
        lv_label_set_text_fmt(lbl, "v%s", loc);
    } else {
        lv_label_set_text(lbl, "");
    }
}

/* 按钮使能/禁用 + 文案 */
static void ota_btn_set(lv_obj_t *btn, lv_obj_t *lbl, const char *txt, bool en)
{
    lv_label_set_text(lbl, txt);
    if (en) lv_obj_clear_state(btn, LV_STATE_DISABLED);   /* LVGL9：禁用是 state 不是 flag */
    else    lv_obj_add_state(btn, LV_STATE_DISABLED);
}

/* 焦点自愈：LVGL 在"焦点对象被禁用"时会丢弃该对象的全部按键(ENTER/ESC/UP/DOWN 都不投递，
 * 见 lv_indev.c is_enabled 分支)。若焦点停在被禁用的"确定"键上，用户既按不动确定、也按
 * DOWN 切不到"重启/返回"键 → 死锁。在"确定键永久禁用"(已完成 / 已是最新)时把焦点主动切到
 * 可用的 Back(重启/返回)键。定时器每帧调用，已在 Back 上则不重复切。 */
static void ota_focus_back_if_enabled(OTA_Widget_t *data)
{
    if (!lv_obj_is_valid(data->Btn_Back)) return;
    if (lv_obj_has_state(data->Btn_Back, LV_STATE_DISABLED)) return;
    lv_group_t *g = app_keypad_get_group();
    if (g && lv_group_get_focused(g) != data->Btn_Back) {
        lv_group_focus_obj(data->Btn_Back);
    }
}

/**
 * @brief OTA 轮询回调（LVGL 任务上下文，~150ms）：读 g_ota_ui 刷新控件
 */
static void OTA_Timer_cb(lv_timer_t* t)
{
    OTA_Widget_t* data = (OTA_Widget_t*) lv_timer_get_user_data(t);
    const ota_ui_state_t *st = app_ota_ui_get_state();

    /* 防悬垂：页面已销毁则停表 */
    if (!lv_obj_is_valid(data->Btn_Back)) { lv_timer_delete(t); return; }

    /* 三目标 → 控件映射：屏幕行顺序 STM32/图片/ESP32，但状态数组下标按 ota_target_t
     * （ESP32=0, IMAGE=1, STM32=2）。必须用 row_target[] 把"行"翻译成"目标枚举"再取状态，
     * 否则进度条/版本号会错位（STM32 行显示成 ESP32 进度）。 */
    static const ota_target_t row_target[OTA_TARGET_COUNT] = {
        OTA_TARGET_STM32, OTA_TARGET_IMAGE, OTA_TARGET_ESP32 };
    lv_obj_t *bars[OTA_TARGET_COUNT]   = { data->Bar_STM32, data->Bar_Bin, data->Bar_ESP32 };
    lv_obj_t *pcts[OTA_TARGET_COUNT]   = { data->Label_STM32, data->Label_Bin, data->Label_ESP32 };
    lv_obj_t *vers[OTA_TARGET_COUNT]   = { data->Label_Ver_STM32, data->Label_Ver_Bin, data->Label_Ver_ESP32 };
    for (int i = 0; i < OTA_TARGET_COUNT; i++) {
        ota_target_t t = row_target[i];
        ota_bar_refresh(bars[i], pcts[i], st->progress[t], st->fail[t]);
        ota_ver_refresh(vers[i], st, t);
    }

    /* 联网状态标签 */
    switch (st->phase) {
    case OTA_UI_CHECKING:
        if (st->wifi_state == APP_WIFI_CONNECTED) {
            lv_label_set_text(data->Label_Net, "联网成功");
            lv_obj_set_style_text_color(data->Label_Net, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
        } else {
            lv_label_set_text(data->Label_Net, "联网中…");
            lv_obj_set_style_text_color(data->Label_Net, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
        }
        break;
    case OTA_UI_WIFI_FAIL:
        lv_label_set_text(data->Label_Net, "联网失败");
        lv_obj_set_style_text_color(data->Label_Net, lv_color_hex(FONT_RED_COLOR), LV_PART_MAIN);
        break;
    default:    /* READY / UPGRADING / DONE / MANIFEST_FAIL 均已拿到 IP */
        lv_label_set_text(data->Label_Net, "联网成功");
        lv_obj_set_style_text_color(data->Label_Net, lv_color_hex(FONT_GREEN_COLOR), LV_PART_MAIN);
        break;
    }

    /* 主提示 + 两按钮（按状态机） */
    bool any_update = st->has_update[OTA_TARGET_STM32] ||
                      st->has_update[OTA_TARGET_IMAGE] ||
                      st->has_update[OTA_TARGET_ESP32];
    switch (st->phase) {
    case OTA_UI_IDLE:
    case OTA_UI_CHECKING:
        lv_label_set_text(data->Label_OTA, "正在检查更新…");
        ota_btn_set(data->Btn_Sure, data->Label_Sure, "检查中…", false);
        ota_btn_set(data->Btn_Back, data->Label_Back, "返回", true);
        break;
    case OTA_UI_WIFI_FAIL:
        lv_label_set_text(data->Label_OTA, "联网失败，请确认热点后重试");
        ota_btn_set(data->Btn_Sure, data->Label_Sure, "重试", true);
        ota_btn_set(data->Btn_Back, data->Label_Back, "返回", true);
        break;
    case OTA_UI_MANIFEST_FAIL:
        lv_label_set_text(data->Label_OTA, "获取版本信息失败，请重试");
        ota_btn_set(data->Btn_Sure, data->Label_Sure, "重试", true);
        ota_btn_set(data->Btn_Back, data->Label_Back, "返回", true);
        break;
    case OTA_UI_READY:
        if (any_update) {
            lv_label_set_text(data->Label_OTA, "发现新版本，是否升级？");
            ota_btn_set(data->Btn_Sure, data->Label_Sure, "开始升级", true);
            ota_btn_set(data->Btn_Back, data->Label_Back, "返回", true);
        } else {
            lv_label_set_text(data->Label_OTA, "已是最新版本");
            ota_btn_set(data->Btn_Sure, data->Label_Sure, "已是最新", false);
            ota_btn_set(data->Btn_Back, data->Label_Back, "返回", true);
            ota_focus_back_if_enabled(data);   /* 确定键禁用：焦点切到返回，否则按键全被丢弃、退不出去 */
        }
        break;
    case OTA_UI_UPGRADING:
        lv_label_set_text(data->Label_OTA, "正在升级，请勿断电…");
        ota_btn_set(data->Btn_Sure, data->Label_Sure, "升级中…", false);
        ota_btn_set(data->Btn_Back, data->Label_Back, "升级中…", false);
        break;
    case OTA_UI_DONE:
        if (st->all_ok) {
            lv_label_set_text(data->Label_OTA, "升级完成，请重启生效");
            ota_btn_set(data->Btn_Sure, data->Label_Sure, "已完成", false);
            ota_btn_set(data->Btn_Back, data->Label_Back, "升级完成(点击重启)", true);
            ota_focus_back_if_enabled(data);   /* 焦点切到重启键，否则完成键选不中、按确定没反应 */
        } else {
            lv_label_set_text(data->Label_OTA, "部分项目升级失败(红条)，可重试");
            ota_btn_set(data->Btn_Sure, data->Label_Sure, "重试", true);
            ota_btn_set(data->Btn_Back, data->Label_Back, "返回", true);
        }
        break;
    }
}

/**
 * @brief OTA升级按钮点击事件回调
 * @param e
 */
void SetPage_OTABtn_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    OTA_Widget_t* data = (OTA_Widget_t*)lv_event_get_user_data(e);        // 用户传进来的数据
    const ota_ui_state_t *st = app_ota_ui_get_state();
    if (code == LV_EVENT_FOCUSED) {
        if (obj == data->Btn_Sure) {
            lv_obj_set_style_text_color(data->Label_Sure, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Sure, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);

            lv_obj_set_style_text_color(data->Label_Back, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Back, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);

        }
        else if (obj == data->Btn_Back) {
            lv_obj_set_style_text_color(data->Label_Back, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Back, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);

            lv_obj_set_style_text_color(data->Label_Sure, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Sure, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);

        }

    }
    else if (code == LV_EVENT_CLICKED) {
        if (obj == data->Btn_Sure) {
            /* READY 且有更新 → 开始顺序升级；其余可点态（失败/重试）→ 重新检查 */
            if (st->phase == OTA_UI_READY) {
                LV_LOG_USER("OTA: start upgrade all");
                app_ota_request(OTA_CMD_UPGRADE_ALL);
            } else {
                LV_LOG_USER("OTA: re-check");
                app_ota_ui_reset();
                app_ota_request(OTA_CMD_BEGIN_CHECK);
            }
        }
        if (obj == data->Btn_Back) {
            /* 升级中禁用不响应；全完成点此重启；其余返回设置列表 */
            if (st->phase == OTA_UI_UPGRADING) return;
            if (st->phase == OTA_UI_DONE && st->all_ok) {
                esp_restart();
                return;
            }
            OTA_Page_Leave(data);
        }
    }
    else if (code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ESC) { OTA_Page_Leave(data); return; }

        /* 只在目标按钮可用时才切焦点：切到被禁用按钮会让 LVGL 丢弃所有按键(再次死锁) */
        if (key == LV_KEY_DOWN && obj == data->Btn_Sure &&
            !lv_obj_has_state(data->Btn_Back, LV_STATE_DISABLED)) {
            lv_group_focus_obj(data->Btn_Back);
            lv_event_stop_processing(e);
        }
        else if (key == LV_KEY_UP && obj == data->Btn_Back &&
                 !lv_obj_has_state(data->Btn_Sure, LV_STATE_DISABLED)) {
            lv_group_focus_obj(data->Btn_Sure);
            lv_event_stop_processing(e);
        }
    }
}

/**
 * @author zqt
 * @brief 滑动条事件回调函数
 * @param e
 */
void SetPage_Slider_Event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);                 // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);            // 触发的事件
    Set_Widget_t* user_data = (Set_Widget_t*)lv_event_get_user_data(e);        // 用户传进来的数据
    //lv_my_child_page_date_t* page_date = lv_obj_get_user_data(obj);  //获取页面数据
    int8_t value = 0;

    if (code == LV_EVENT_FOCUSED) {                         //对焦事件
        /* [我们] lv_obj_is_valid 门控：音量页↔亮度页共享本回调，防垂死对象（坑 B2-4 族） */
        if (obj == user_data->Slider_Volume && lv_obj_is_valid(user_data->Slider_Volume)) {  //音量滑动条
            Set_SliVol_UnSel_Style(user_data);
            lv_obj_add_style(user_data->Slider_Volume, &style_BlueKnob, LV_PART_KNOB);
        }
        else if (obj == user_data->Btn_VolBack && lv_obj_is_valid(user_data->Btn_VolBack)) { //音量页返回按钮
            Set_SliVol_UnSel_Style(user_data);
            lv_obj_set_style_text_color(user_data->Label_VolBack, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(user_data->Btn_VolBack, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
        else if (obj == user_data->Slider_Bright && lv_obj_is_valid(user_data->Slider_Bright)) { //亮度滑动条
            Set_SliBri_UnSel_Style(user_data);
            lv_obj_add_style(user_data->Slider_Bright, &style_BlueKnob, LV_PART_KNOB);
        }
        else if (obj == user_data->Btn_BriBack && lv_obj_is_valid(user_data->Btn_BriBack)) {   //亮度页返回按钮
            Set_SliBri_UnSel_Style(user_data);
            lv_obj_set_style_text_color(user_data->Label_BriBack, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(user_data->Btn_BriBack, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
    }
   
    else if (code == LV_EVENT_VALUE_CHANGED) {             //滑动条值改变事件

        if (obj == user_data->Slider_Volume) {
            value = lv_slider_get_value(obj);
            user_data->Value_Volume = value;
            app_params_set_volume(value);   /* [我们] 参数存储：同步内存副本（保存时机=离开页/确认） */
            /* [我们] 右端数字实时显示滑条值 */
            if (user_data->Label_VolValue) lv_label_set_text_fmt(user_data->Label_VolValue, "%d", (int)value);
            voice_preview((uint8_t)value);  /* [我们] 语音实时预览（拖动出声，内部 debounce） */
            LV_LOG_USER("音量值：%d", (int)value);
        }
        else if (obj == user_data->Slider_Bright) {
            value = lv_slider_get_value(obj);
            user_data->Value_Bright = value;
            app_params_set_bright(value);  /* [我们] 参数存储：同步内存副本（保存时机=离开页/确认） */
            /* [我们] 右端数字实时显示滑条值 */
            if (user_data->Label_BriValue) lv_label_set_text_fmt(user_data->Label_BriValue, "%d", (int)value);
            LV_LOG_USER("亮度值：%d", (int)value);
        }
        
    }
    else if (code == LV_EVENT_KEY) {                       /* [我们] 按键导航 */
        uint32_t key = lv_event_get_key(e);
        /* [我们] 返回设置列表；勿 stop_processing：stop_processing 会先 break 跳过 e->deleted 检查，
         * res 保持 OK → indev 补发 LV_EVENT_CANCEL 到已删对象 → use-after-free 崩溃（坑 B3-3） */
        if (key == LV_KEY_ESC) { 
            Set_BackToList(); 
            return; 
        }
        if (key == LV_KEY_ENTER) {
            /* ENTER：滑条 → 返回按钮（LEFT/RIGHT 留给滑条原生调值） */
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
            /* 返回按钮上按方向键回到滑条 */
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

            Set_BackToList();               /* [我们] 返回设置列表（二级菜单） */
        }
        else if (user_data->Btn_BriBack == obj) {

            Set_BackToList();
        }
    }
}

/**
 * @brief 设置完成 定时器跳转函数
 * @param t
 */
void Set_timer_cb(lv_timer_t* t)
{
    (void)t;
    static uint8_t count = 0;

    LV_LOG_USER("report_timer_cb:%d", count);
    count++;
    if (count >= 30) {
        count = 0;
        Set_BackToMenu();               // [我们] 统一清理：停定时器 + 移出 group + 返回菜单
    }
}
/**
 * @brief 选择语言事件回调函数
 * @param e 
 */
void SetPage_Select_Language_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    Set_Widget_t* data = (Set_Widget_t*)lv_event_get_user_data(e);

     if (code == LV_EVENT_FOCUSED) {
         /* [我们] lv_obj_is_valid 门控：本页对象（防垂死引用，坑 B2-4 族） */
         if (lv_obj_is_valid(data->Btn_Page3Next) && lv_obj_is_valid(data->Knob_ChiSrc)) {
             Set_SelLang_UnSel_Style(data);
         }
        if (data->Btn_Page3Next == obj && lv_obj_is_valid(data->Label_P3Next)) {
            lv_obj_set_style_text_color(data->Label_P3Next, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Page3Next, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
        else if (data->Btn_China == obj && lv_obj_is_valid(data->Knob_ChiSrc)) {
            lv_image_set_src(data->Knob_ChiSrc, KNOB_SEL_ICON);
        }
        else if (data->Btn_English == obj && lv_obj_is_valid(data->Knob_EngSrc)) {
            lv_image_set_src(data->Knob_EngSrc, KNOB_SEL_ICON);
        }
     }
    else if (code == LV_EVENT_KEY) {                  /* [我们] UP/DOWN 切换 + ESC 返回列表 */
        uint32_t key = lv_event_get_key(e);
        /* [我们] 勿 stop_processing（防 CANCEL 打到已删对象，坑 B3-3） */
        if (key == LV_KEY_ESC) { Set_BackToList(); return; }
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
            Set_SelLang_UnSel_Style(data);          //跳转到设置日期页
            Set_Page_Load(Page_Date);
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
 * @brief 时间设置事件回调函数
 * @param e
 */
void SetPage_Time_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    Set_Widget_t* data = (Set_Widget_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {
        Set_Time_UnSel_Style(data);
        if (data->Btn_Sure == obj && lv_obj_is_valid(data->Label_Sure)) {
            lv_obj_set_style_text_color(data->Label_Sure, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Sure, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
    }
    else if (code == LV_EVENT_KEY) {                  /* [我们] ESC 返回列表 / UP 回到分滚轮 */
        uint32_t key = lv_event_get_key(e);
        /* [我们] 勿 stop_processing（防 CANCEL 打到已删对象，坑 B3-3） */
        if (key == LV_KEY_ESC) { Set_BackToList(); return; }
        if (key == LV_KEY_UP && obj == data->Btn_Sure && lv_obj_is_valid(data->Roller_Min)) {
            lv_group_focus_obj(data->Roller_Min);
            lv_event_stop_processing(e);
        }
    }
    else if (code == LV_EVENT_CLICKED) {

        if (data->Btn_Sure == obj) {           //时间设置完成
            Set_Time_UnSel_Style(data);
            /* [我们] 真实时间设置：把草稿(年月日时分)写入 STM32 RTC（FC10+FC06）
             * 成功后顶栏由心跳(app_heartbeat)同步的 C 时钟自动刷新（1s 内） */
            int ret = app_heartbeat_commit();
            if (ret != MB_OK) {
                ESP_LOGW(TAG, "time commit failed: %s", app_modbus_err_str(ret));
            }
            Set_Page_Load(Page_Finish);        //跳转到设置完成页
            if (data->Timer == NULL) {
                data->Timer = lv_timer_create(Set_timer_cb, 100, NULL);
            }
        }
    }
}


/**
 * @brief 日期设置事件回调函数
 * @param e
 */
void SetPage_Date_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    Set_Widget_t* data = (Set_Widget_t*)lv_event_get_user_data(e);
    //lv_my_child_page_date_t* page_date = lv_obj_get_user_data(obj); // 获取页面数据

    if (code == LV_EVENT_FOCUSED) {
        /* [我们] lv_obj_is_valid 门控：本页对象 */
        if (lv_obj_is_valid(data->Btn_Page4Next)) Set_Date_UnSel_Style(data);
        if (data->Btn_Page4Next == obj && lv_obj_is_valid(data->Label_P4Next)) {
            lv_obj_set_style_text_color(data->Label_P4Next, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_Page4Next, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
        else if (data->Btn_DateBack == obj && lv_obj_is_valid(data->Label_DatBack)) {
            lv_obj_set_style_text_color(data->Label_DatBack, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
            lv_obj_set_style_bg_color(data->Btn_DateBack, lv_color_hex(FONT_BLUE_COLOR), LV_STATE_DEFAULT);
        }
    }
    else if (code == LV_EVENT_KEY) {                  /* [我们] UP/DOWN 在 返回/下一步 间切换 + ESC 返回列表 */
        uint32_t key = lv_event_get_key(e);
        /* [我们] 勿 stop_processing（防 CANCEL 打到已删对象，坑 B3-3） */
        if (key == LV_KEY_ESC) { Set_BackToList(); return; }
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
        /*Set_Date_UnSel_Style(data);*/
        if (data->Btn_Page4Next == obj) {
            //page_date->jump(gChild_SetPage_list[SetPage4].page_cont,
            //    gChild_SetPage_list[SetPage5].page_cont);        //跳转到设置日期页
            Set_Page_Load(Page_Time);
        }
        else if (data->Btn_DateBack == obj) {
            //page_date->jump(gChild_SetPage_list[SetPage4].page_cont,
            //    gChild_SetPage_list[SetPage0].page_cont);        //返回菜单页
            //lv_my_ui_pageSet_t.jump(g_page_list[Menu_Page]->page_cont);
            Set_BackToList();               // [我们] 返回设置列表（二级菜单）
        }
    }
}

/**
 * @brief 设置菜单列表事件回调函数
 * @param e 
 */
void SetPage_List_event_handler(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* obj = lv_event_get_target(e);
    Set_Widget_t* data = (Set_Widget_t*)lv_event_get_user_data(e);
  
    if (code == LV_EVENT_KEY) {                       /* [我们] UP/DOWN 切换 + ESC 返回主菜单 */
        uint32_t key = lv_event_get_key(e);
        /* [我们] 勿 stop_processing（防 CANCEL 打到已删对象，坑 B3-3） */
        if (key == LV_KEY_ESC) { Set_BackToMenu(); return; }
        int idx = -1;
        for (int i = 0; i < Btn_SUM; i++) if (obj == data->Btn[i]) { idx = i; break; }
        if (idx >= 0) {
            if (key == LV_KEY_DOWN) {
                lv_group_focus_obj(data->Btn[(idx + 1) % Btn_SUM]);
                lv_event_stop_processing(e);
            }
            else if (key == LV_KEY_UP) {
                lv_group_focus_obj(data->Btn[(idx + Btn_SUM - 1) % Btn_SUM]);
                lv_event_stop_processing(e);
            }
        }
    }
    else if (code == LV_EVENT_CLICKED) {
        if (data->Btn[Btn_Language] == obj) {
                  
            Set_Page_Load(Page_Language);           //跳转到语言选择页
        }
        else if (data->Btn[Btn_Volume] == obj) {
                    
            Set_Page_Load(Page_Volume);             //跳转到音量设置页
        }
        else if (data->Btn[Btn_Bright] == obj) {
                   
            Set_Page_Load(Page_Bright);              //跳转到亮度设置页
        }
        else if (data->Btn[Btn_Date] == obj) {
              
            Set_Page_Load(Page_Date);               //跳转到设置日期页
        } 
        else if (data->Btn[Btn_OTA] == obj) {

            Set_Page_Load(Page_OTA);               //跳转到设置日期页
        } 
    }
}
/**
 * @brief 设置音量滑动条界面未选中样式
 * @param data
 */
static void Set_SliVol_UnSel_Style(Set_Widget_t* data)
{
    lv_obj_remove_style(data->Slider_Volume, &style_BlueKnob, LV_PART_KNOB);
    lv_obj_set_style_text_color(data->Label_VolBack, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_VolBack, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);

}
/**
 * @brief 设置亮度滑动条界面未选中样式
 * @param data
 */
static void Set_SliBri_UnSel_Style(Set_Widget_t* data)
{
    lv_obj_remove_style(data->Slider_Bright, &style_BlueKnob, LV_PART_KNOB);
    lv_obj_set_style_text_color(data->Label_BriBack, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_BriBack, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
}
/**
 * @brief 设置选择语言界面未选中样式
 * @param data
 */
static void Set_SelLang_UnSel_Style(Set_Widget_t* data)
{
    lv_image_set_src(data->Knob_ChiSrc, KNOB_UNSEL_ICON);
    lv_image_set_src(data->Knob_EngSrc, KNOB_UNSEL_ICON);
    lv_obj_set_style_text_color(data->Label_P3Next, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_Page3Next, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
}
/**
 * @brief 时间界面未选中样式
 * @param data
 */
static void Set_Time_UnSel_Style(Set_Widget_t* data)
{
    lv_obj_set_style_text_color(data->Label_Sure, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_Sure, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);

}
/**
 * @brief 设置界面界面未选中样式
 * @param data
 */
static void Set_Date_UnSel_Style(Set_Widget_t* data)
{
    lv_obj_set_style_text_color(data->Label_P4Next, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_Page4Next, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(data->Label_DatBack, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_color(data->Btn_DateBack, lv_color_hex(FONT_WHITE_COLOR), LV_STATE_DEFAULT);
}

/**
 * @author zqt
 * @brief 设置时间滚轮事件回调
 * @param e
 */
/* [我们] 解析滚轮选中串（可含中文后缀 年月日时分/空格）→ 数字 */
static int set_parse_num(const char *s)
{
    int v = 0, started = 0;
    if (s == NULL) return 0;
    for (; *s != '\0'; s++)
    {
        if (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); started = 1; }
        else if (started) break;          /* 数字后遇后缀/分隔即停 */
    }
    return started ? v : 0;
}

/**
 * @author zqt
 * @brief 设置时间/日期滚轮值捕获：[我们] 解析选中项 → 更新对时草稿
 *        （年月日分页选择，时/分页选择；草稿跨页保存，确定时统一提交）
 * @param e
 */
void roller_event_cb(lv_event_t* e)
{
    lv_obj_t* obj;
    Set_Widget_t* data;
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
    data = (Set_Widget_t*)lv_event_get_user_data(e);
    if (data == NULL) return;

    app_heartbeat_get_pending(&t);

    if (obj == data->Roller_Year) {
        lv_roller_get_selected_str(data->Roller_Year, buf, sizeof(buf));
        t.year = set_parse_num(buf);
    } else if (obj == data->Roller_Month) {
        lv_roller_get_selected_str(data->Roller_Month, buf, sizeof(buf));
        t.month = set_parse_num(buf);
    } else if (obj == data->Roller_Day) {
        lv_roller_get_selected_str(data->Roller_Day, buf, sizeof(buf));
        t.day = set_parse_num(buf);
    } else if (obj == data->Roller_Hour) {
        lv_roller_get_selected_str(data->Roller_Hour, buf, sizeof(buf));
        t.hour = set_parse_num(buf);
    } else if (obj == data->Roller_Min) {
        lv_roller_get_selected_str(data->Roller_Min, buf, sizeof(buf));
        t.minute = set_parse_num(buf);
    } else {
        return;
    }

    app_heartbeat_set_pending(&t);
}
/* ===== [我们] 滚轮按键：ENTER 前进焦点（年→月→日→下一步 / 时→分→确定），ESC 返回 =====
 * UP/DOWN/LEFT/RIGHT 交给滚轮原生滚动（lv_roller 原生处理） */
void Set_Roller_Key_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_KEY) return;
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t key = lv_event_get_key(e);

    /* [我们] 返回设置列表；勿 stop_processing（防 CANCEL 打到已删对象，坑 B3-3） */
    if (key == LV_KEY_ESC) { Set_BackToList(); return; }
    if (key != LV_KEY_ENTER) return;

    lv_obj_t* target = NULL;
    if (obj == Set_Widget.Roller_Year)       target = Set_Widget.Roller_Month;
    else if (obj == Set_Widget.Roller_Month) target = Set_Widget.Roller_Day;
    else if (obj == Set_Widget.Roller_Day)   target = Set_Widget.Btn_Page4Next;
    else if (obj == Set_Widget.Roller_Hour)  target = Set_Widget.Roller_Min;
    else if (obj == Set_Widget.Roller_Min)   target = Set_Widget.Btn_Sure;
    if (target && lv_obj_is_valid(target)) {
        lv_group_focus_obj(target);
        lv_event_stop_processing(e);
    }
}

/* [我们] 无按钮页面的 ESC 处理（设置完成页，3s 定时器也会自动返回） */
void Set_Page_Esc_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_KEY && lv_event_get_key(e) == LV_KEY_ESC) {
        /* [我们] 勿 stop_processing（防 CANCEL 打到已删对象，坑 B3-3） */
        Set_BackToMenu();
    }
}