/**
  ******************************************************************************
  * @文件名称   set_En_ui.c
  * @文件描述   英文设置页（两套 UI，阶段 E2；复制中文 SetPage 直译英文草稿）
  *            布局沿用中文 SetPage；符号统一 *_En 前缀防链接冲突。
  *            参数存储接入与中文一致（音量/亮度滑条同步 app_params + 离开页保存）。
  ******************************************************************************
  */

#include "set_En_ui.h"
#include "set_En_ui_event.h"
#include "basic.h"
#include "app_keypad.h"   /* keypad group 注册 */
#include "app_params.h"   /* 参数存储（NVS） */
#include "app_heartbeat.h"  /*  心跳（RTC 对时） */
#include <string.h>       /*  memset 清零悬垂指针 */
#include "app_ota.h"
#include "app_wifi.h"

static uint16_t page_open(void);
static uint16_t page_close(void);
static uint16_t page_jump(lv_obj_t* page);
static uint16_t ChildPage_jump(lv_obj_t* mysPage, lv_obj_t* dirPage);

static void Set_En_Time_Roller(Set_En_Widget_t* widget, lv_obj_t* page_cont);
static void Set_En_Date_Roller(Set_En_Widget_t* widget, lv_obj_t* page_cont);

static void Set_En_Page0_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont);
static void Set_En_Page1_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont);
static void Set_En_Page2_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont);
static void Set_En_Page3_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont);
static void Set_En_Page4_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont);
static void Set_En_Page5_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont);
static void Set_En_Page6_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont);
static void Set_En_Page7_widget(OTA_Widget_EN_t* widget, lv_obj_t* page_cont);

Set_En_Widget_t Set_En_Widget;

OTA_Widget_EN_t ota_data_en;

static lv_obj_t *s_en_page_esc_obj = NULL;   /* [我们] 当前无按钮页面的 ESC 焦点对象（Page6） */

/* ===== [我们] 英文设置页资源清理（切页/返回前调用，防悬垂崩溃，坑 6.8 / B2-4 同族） ===== */
void Set_En_StopTimer(void)
{
    if (Set_En_Widget.Timer) { lv_timer_del(Set_En_Widget.Timer); Set_En_Widget.Timer = NULL; }
}

void Set_En_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g == NULL) return;
    for (int i = 0; i < En_Btn_SUM; i++)
        if (lv_obj_is_valid(Set_En_Widget.Btn[i])) lv_group_remove_obj(Set_En_Widget.Btn[i]);
    if (lv_obj_is_valid(Set_En_Widget.Btn_China))      lv_group_remove_obj(Set_En_Widget.Btn_China);
    if (lv_obj_is_valid(Set_En_Widget.Btn_English))    lv_group_remove_obj(Set_En_Widget.Btn_English);
    if (lv_obj_is_valid(Set_En_Widget.Btn_Page3Next))  lv_group_remove_obj(Set_En_Widget.Btn_Page3Next);
    if (lv_obj_is_valid(Set_En_Widget.Btn_Page4Next))  lv_group_remove_obj(Set_En_Widget.Btn_Page4Next);
    if (lv_obj_is_valid(Set_En_Widget.Roller_Year))    lv_group_remove_obj(Set_En_Widget.Roller_Year);
    if (lv_obj_is_valid(Set_En_Widget.Roller_Month))   lv_group_remove_obj(Set_En_Widget.Roller_Month);
    if (lv_obj_is_valid(Set_En_Widget.Roller_Day))     lv_group_remove_obj(Set_En_Widget.Roller_Day);
    if (lv_obj_is_valid(Set_En_Widget.Roller_Hour))    lv_group_remove_obj(Set_En_Widget.Roller_Hour);
    if (lv_obj_is_valid(Set_En_Widget.Roller_Min))     lv_group_remove_obj(Set_En_Widget.Roller_Min);
    if (lv_obj_is_valid(Set_En_Widget.Btn_Sure))       lv_group_remove_obj(Set_En_Widget.Btn_Sure);
    if (lv_obj_is_valid(Set_En_Widget.Slider_Volume))  lv_group_remove_obj(Set_En_Widget.Slider_Volume);
    if (lv_obj_is_valid(Set_En_Widget.Slider_Bright))  lv_group_remove_obj(Set_En_Widget.Slider_Bright);
    if (lv_obj_is_valid(Set_En_Widget.Btn_VolBack))    lv_group_remove_obj(Set_En_Widget.Btn_VolBack);
    if (lv_obj_is_valid(Set_En_Widget.Btn_BriBack))    lv_group_remove_obj(Set_En_Widget.Btn_BriBack);
    if (lv_obj_is_valid(Set_En_Widget.Btn_DateBack))   lv_group_remove_obj(Set_En_Widget.Btn_DateBack);
    if (lv_obj_is_valid(ota_data_en.Btn_Sure))         lv_group_remove_obj(ota_data_en.Btn_Sure);
    if (lv_obj_is_valid(ota_data_en.Btn_Back))         lv_group_remove_obj(ota_data_en.Btn_Back);
    if (s_en_page_esc_obj)                             { lv_group_remove_obj(s_en_page_esc_obj); s_en_page_esc_obj = NULL; }
}

void Set_En_BackToMenu(void)
{
    Set_En_StopTimer();
    Set_En_LeaveGroup();
    app_params_save_settings();   /* [我们] 参数存储：离开设置页前保存 */
    Goto_MenuPage();              /* [我们] 按当前语言回对应菜单（英文→英文菜单） */
    memset(&Set_En_Widget, 0, sizeof(Set_En_Widget));
    memset(&ota_data_en, 0, sizeof(ota_data_en));
}

/* [我们] 返回设置列表（二级菜单）：停定时器 + 清理 + 重建 Page0 */
void Set_En_BackToList(void)
{
    Set_En_StopTimer();
    if (ota_data_en.Timer)
    {
        lv_timer_del(ota_data_en.Timer);
        ota_data_en.Timer = NULL;
    }
    app_params_save_settings();   /* [我们] 参数存储：离开子页前保存（memset 前） */
    Set_En_Page_Load(En_Page_Menu);
}

/**
 * @brief 评估页子页面创建入口
 * @param page_id
 */
void Set_En_Page_Load(Set_En_PageID_t page_id)
{
    Set_En_LeaveGroup();           /* [我们] 先移出 group，防 Page_Clean 时 refocus FOCUSED 悬垂（坑 6.8） */
    Page_Clean();
    memset(&Set_En_Widget, 0, sizeof(Set_En_Widget));  /* [我们] 清零旧页控件指针（坑 B2-4 族） */
    switch (page_id) {
    case En_Page_Volume:
        Set_En_Page1_widget(&Set_En_Widget, g_Ui.page_container);
        break;
    case En_Page_Bright:
        Set_En_Page2_widget(&Set_En_Widget, g_Ui.page_container);
        break;
    case En_Page_Language:
        Set_En_Page3_widget(&Set_En_Widget, g_Ui.page_container);
        break;
    case En_Page_Date:
        Set_En_Page4_widget(&Set_En_Widget, g_Ui.page_container);
        break;
    case En_Page_Time:
        Set_En_Page5_widget(&Set_En_Widget, g_Ui.page_container);
        break;
    case En_Page_OTA:
        Set_En_Page7_widget(&ota_data_en, g_Ui.page_container);
        break;
    case En_Page_Finish:
        Set_En_Page6_widget(&Set_En_Widget, g_Ui.page_container);
        break;
    case En_Page_Menu:
    default:
        Set_En_Page0_widget(&Set_En_Widget, g_Ui.page_container);
        break;
    }
}

/**
 * @brief 设置页设计（英文）
 * @param  none
 */
void Set_En_ui(void)
{
    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(page_cont);
    lv_obj_clear_flag(page_cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(page_cont, LV_SCROLLBAR_MODE_OFF);

    Set_En_Page0_widget(&Set_En_Widget, page_cont);
}
/**
 * @brief 设置页第7页 OTA升级页（英文）
 */
static void Set_En_Page7_widget(OTA_Widget_EN_t* widget, lv_obj_t* page_cont)
{
    if (widget->Timer){lv_timer_del(widget->Timer);widget->Timer = NULL; }
    memset(widget, 0, sizeof(*widget));   /* [我们] ota_data 独立全局，不被 Set_Page_Load 清零，重入先清旧指针/定时器 */
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);

    /*热点账号（与 app_wifi.h 中 APP_WIFI_SSID 一致）*/
    lv_obj_t* label = Creat_Label(cont, "SSID: EDA_EMG_OTA", &lv_font_montserrat_16);
    lv_obj_set_pos(label, 50, 8);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*密码*/
    lv_obj_t* label2 = Creat_Label(cont, "Password: 12345678", &lv_font_montserrat_16);
    lv_obj_set_pos(label2, 50, 40);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /* [我们] 联网状态标签（轮询刷新：联网中/成功/失败） */
    lv_obj_t* net = Creat_Label(cont, "Connecting...", &lv_font_montserrat_16);
    lv_obj_set_pos(net, 250, 40);
    lv_obj_set_style_text_color(net, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_Net = net;

    /* [我们] 主提示（检查中/有更新/已最新/升级中/完成/失败） */
    lv_obj_t* label3 = Creat_Label(cont, "Checking Update...", &lv_font_montserrat_16);
    lv_obj_set_pos(label3, 50, 78);
    lv_obj_set_style_text_color(label3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Label_OTA = label3;

    /*确认按钮（是/开始升级/重试；检查中先禁用，轮询按状态使能）*/
    lv_obj_t* Btn = Create_Button(cont, 120, 28, 0xffffff, 30, 0);
    lv_obj_set_pos(Btn, 330, 74);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn, Set_En_OTABtn_Event_cb, LV_EVENT_ALL, widget);
    lv_obj_add_state(Btn, LV_STATE_DISABLED);   /* LVGL9：禁用是 state；检查中先灰，轮询按状态使能 */
    widget->Btn_Sure = Btn;
    lv_obj_t* label4 = Creat_Label(Btn, "Checking...", &lv_font_montserrat_16);
    lv_obj_center(label4);
    lv_obj_set_style_text_color(label4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Label_Sure = label4;

    /* [我们] 三目标行：名称 + 进度条(条内百分比) + 右侧版本号 */
    struct {
        char       *name;   /* Creat_Label 形参为 char*（字符串字面量在 C 中可隐式转 char*） */
        int         y;
        lv_obj_t  **bar;
        lv_obj_t  **pct;
        lv_obj_t  **ver;
    } rows[3] = {
        { "STM32:", 120, &widget->Bar_STM32, &widget->Label_STM32, &widget->Label_Ver_STM32 },
        { "Image:",   154, &widget->Bar_Bin,   &widget->Label_Bin,   &widget->Label_Ver_Bin   },
        { "ESP32:",  188, &widget->Bar_ESP32, &widget->Label_ESP32, &widget->Label_Ver_ESP32 },
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t* nm = Creat_Label(cont, rows[i].name, &lv_font_montserrat_16);
        lv_obj_set_pos(nm, 50, rows[i].y);
        lv_obj_set_style_text_color(nm, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

        lv_obj_t* bar = Create_Bar(cont, FONT_BLUE_COLOR, 100);
        lv_obj_set_pos(bar, 110, rows[i].y);
        lv_obj_set_size(bar, 200, 20);
        *rows[i].bar = bar;

        lv_obj_t* pct = Creat_Label(bar, "0%", &lv_font_montserrat_16);
        lv_obj_center(pct);
        lv_obj_set_style_text_color(pct, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
        *rows[i].pct = pct;

        lv_obj_t* ver = Creat_Label(cont, "", &lv_font_montserrat_16);
        lv_obj_set_pos(ver, 318, rows[i].y);
        lv_obj_set_style_text_color(ver, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        *rows[i].ver = ver;
    }

    /*按钮返回（升级全成功后变"升级完成(点击重启)"）*/
    lv_obj_t* Btn2 = Create_Button(cont, 160, 30, 0xffffff, 30, 0);
    lv_obj_align(Btn2, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_border_width(Btn2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn2, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn2, Set_En_OTABtn_Event_cb, LV_EVENT_ALL, widget);
    widget->Btn_Back = Btn2;
    lv_obj_t* label11 = Creat_Label(Btn2, "Back", &lv_font_montserrat_16);
    lv_obj_center(label11);
    lv_obj_set_style_text_color(label11, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_Back = label11;

    lv_group_t *g1 = app_keypad_get_group();
    if (g1) {
        lv_group_add_obj(g1, widget->Btn_Back);
        lv_group_add_obj(g1, widget->Btn_Sure);
        lv_group_focus_obj(widget->Btn_Back);   /* 检查中 Sure 禁用，焦点先放返回 */
    }

    /* [我们] 进页面即发起检查：复位状态 → 投 BEGIN_CHECK（连 WiFi + 拉清单比对）→ 启动轮询 */
    OTA_En_Page_Enter(widget);
}
/**
 * @brief 设置页第6页 设置完成页（英文）
 */
static void Set_En_Page6_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    /*图标*/
    lv_obj_t* Img = Creat_Image(cont, EN_SETFINISH_ICON);
    lv_obj_set_pos(Img, 172, 60);
    /*标题*/
    lv_obj_t* title1 = Creat_Label(cont, "Setup completed,entering the system ..", &lv_font_montserrat_20);
    lv_obj_set_pos(title1, 40, 200);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /* ===== [我们] 设置完成：注册 cont 承接 ESC ===== */
    lv_group_t *g6 = app_keypad_get_group();
    if (g6) { lv_group_add_obj(g6, cont); lv_group_focus_obj(cont); s_en_page_esc_obj = cont; }
    lv_obj_add_event_cb(cont, Set_En_Page_Esc_cb, LV_EVENT_KEY, NULL);
}

/**
 * @brief 设置页第5页 时间设置页（英文）
 */
static void Set_En_Page5_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    Set_En_Time_Roller(widget, cont);

    /*按钮返回(OK)*/
    lv_obj_t* Btn = Create_Button(cont, 338, 48, 0xffffff, 30, 0);
    lv_obj_align(Btn, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(0x6786f1), LV_PART_MAIN);
    widget->Btn_Sure = Btn;
    /*标签*/
    lv_obj_t* label = Creat_Label(Btn, "C o n f i r m", &lv_font_montserrat_14);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_Sure = label;
    lv_obj_add_event_cb(Btn, Set_En_Time_event_cb, LV_EVENT_ALL, widget);

    /* ===== [我们] 时间页：时/分滚轮 + OK 注册 group，默认聚焦时滚轮 ===== */
    lv_group_t *g5 = app_keypad_get_group();
    if (g5) {
        lv_group_add_obj(g5, widget->Roller_Hour);
        lv_group_add_obj(g5, widget->Roller_Min);
        lv_group_add_obj(g5, widget->Btn_Sure);
        lv_group_focus_obj(widget->Roller_Hour);
    }
    lv_obj_add_event_cb(widget->Roller_Hour, Set_En_Roller_Key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(widget->Roller_Min,  Set_En_Roller_Key_cb, LV_EVENT_KEY, NULL);
}

/**
 * @brief 设置页第4页 日期设置页（英文）
 */
static void Set_En_Page4_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    Set_En_Date_Roller(widget, cont);

    /*按钮返回*/
    lv_obj_t* Btn = Create_Button(cont, 159, 48, 0xffffff, 30, 0);
    lv_obj_set_pos(Btn, 60, 200);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(0x6786f1), LV_PART_MAIN);
    widget->Btn_DateBack = Btn;
    lv_obj_add_event_cb(Btn, Set_En_Date_event_cb, LV_EVENT_ALL, widget);
    /*标签*/
    lv_obj_t* label = Creat_Label(Btn, "B a c k", &lv_font_montserrat_14);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_DatBack = label;
    /*按钮下一步*/
    lv_obj_t* Btn2 = Create_Button(cont, 159, 48, 0xffffff, 30, 0);
    lv_obj_set_pos(Btn2, 239, 200);
    lv_obj_set_style_border_width(Btn2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn2, lv_color_hex(0x6786f1), LV_PART_MAIN);
    widget->Btn_Page4Next = Btn2;
    /*标签*/
    lv_obj_t* label2 = Creat_Label(Btn2, "N e x t", &lv_font_montserrat_14);
    lv_obj_center(label2);
    widget->Label_P4Next = label2;
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn2, Set_En_Date_event_cb, LV_EVENT_ALL, widget);

    /* ===== [我们] 日期页：3 滚轮 + 返回/下一步 注册 group，默认聚焦年份滚轮 ===== */
    lv_group_t *g4 = app_keypad_get_group();
    if (g4) {
        lv_group_add_obj(g4, widget->Roller_Year);
        lv_group_add_obj(g4, widget->Roller_Month);
        lv_group_add_obj(g4, widget->Roller_Day);
        lv_group_add_obj(g4, widget->Btn_Page4Next);
        lv_group_add_obj(g4, widget->Btn_DateBack);
        lv_group_focus_obj(widget->Roller_Year);
    }
    lv_obj_add_event_cb(widget->Roller_Year,  Set_En_Roller_Key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(widget->Roller_Month, Set_En_Roller_Key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(widget->Roller_Day,   Set_En_Roller_Key_cb, LV_EVENT_KEY, NULL);
}

/**
 * @brief 设置页第3页 语言选择页（英文）
 */
static void Set_En_Page3_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    /*按钮中文*/
    lv_obj_t* Btn = Create_Button(cont, 340, 68, 0xf6f6f6, 15, 0);
    lv_obj_align(Btn, LV_ALIGN_CENTER, 0, -80);
    lv_obj_set_style_shadow_opa(Btn, 0, LV_PART_MAIN);
    widget->Btn_China = Btn;
    lv_obj_t* knob = Creat_Image(Btn, EN_KNOB_UNSEL_ICON);
    lv_obj_align(knob, LV_ALIGN_RIGHT_MID, -10, 0);
    widget->Knob_ChiSrc = knob;
    lv_obj_t* label = Creat_Label(Btn, "中文", basic_widget.Chinese_Font_30);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_BLACK_COLOR), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn, Set_En_Select_Language_event_cb, LV_EVENT_ALL, widget);

    /*按钮英文*/
    lv_obj_t* Btn1 = Create_Button(cont, 340, 68, 0xf6f6f6, 15, 0);
    lv_obj_align(Btn1, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_shadow_opa(Btn1, 0, LV_PART_MAIN);
    widget->Btn_English = Btn1;
    lv_obj_t* knob1 = Creat_Image(Btn1, EN_KNOB_UNSEL_ICON);
    lv_obj_align(knob1, LV_ALIGN_RIGHT_MID, -10, 0);
    widget->Knob_EngSrc = knob1;
    lv_obj_t* label1 = Creat_Label(Btn1, "ENGLISH", &lv_font_montserrat_30);
    lv_obj_center(label1);
    lv_obj_set_style_text_color(label1, lv_color_hex(FONT_BLACK_COLOR), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn1, Set_En_Select_Language_event_cb, LV_EVENT_ALL, widget);

    /*按钮下一步*/
    lv_obj_t* Btn2 = Create_Button(cont, 338, 48, 0x6786f1, 20, 0);
    lv_obj_align(Btn2, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_border_width(Btn2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn2, lv_color_hex(0x6786f1), LV_PART_MAIN);
    widget->Btn_Page3Next = Btn2;
    lv_obj_t* label2 = Creat_Label(Btn2, "N e x t", &lv_font_montserrat_14);
    lv_obj_center(label2);
    widget->Label_P3Next = label2;
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn2, Set_En_Select_Language_event_cb, LV_EVENT_ALL, widget);

    /* ===== [我们] 语言页：Chinese/English/Next 注册 group，默认聚焦 Chinese ===== */
    lv_group_t *g3 = app_keypad_get_group();
    if (g3) {
        lv_group_add_obj(g3, widget->Btn_China);
        lv_group_add_obj(g3, widget->Btn_English);
        lv_group_add_obj(g3, widget->Btn_Page3Next);
        lv_group_focus_obj(widget->Btn_China);
    }
}

/**
 * @brief 设置页第2页 亮度设置页（英文）
 */
static void Set_En_Page2_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    /*标签 5*/
    lv_obj_t* label0 = Creat_Label(cont, "5", &lv_font_montserrat_18);
    lv_obj_set_pos(label0, 390, 119);
    lv_obj_set_style_text_color(label0, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*标签 0*/
    lv_obj_t* label1 = Creat_Label(cont, "0", &lv_font_montserrat_18);
    lv_obj_set_pos(label1, 40, 119);
    lv_obj_set_style_text_color(label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*标签 标题*/
    lv_obj_t* label2 = Creat_Label(cont, "Brightness", &lv_font_montserrat_20);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, -60);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    lv_obj_t* Obj4 = Create_Obj(cont, 300, 4, 0xebecf0, 1);
    lv_obj_set_pos(Obj4, 70, 124);
    /*创建滑动条*/
    lv_obj_t* slider = Create_Sliders(cont, FONT_BLUE_COLOR, 5);
    lv_obj_set_pos(slider, 70, 120);
    widget->Slider_Bright = slider;
    widget->Label_BriValue = label0;
    lv_label_set_text_fmt(label0, "%d", (int)lv_slider_get_value(slider));
    lv_obj_add_event_cb(widget->Slider_Bright, Set_En_Slider_Event_cb, LV_EVENT_ALL, widget);
    /* [我们] 参数存储：滑条初值 = NVS 已存亮度（LVGL9.5 set_value 不发事件，手动同步） */
    widget->Value_Bright = app_params_get_bright();
    lv_slider_set_value(widget->Slider_Bright, widget->Value_Bright, LV_ANIM_OFF);
    lv_label_set_text_fmt(widget->Label_BriValue, "%d", (int)widget->Value_Bright);

    /*按钮返回*/
    lv_obj_t* Btn = Create_Button(cont, 164, 48, 0xffffff, 30, 0);
    lv_obj_align(Btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(0x6786f1), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn, Set_En_Slider_Event_cb, LV_EVENT_ALL, widget);
    widget->Btn_BriBack = Btn;
    lv_obj_t* label = Creat_Label(Btn, "B a c k", &lv_font_montserrat_14);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_BriBack = label;

    /* ===== [我们] 亮度页：滑条 + 返回按钮注册 group，默认聚焦滑条 ===== */
    lv_group_t *g2 = app_keypad_get_group();
    if (g2) {
        lv_group_add_obj(g2, widget->Slider_Bright);
        lv_group_add_obj(g2, widget->Btn_BriBack);
        lv_group_focus_obj(widget->Slider_Bright);
    }
}

/**
 * @brief 设置页第1页 音量设置页（英文）
 */
static void Set_En_Page1_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    /*标签 5*/
    lv_obj_t* label0 = Creat_Label(cont, "5", &lv_font_montserrat_18);
    lv_obj_set_pos(label0, 390, 119);
    lv_obj_set_style_text_color(label0, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*标签 0*/
    lv_obj_t* label1 = Creat_Label(cont, "0", &lv_font_montserrat_18);
    lv_obj_set_pos(label1, 40, 119);
    lv_obj_set_style_text_color(label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*标签 标题*/
    lv_obj_t* label2 = Creat_Label(cont, "Volume", &lv_font_montserrat_20);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, -60);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    lv_obj_t* Obj4 = Create_Obj(cont, 300, 4, 0xebecf0, 1);
    lv_obj_set_pos(Obj4, 70, 124);
    /*创建滑动条*/
    lv_obj_t* slider = Create_Sliders(cont, FONT_BLUE_COLOR, 5);
    lv_obj_set_pos(slider, 70, 120);
    widget->Slider_Volume = slider;
    widget->Label_VolValue = label0;
    lv_label_set_text_fmt(label0, "%d", (int)lv_slider_get_value(slider));
    lv_obj_add_event_cb(widget->Slider_Volume, Set_En_Slider_Event_cb, LV_EVENT_ALL, widget);
    /* [我们] 参数存储：滑条初值 = NVS 已存音量（LVGL9.5 set_value 不发事件，手动同步） */
    widget->Value_Volume = app_params_get_volume();
    lv_slider_set_value(widget->Slider_Volume, widget->Value_Volume, LV_ANIM_OFF);
    lv_label_set_text_fmt(widget->Label_VolValue, "%d", (int)widget->Value_Volume);

    /*按钮返回*/
    lv_obj_t* Btn = Create_Button(cont, 164, 48, 0xffffff, 30, 0);
    lv_obj_align(Btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(0x6786f1), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn, Set_En_Slider_Event_cb, LV_EVENT_ALL, widget);
    widget->Btn_VolBack = Btn;
    lv_obj_t* label = Creat_Label(Btn, "B a c k", &lv_font_montserrat_14);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_VolBack = label;

    /* ===== [我们] 音量页：滑条 + 返回按钮注册 group，默认聚焦滑条 ===== */
    lv_group_t *g1 = app_keypad_get_group();
    if (g1) {
        lv_group_add_obj(g1, widget->Slider_Volume);
        lv_group_add_obj(g1, widget->Btn_VolBack);
        lv_group_focus_obj(widget->Slider_Volume);
    }
}

/**
 * @brief 设置页第0页 列表页（英文）
 */
static void Set_En_Page0_widget(Set_En_Widget_t* widget, lv_obj_t* page_cont)
{
    const char* items[] = {
    "Volume adjustment",
    "Brightness adjustment",
    "Language/",
    "Date & Time Settings",
    "OTA Upgrade"
    };

    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);
    lv_obj_center(cont);

    /* ===== 选中状态样式 ===== */
    static lv_style_t style_sel;
    lv_style_init(&style_sel);
    lv_style_set_bg_color(&style_sel, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_text_color(&style_sel, lv_color_white());

    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_border_width(&style_btn, 0);
    lv_style_set_outline_width(&style_btn, 0);
    lv_style_set_pad_row(&style_btn, 0);
    lv_style_set_text_color(&style_btn, lv_color_hex(FONT_GRAY_COLOR));

    /*创建菜单列表*/
    lv_obj_t* list1 = lv_list_create(cont);
    lv_obj_set_size(list1, 480, 230);
    lv_obj_center(list1);
    lv_obj_add_style(list1, &style_btn, LV_STATE_DEFAULT);
    /*添加按钮*/
    for (int i = 0; i < 5; i++) {
        lv_obj_t* btn = lv_list_add_button(list1, NULL, items[i]);
        lv_obj_set_height(btn, 44);
        lv_obj_set_style_pad_ver(btn, 0, LV_PART_MAIN);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_add_style(btn, &style_btn, LV_STATE_DEFAULT);
        lv_obj_add_style(btn, &style_sel, LV_STATE_FOCUSED | LV_PART_MAIN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        if (i == 2) {
            lv_obj_t* en_label = lv_obj_get_child(btn, 0);
            lv_obj_set_flex_grow(en_label, 0);
            lv_obj_set_style_text_font(en_label, &lv_font_montserrat_18, LV_PART_MAIN);

            /* 创建中文字 */
            lv_obj_t* zh_label = lv_label_create(btn);
            lv_label_set_text(zh_label, "语言");
            lv_obj_set_style_text_font(zh_label, basic_widget.Chinise_Font_Btn, LV_PART_MAIN);
            lv_obj_set_style_text_color(zh_label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
            lv_obj_set_style_pad_column(btn, 3, LV_PART_MAIN);
        }
        lv_obj_add_event_cb(btn, Set_En_List_event_handler, LV_EVENT_ALL, widget);
        widget->Btn[i] = btn;
    }

    /* ===== [我们] 5 个列表按钮注册到 keypad group ===== */
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        for (int i = 0; i < 5; i++) lv_group_add_obj(g, widget->Btn[i]);
        lv_group_focus_obj(widget->Btn[En_Btn_Volume]);
    }
}

/* ==================== 日期/时间滚轮（英文：纯数字，去掉"年月日时分"后缀） ==================== */
/* [我们] 修正：0~23 / 0~59 两位补零（原 1~24 / 1~60 无法选 0、含非法 24/60） */
const char* En_Hour = "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13"
                      "\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23";
const char* En_Minu = "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15"
                      "\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31"
                      "\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47"
                      "\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59";
const char* En_Year = "2026\n2027\n2028\n2029\n2030\n2031\n2032\n2033\n2034";
const char* En_Month = "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12";
const char* En_Day =   "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12\n13\n14\n15\n16"
                       "\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31";

/**
 * @brief 创建基本滚轮（英文设置页，样式同中文 Set_Create_Roller）
 */
static lv_obj_t* Set_En_Create_Roller(lv_obj_t* page_cont, int32_t w, int32_t h, const char* option, int32_t space)
{
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_color(&style, lv_color_hex(FONT_GRAY_COLOR));
    lv_style_set_border_width(&style, 0);
    lv_style_set_radius(&style, 0);
    lv_style_set_text_line_space(&style, 26);

    static lv_style_t style2;
    lv_style_init(&style2);
    lv_style_set_radius(&style2, 3);
    lv_style_set_text_font(&style2, &lv_font_montserrat_18);
    lv_style_set_text_color(&style2, lv_color_hex(FONT_BLACK_COLOR));
    lv_style_set_bg_opa(&style2, 0);

    lv_obj_t* roller = lv_roller_create(page_cont);
    lv_obj_set_size(roller, w, h);
    lv_obj_add_style(roller, &style, 0);
    lv_obj_add_style(roller, &style2, LV_PART_SELECTED);
    lv_obj_set_style_bg_opa(roller, 100, LV_PART_MAIN);
    lv_roller_set_options(roller, option, LV_ROLLER_MODE_INFINITE);
    lv_obj_set_style_text_font(roller, &lv_font_montserrat_18, LV_PART_MAIN);
    lv_obj_set_style_radius(roller, 0, LV_PART_MAIN);
    lv_roller_set_selected(roller, 3, LV_ANIM_ON);
    return roller;
}

/* [我们] 滚轮初值 = 对时草稿（首次=当前 RTC 时间；未读到用默认值）
 * 年滚轮 2026 起共 9 项；月/日/时/分按选项表索引 = 值-1 / 值 */
static void Set_En_Roller_SyncFromPending(Set_En_Widget_t* widget)
{
    app_rtc_t t;
    int idx;

    app_heartbeat_get_pending(&t);

    idx = t.year - 2026; if (idx < 0) idx = 0; else if (idx > 8) idx = 8;
    if (widget->Roller_Year)  lv_roller_set_selected(widget->Roller_Year,  (uint32_t)idx, LV_ANIM_OFF);
    idx = t.month - 1; if (idx < 0) idx = 0; else if (idx > 11) idx = 11;
    if (widget->Roller_Month) lv_roller_set_selected(widget->Roller_Month, (uint32_t)idx, LV_ANIM_OFF);
    idx = t.day - 1; if (idx < 0) idx = 0; else if (idx > 30) idx = 30;
    if (widget->Roller_Day)   lv_roller_set_selected(widget->Roller_Day,   (uint32_t)idx, LV_ANIM_OFF);
    idx = t.hour; if (idx < 0) idx = 0; else if (idx > 23) idx = 23;
    if (widget->Roller_Hour)  lv_roller_set_selected(widget->Roller_Hour,  (uint32_t)idx, LV_ANIM_OFF);
    idx = t.minute; if (idx < 0) idx = 0; else if (idx > 59) idx = 59;
    if (widget->Roller_Min)   lv_roller_set_selected(widget->Roller_Min,   (uint32_t)idx, LV_ANIM_OFF);
}

/**
 * @brief 时间选择滚轮（英文）
 */
static void Set_En_Time_Roller(Set_En_Widget_t* widget, lv_obj_t* page_cont)
{
    static lv_point_precise_t line[] = { {0, 0},{0, 20} };
    /*小时滚轮*/
    lv_obj_t* hour_roller1 = Set_En_Create_Roller(page_cont, 100, 180, En_Hour, 40);
    lv_obj_set_pos(hour_roller1, 100, 16);
    lv_obj_add_event_cb(hour_roller1, Set_En_Roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Hour = hour_roller1;
    /*竖线*/
    lv_obj_t* line1 = creat_line(page_cont, 1, 33, line, 2);
    lv_obj_set_pos(line1, 225, 95);
    lv_obj_set_style_line_color(line1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*分钟滚轮*/
    lv_obj_t* minu_roller1 = Set_En_Create_Roller(page_cont, 100, 180, En_Minu, 40);
    lv_obj_set_pos(minu_roller1, 250, 16);
    lv_obj_add_event_cb(minu_roller1, Set_En_Roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Min = minu_roller1;

    /* [我们] 初值 = 对时草稿（首次=当前 RTC 时间） */
    Set_En_Roller_SyncFromPending(widget);

    /*顶部模糊图层*/
    lv_obj_t* Obj2 = Create_Obj(page_cont, 480, 30, 0x2195f6, 0);
    lv_obj_set_pos(Obj2, 0, 16);
    lv_obj_set_style_border_width(Obj2, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Obj2, 255, LV_PART_MAIN);
    lv_obj_add_style(Obj2, &style_BlurShadow, LV_PART_MAIN);
    /*底部模糊图层*/
    lv_obj_t* Obj3 = Create_Obj(page_cont, 480, 30, 0x2195f6, 0);
    lv_obj_set_pos(Obj3, 0, 200);
    lv_obj_set_style_border_width(Obj3, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Obj3, 255, LV_PART_MAIN);
    lv_obj_add_style(Obj3, &style_BlurShadow, LV_PART_MAIN);
}

/**
 * @brief 日期选择滚轮（英文）
 */
static void Set_En_Date_Roller(Set_En_Widget_t* widget, lv_obj_t* page_cont)
{
    static lv_point_precise_t line[] = { {0, 0},{0, 20} };
    /*年 滚轮*/
    lv_obj_t* year_roller1 = Set_En_Create_Roller(page_cont, 100, 180, En_Year, 10);
    lv_obj_set_pos(year_roller1, 60, 16);
    lv_obj_add_event_cb(year_roller1, Set_En_Roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Year = year_roller1;
    /*竖线*/
    lv_obj_t* line1 = creat_line(page_cont, 1, 33, line, 2);
    lv_obj_set_pos(line1, 170, 95);
    lv_obj_set_style_line_color(line1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /*月 滚轮*/
    lv_obj_t* month_roller1 = Set_En_Create_Roller(page_cont, 100, 180, En_Month, 20);
    lv_obj_set_pos(month_roller1, 175, 16);
    lv_obj_add_event_cb(month_roller1, Set_En_Roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Month = month_roller1;
    /*竖线*/
    lv_obj_t* line2 = creat_line(page_cont, 1, 33, line, 2);
    lv_obj_set_pos(line2, 280, 95);
    lv_obj_set_style_line_color(line2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*日 滚轮*/
    lv_obj_t* day_roller1 = Set_En_Create_Roller(page_cont, 100, 180, En_Day, 20);
    lv_obj_set_pos(day_roller1, 290, 16);
    lv_obj_add_event_cb(day_roller1, Set_En_Roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Day = day_roller1;

    /* [我们] 初值 = 对时草稿（首次=当前 RTC 时间） */
    Set_En_Roller_SyncFromPending(widget);

    /*顶部模糊图层*/
    lv_obj_t* Obj2 = Create_Obj(page_cont, 480, 30, 0x2195f6, 0);
    lv_obj_set_pos(Obj2, 0, 16);
    lv_obj_set_style_border_width(Obj2, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Obj2, 255, LV_PART_MAIN);
    lv_obj_add_style(Obj2, &style_BlurShadow, LV_PART_MAIN);
    /*底部模糊图层*/
    lv_obj_t* Obj3 = Create_Obj(page_cont, 480, 30, 0x2195f6, 0);
    lv_obj_set_pos(Obj3, 0, 200);
    lv_obj_set_style_border_width(Obj3, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(Obj3, 255, LV_PART_MAIN);
    lv_obj_add_style(Obj3, &style_BlurShadow, LV_PART_MAIN);
}
