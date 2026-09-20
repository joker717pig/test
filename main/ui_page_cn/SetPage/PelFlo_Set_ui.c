#include "PelFlo_Set_ui.h"
#include "PelFlo_Set_ui_event_cb.h"
#include "basic.h"
#include "app_keypad.h"   /* [我们] keypad group 注册 */
#include "app_params.h"   /* [我们] 参数存储（NVS） */
#include "app_heartbeat.h"  /* [我们] 心跳（RTC 对时） */
#include "app_ota.h"      /* [我们] OTA 全局状态轮询 */
#include "app_wifi.h"     /* [我们] 进 OTA 页连 WiFi / 离开断开 */
#include <string.h>        /* [我们] memset 清零悬垂指针 */


static void Time_Roller(Set_Widget_t* widget, lv_obj_t* page_cont);
static void Date_Roller(Set_Widget_t* widget, lv_obj_t* page_cont);

static void PelFloSet_Page0_widget(Set_Widget_t* widget, lv_obj_t* page_cont);
static void PelFloSet_Page1_widget(Set_Widget_t* widget, lv_obj_t* page_cont);
static void PelFloSet_Page2_widget(Set_Widget_t* widget, lv_obj_t* page_cont);
static void PelFloSet_Page3_widget(Set_Widget_t* widget, lv_obj_t* page_cont);
static void PelFloSet_Page4_widget(Set_Widget_t* widget, lv_obj_t* page_cont);
static void PelFloSet_Page5_widget(Set_Widget_t* widget, lv_obj_t* page_cont);
static void PelFloSet_Page6_widget(Set_Widget_t* widget, lv_obj_t* page_cont);
static void PelFloSet_Page7_widget(OTA_Widget_t* widget, lv_obj_t* page_cont);

Set_Widget_t Set_Widget;
static lv_obj_t *s_page_esc_obj = NULL;   /* [我们] 当前无按钮页面的 ESC 焦点对象（Page6） */
OTA_Widget_t ota_data;

/* ===== [我们] 设置页资源清理（切页/返回前调用，防悬垂崩溃，坑 6.8 / B2-4 同族） ===== */
void Set_StopTimer(void)
{
    if (Set_Widget.Timer) { lv_timer_del(Set_Widget.Timer); Set_Widget.Timer = NULL; }
}

void Set_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g == NULL) return;
    /* [我们] lv_obj_is_valid 防悬垂：跨页/返回后旧页控件指针已失效（Page_Clean 后），
     * 直接 lv_group_remove_obj 悬垂指针 → use-after-free 崩溃（坑 B2-4） */
    for (int i = 0; i < Btn_SUM; i++)
    if (lv_obj_is_valid(Set_Widget.Btn[i])) lv_group_remove_obj(Set_Widget.Btn[i]);
    if (lv_obj_is_valid(Set_Widget.Btn_China))      lv_group_remove_obj(Set_Widget.Btn_China);
    if (lv_obj_is_valid(Set_Widget.Btn_English))    lv_group_remove_obj(Set_Widget.Btn_English);
    if (lv_obj_is_valid(Set_Widget.Btn_Page3Next))  lv_group_remove_obj(Set_Widget.Btn_Page3Next);
    if (lv_obj_is_valid(Set_Widget.Btn_Page4Next))  lv_group_remove_obj(Set_Widget.Btn_Page4Next);
    if (lv_obj_is_valid(Set_Widget.Roller_Year))    lv_group_remove_obj(Set_Widget.Roller_Year);
    if (lv_obj_is_valid(Set_Widget.Roller_Month))   lv_group_remove_obj(Set_Widget.Roller_Month);
    if (lv_obj_is_valid(Set_Widget.Roller_Day))     lv_group_remove_obj(Set_Widget.Roller_Day);
    if (lv_obj_is_valid(Set_Widget.Roller_Hour))    lv_group_remove_obj(Set_Widget.Roller_Hour);
    if (lv_obj_is_valid(Set_Widget.Roller_Min))     lv_group_remove_obj(Set_Widget.Roller_Min);
    if (lv_obj_is_valid(Set_Widget.Btn_Sure))       lv_group_remove_obj(Set_Widget.Btn_Sure);
    if (lv_obj_is_valid(Set_Widget.Slider_Volume))  lv_group_remove_obj(Set_Widget.Slider_Volume);
    if (lv_obj_is_valid(Set_Widget.Slider_Bright))  lv_group_remove_obj(Set_Widget.Slider_Bright);
    if (lv_obj_is_valid(Set_Widget.Btn_VolBack))    lv_group_remove_obj(Set_Widget.Btn_VolBack);
    if (lv_obj_is_valid(Set_Widget.Btn_BriBack))    lv_group_remove_obj(Set_Widget.Btn_BriBack);
    if (lv_obj_is_valid(Set_Widget.Btn_DateBack))   lv_group_remove_obj(Set_Widget.Btn_DateBack);
    if (s_page_esc_obj)                             { lv_group_remove_obj(s_page_esc_obj); s_page_esc_obj = NULL; }
}

void Set_BackToMenu(void)
{
    Set_StopTimer();
    Set_LeaveGroup();
    app_params_save_settings();   /* [我们] 参数存储：离开设置页前保存（Set_Widget 仍持有最新值） */
    Goto_MenuPage();
    memset(&Set_Widget, 0, sizeof(Set_Widget));  /* [我们] 清零，下次进入设置页从干净状态重建 */
}

/* [我们] 返回设置列表（二级菜单）：停定时器 + 清理 + 重建 Page0 */
void Set_BackToList(void)
{
    Set_StopTimer();
    /* [我们] 参数存储：离开子页前保存（Set_Page_Load 内会 memset 清掉 Set_Widget，必须先存） */
    app_params_save_settings();
    Set_Page_Load(Page_Menu);
}

/**
 * @brief 评估页子页面创建入口
 * @param page_id
 */
void Set_Page_Load(Set_PageID_t page_id)
{
    Set_LeaveGroup();           /* [我们] 先移出 group，防 Page_Clean 时 refocus FOCUSED 悬垂（坑 6.8） */
    Page_Clean();
    memset(&Set_Widget, 0, sizeof(Set_Widget));  /* [我们] 清零旧页控件指针，防悬垂指针地址复用被误判（坑 B2-4 族） */
    switch (page_id) {
    case Page_Volume:
        PelFloSet_Page1_widget(&Set_Widget, g_Ui.page_container);
        break;
    case Page_Bright:
        PelFloSet_Page2_widget(&Set_Widget, g_Ui.page_container);
        break;
    case Page_Language:
        PelFloSet_Page3_widget(&Set_Widget, g_Ui.page_container);
        break;
    case Page_Date:
        PelFloSet_Page4_widget(&Set_Widget, g_Ui.page_container);
        break;
    case Page_Time:
        PelFloSet_Page5_widget(&Set_Widget, g_Ui.page_container);
        break;
    case Page_OTA:
        PelFloSet_Page7_widget(&ota_data, g_Ui.page_container);
        break;
    case Page_Finish:
        PelFloSet_Page6_widget(&Set_Widget, g_Ui.page_container);
        break;
    case Page_Menu:
        PelFloSet_Page0_widget(&Set_Widget, g_Ui.page_container);
        break;
    default:
        break;
    }
}
/**
 * @brief 设置页设计
 * @param  none
 */
void PelFlo_Set_ui(void)
{
    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, FONT_WHITE_COLOR, 1);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(page_cont);
    //lv_obj_add_flag(page_cont, LV_OBJ_FLAG_HIDDEN);                     // 隐藏界面
    lv_obj_clear_flag(page_cont, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    lv_obj_set_scrollbar_mode(page_cont, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/

    PelFloSet_Page0_widget(&Set_Widget, page_cont);

}
void SetPage_SetBar_Value(lv_obj_t * bar,int32_t v)
{
    lv_bar_set_value(bar, v, LV_ANIM_OFF);
}

/**
 * @brief 设置页第7页 OTA升级页
 * @param  none
 */
static void PelFloSet_Page7_widget(OTA_Widget_t* widget, lv_obj_t* page_cont)
{
    memset(widget, 0, sizeof(*widget));   /* [我们] ota_data 独立全局，不被 Set_Page_Load 清零，重入先清旧指针/定时器 */
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);

    /*热点账号（与 app_wifi.h 中 APP_WIFI_SSID 一致）*/
    lv_obj_t* label = Creat_Label(cont, "账号：  EDA_EMG_OTA", basic_widget.Chinise_Font_OTA);
    lv_obj_set_pos(label, 50, 8);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*密码*/
    lv_obj_t* label2 = Creat_Label(cont, "密码：  12345678", basic_widget.Chinise_Font_OTA);
    lv_obj_set_pos(label2, 50, 40);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /* [我们] 联网状态标签（轮询刷新：联网中/成功/失败） */
    lv_obj_t* net = Creat_Label(cont, "联网中…", basic_widget.Chinise_Font_OTA);
    lv_obj_set_pos(net, 250, 40);
    lv_obj_set_style_text_color(net, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_Net = net;

    /* [我们] 主提示（检查中/有更新/已最新/升级中/完成/失败） */
    lv_obj_t* label3 = Creat_Label(cont, "正在检查更新…", basic_widget.Chinise_Font_OTA);
    lv_obj_set_pos(label3, 50, 78);
    lv_obj_set_style_text_color(label3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    widget->Label_OTA = label3;

    /*确认按钮（是/开始升级/重试；检查中先禁用，轮询按状态使能）*/
    lv_obj_t* Btn = Create_Button(cont, 96, 28, 0xffffff, 30, 0);
    lv_obj_set_pos(Btn, 330, 74);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn, SetPage_OTABtn_Event_cb, LV_EVENT_ALL, widget);
    lv_obj_add_state(Btn, LV_STATE_DISABLED);   /* LVGL9：禁用是 state；检查中先灰，轮询按状态使能 */
    widget->Btn_Sure = Btn;
    lv_obj_t* label4 = Creat_Label(Btn, "检查中…", basic_widget.Chinise_Font_OTA);
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
        { "STM32：", 120, &widget->Bar_STM32, &widget->Label_STM32, &widget->Label_Ver_STM32 },
        { "图片：",   154, &widget->Bar_Bin,   &widget->Label_Bin,   &widget->Label_Ver_Bin   },
        { "ESP32：",  188, &widget->Bar_ESP32, &widget->Label_ESP32, &widget->Label_Ver_ESP32 },
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t* nm = Creat_Label(cont, rows[i].name, basic_widget.Chinise_Font_OTA);
        lv_obj_set_pos(nm, 50, rows[i].y);
        lv_obj_set_style_text_color(nm, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

        lv_obj_t* bar = Create_Bar(cont, FONT_BLUE_COLOR, 100);
        lv_obj_set_pos(bar, 110, rows[i].y);
        lv_obj_set_size(bar, 200, 20);
        *rows[i].bar = bar;

        lv_obj_t* pct = Creat_Label(bar, "0%", basic_widget.Chinise_Font_OTA);
        lv_obj_center(pct);
        lv_obj_set_style_text_color(pct, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
        *rows[i].pct = pct;

        lv_obj_t* ver = Creat_Label(cont, "", basic_widget.Chinise_Font_OTA);
        lv_obj_set_pos(ver, 318, rows[i].y);
        lv_obj_set_style_text_color(ver, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
        *rows[i].ver = ver;
    }

    /*按钮返回（升级全成功后变"升级完成(点击重启)"）*/
    lv_obj_t* Btn2 = Create_Button(cont, 170, 30, 0xffffff, 30, 0);
    lv_obj_align(Btn2, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_set_style_border_width(Btn2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn2, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn2, SetPage_OTABtn_Event_cb, LV_EVENT_ALL, widget);
    widget->Btn_Back = Btn2;
    lv_obj_t* label11 = Creat_Label(Btn2, "返回", basic_widget.Chinise_Font_OTA);
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
    OTA_Page_Enter(widget);
}

/**
 * @brief 设置页第6页 设置完成页
 * @param  none
 */
static void PelFloSet_Page6_widget(Set_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
   lv_obj_center(cont);
   
    /*图标*/
    lv_obj_t* Img = Creat_Image(cont, SETFINISH_ICON);
    lv_obj_set_pos(Img, 172, 60);
    /*标题*/
    lv_obj_t* title1 = Creat_Label(cont, "设置完成，进入系统中...", basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(title1, 130, 200);
    lv_obj_set_style_text_color(title1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /* ===== [我们] 设置完成：注册 cont 承接 ESC（也可等 3s 自动返回） ===== */
    lv_group_t *g6 = app_keypad_get_group();
    if (g6) { lv_group_add_obj(g6, cont); lv_group_focus_obj(cont); s_page_esc_obj = cont; }
    lv_obj_add_event_cb(cont, Set_Page_Esc_cb, LV_EVENT_KEY, NULL);
}
/**
 * @brief 设置页第5页 时间设置页
 * @param  none
 */
static void PelFloSet_Page5_widget(Set_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);

    Time_Roller(widget, cont);

    /*按钮返回*/
    lv_obj_t* Btn = Create_Button(cont, 338, 48, 0xffffff, 30, 0);
    lv_obj_align(Btn, LV_ALIGN_BOTTOM_MID, 0, -25);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(0x6786f1), LV_PART_MAIN);
    widget->Btn_Sure = Btn;
    //标签    
    lv_obj_t* label = Creat_Label(Btn, "确定", basic_widget.Chinise_Font_Btn);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_Sure = label;
    lv_obj_add_event_cb(Btn, SetPage_Time_event_cb, LV_EVENT_ALL, widget);

    /* ===== [我们] 时间页：时/分滚轮 + 确定 注册 group，默认聚焦时滚轮 ===== */
    lv_group_t *g5 = app_keypad_get_group();
    if (g5) {
        lv_group_add_obj(g5, widget->Roller_Hour);
        lv_group_add_obj(g5, widget->Roller_Min);
        lv_group_add_obj(g5, widget->Btn_Sure);
        lv_group_focus_obj(widget->Roller_Hour);
    }
    /* [我们] 滚轮：ENTER 前进焦点 / ESC 返回 */
    lv_obj_add_event_cb(widget->Roller_Hour, Set_Roller_Key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(widget->Roller_Min,  Set_Roller_Key_cb, LV_EVENT_KEY, NULL);
}

/**
 * @brief 设置页第4页 日期设置页
 * @param  none
 */
static void PelFloSet_Page4_widget(Set_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    
    /* lv_obj_set_style_bg_color(cont, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);*/
   /* lv_my_SetPage4_t.page_cont = cont;*/

    Date_Roller(widget, cont);

    /*按钮返回*/
    lv_obj_t* Btn = Create_Button(cont, 159, 48, 0xffffff, 30, 0);
    lv_obj_set_pos(Btn,60,200);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(0x6786f1), LV_PART_MAIN);
    widget->Btn_DateBack = Btn;
    lv_obj_add_event_cb(Btn, SetPage_Date_event_cb, LV_EVENT_ALL, widget);
    //标签    
    lv_obj_t* label = Creat_Label(Btn, "返回", basic_widget.Chinise_Font_Btn);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_DatBack = label;
    /*按钮下一步*/
    lv_obj_t* Btn2 = Create_Button(cont, 159, 48, 0xffffff, 30, 0);
    lv_obj_set_pos(Btn2, 239, 200);
    lv_obj_set_style_border_width(Btn2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn2, lv_color_hex(0x6786f1), LV_PART_MAIN);
    widget->Btn_Page4Next = Btn2;
    //标签    
    lv_obj_t* label2 = Creat_Label(Btn2, "下一步", basic_widget.Chinise_Font_Btn);
    lv_obj_center(label2);
    widget->Label_P4Next = label2;
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);

    /*lv_obj_set_user_data(Btn2, gChild_SetPage_list[SetPage4]);*/
    lv_obj_add_event_cb(Btn2, SetPage_Date_event_cb, LV_EVENT_ALL, widget);

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
    /* [我们] 滚轮：ENTER 前进焦点 / ESC 返回（UP/DOWN/LEFT/RIGHT 原生滚动） */
    lv_obj_add_event_cb(widget->Roller_Year,  Set_Roller_Key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(widget->Roller_Month, Set_Roller_Key_cb, LV_EVENT_KEY, NULL);
    lv_obj_add_event_cb(widget->Roller_Day,   Set_Roller_Key_cb, LV_EVENT_KEY, NULL);
}

/**
 * @brief 设置页第3页 语言选择页
 * @param  none
 */
static void PelFloSet_Page3_widget(Set_Widget_t* widget, lv_obj_t* page_cont)
{
    
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
  

    /*按钮中文*/
    lv_obj_t* Btn = Create_Button(cont, 340, 68, 0xf6f6f6, 15, 0);
    lv_obj_align(Btn, LV_ALIGN_CENTER, 0, -80);
    lv_obj_set_style_shadow_opa(Btn, 0, LV_PART_MAIN);
    /*lv_obj_add_style(Btn, &style_gradient, LV_PART_MAIN);*/
    /*lv_obj_add_style(Btn, &style_shadow, LV_PART_MAIN);*/
    widget->Btn_China = Btn;
    /*点击图标*/
    lv_obj_t* knob = Creat_Image(Btn, KNOB_UNSEL_ICON);
    lv_obj_align(knob, LV_ALIGN_RIGHT_MID, -10, 0);
    widget->Knob_ChiSrc = knob;
    //标签    
    lv_obj_t* label = Creat_Label(Btn, "中文", basic_widget.Chinese_Font_30);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_BLACK_COLOR), LV_PART_MAIN);

    lv_obj_add_event_cb(Btn, SetPage_Select_Language_event_cb, LV_EVENT_ALL, widget);

    /*按钮英文 */
    lv_obj_t* Btn1 = Create_Button(cont, 340, 68, 0xf6f6f6, 15, 0);
    lv_obj_align(Btn1, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_shadow_opa(Btn1, 0, LV_PART_MAIN);
    /*lv_obj_add_style(Btn1, &style_gradient, LV_PART_MAIN);*/
    /*lv_obj_add_style(Btn, &style_shadow, LV_PART_MAIN);*/
    widget->Btn_English = Btn1;
    /*点击图标*/
    lv_obj_t* knob1 = Creat_Image(Btn1, KNOB_UNSEL_ICON);
    lv_obj_align(knob1, LV_ALIGN_RIGHT_MID, -10, 0);
    widget->Knob_EngSrc = knob1;
    //标签    
    lv_obj_t* label1 = Creat_Label(Btn1, "ENGLISH", basic_widget.Chinese_Font_30);
    lv_obj_center(label1);
    lv_obj_set_style_text_color(label1, lv_color_hex(FONT_BLACK_COLOR), LV_PART_MAIN);

    lv_obj_add_event_cb(Btn1, SetPage_Select_Language_event_cb, LV_EVENT_ALL, widget);

    /*按钮下一步 */
    lv_obj_t* Btn2 = Create_Button(cont, 338, 48, 0x6786f1, 20, 0);
    lv_obj_align(Btn2, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_border_width(Btn2, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn2, lv_color_hex(0x6786f1), LV_PART_MAIN);
    widget->Btn_Page3Next = Btn2;
    //标签    
    lv_obj_t* label2 = Creat_Label(Btn2, "下一步", basic_widget.Chinise_Font_Btn);
    lv_obj_center(label2);
    widget->Label_P3Next = label2;
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_WHITE_COLOR), LV_PART_MAIN);
    /*lv_obj_set_user_data(Btn2, gChild_SetPage_list[SetPage3]);*/
    lv_obj_add_event_cb(Btn2, SetPage_Select_Language_event_cb, LV_EVENT_ALL, widget);

    /* ===== [我们] 语言页：中文/English/下一步 注册 group，默认聚焦中文 ===== */
    lv_group_t *g3 = app_keypad_get_group();
    if (g3) {
        lv_group_add_obj(g3, widget->Btn_China);
        lv_group_add_obj(g3, widget->Btn_English);
        lv_group_add_obj(g3, widget->Btn_Page3Next);
        lv_group_focus_obj(widget->Btn_China);
    }
}
/**
 * @brief 设置页第2页 亮度设置页
 * @param  none
 */
static void PelFloSet_Page2_widget(Set_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
  

    //标签    
    lv_obj_t* label0 = Creat_Label(cont, "5", basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(label0, 390, 119);
    lv_obj_set_style_text_color(label0, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    //标签    
    lv_obj_t* label1 = Creat_Label(cont, "0", basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(label1, 40, 119);
    lv_obj_set_style_text_color(label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    //标签    
    lv_obj_t* label2 = Creat_Label(cont, "亮度调节", basic_widget.Chinise_Font_Title);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, -60);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    lv_obj_t* Obj4 = Create_Obj(cont, 300, 4, 0xebecf0, 1);
    lv_obj_set_pos(Obj4, 70, 124);
    /*创建滑动条*/
    lv_obj_t* slider = Create_Sliders(cont, FONT_BLUE_COLOR, 5);
    lv_obj_set_pos(slider, 70, 120);
    widget->Slider_Bright = slider;
    /* [我们] 右端数字实时显示滑条值 */
    widget->Label_BriValue = label0;
    lv_label_set_text_fmt(label0, "%d", (int)lv_slider_get_value(slider));
    lv_obj_add_event_cb(widget->Slider_Bright, SetPage_Slider_Event_cb, LV_EVENT_ALL, widget);    //将加按钮添加任何事件回调
    /* [我们] 参数存储：滑条初值 = NVS 已存亮度。
     * ⚠️ LVGL9.5 lv_slider_set_value(ANIM_OFF) 只移滑条不发 VALUE_CHANGED（lv_bar.c 确认），
     * 必须手动同步 Set_Widget.Value_Bright + 右侧数字 */
    widget->Value_Bright = app_params_get_bright();
    lv_slider_set_value(widget->Slider_Bright, widget->Value_Bright, LV_ANIM_OFF);
    lv_label_set_text_fmt(widget->Label_BriValue, "%d", (int)widget->Value_Bright);

    /*按钮返回*/
    lv_obj_t* Btn = Create_Button(cont, 164, 48, 0xffffff, 30, 0);
    lv_obj_align(Btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(0x6786f1), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn, SetPage_Slider_Event_cb, LV_EVENT_ALL, widget);
    widget->Btn_BriBack = Btn;

    //标签    
    lv_obj_t* label = Creat_Label(Btn, "返回", basic_widget.Chinise_Font_Btn);
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
 * @brief 设置页第1页 音量设置页
 * @param  none
 */
static void PelFloSet_Page1_widget(Set_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);


    //标签    
    lv_obj_t* label0 = Creat_Label(cont, "5", basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(label0, 390, 119);
    lv_obj_set_style_text_color(label0, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    //标签    
    lv_obj_t* label1 = Creat_Label(cont, "0", basic_widget.Chinise_Font_Title);
    lv_obj_set_pos(label1, 40, 119);
    lv_obj_set_style_text_color(label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
   
    //标签    
    lv_obj_t* label2 = Creat_Label(cont, "音量调节", basic_widget.Chinise_Font_Title);
    lv_obj_align(label2, LV_ALIGN_CENTER, 0, -60);
    lv_obj_set_style_text_color(label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    lv_obj_t* Obj4 = Create_Obj(cont, 300, 4, 0xebecf0, 1);
    lv_obj_set_pos(Obj4, 70, 124);
    /*创建滑动条*/
    lv_obj_t* slider = Create_Sliders(cont, FONT_BLUE_COLOR, 5);
    lv_obj_set_pos(slider, 70, 120);
    widget->Slider_Volume = slider;
    /* [我们] 右端数字实时显示滑条值 */
    widget->Label_VolValue = label0;
    lv_label_set_text_fmt(label0, "%d", (int)lv_slider_get_value(slider));
    lv_obj_add_event_cb(widget->Slider_Volume, SetPage_Slider_Event_cb, LV_EVENT_ALL, widget);    //将加按钮添加任何事件回调
    /* [我们] 参数存储：滑条初值 = NVS 已存音量。
     * ⚠️ LVGL9.5 lv_slider_set_value(ANIM_OFF) 只移滑条不发 VALUE_CHANGED（lv_bar.c 确认），
     * 必须手动同步 Set_Widget.Value_Volume + 右侧数字（否则滑条位置对但数值仍显示初值 1） */
    widget->Value_Volume = app_params_get_volume();
    lv_slider_set_value(widget->Slider_Volume, widget->Value_Volume, LV_ANIM_OFF);
    lv_label_set_text_fmt(widget->Label_VolValue, "%d", (int)widget->Value_Volume);

    /*按钮返回*/
    lv_obj_t* Btn = Create_Button(cont, 164, 48, 0xffffff, 30, 0);
    lv_obj_align(Btn, LV_ALIGN_BOTTOM_MID, 0, -40);
    lv_obj_set_style_border_width(Btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(Btn, lv_color_hex(0x6786f1), LV_PART_MAIN);
    lv_obj_add_event_cb(Btn, SetPage_Slider_Event_cb, LV_EVENT_ALL, widget);
    widget->Btn_VolBack = Btn;

    //标签    
    lv_obj_t* label = Creat_Label(Btn, "返回", basic_widget.Chinise_Font_Btn);
    lv_obj_center(label);
    lv_obj_set_style_text_color(label, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);
    widget->Label_VolBack = label;

    /* ===== [我们] 音量页：滑条 + 返回按钮注册 group，默认聚焦滑条（LEFT/RIGHT 原生调值） ===== */
    lv_group_t *g1 = app_keypad_get_group();
    if (g1) {
        lv_group_add_obj(g1, widget->Slider_Volume);
        lv_group_add_obj(g1, widget->Btn_VolBack);
        lv_group_focus_obj(widget->Slider_Volume);
    }
}
/**
 * @brief 设置页第0页
 * @param  none
 */
static void PelFloSet_Page0_widget(Set_Widget_t* widget, lv_obj_t* page_cont)
{
    const char* items[] = { "        音量调节", "        亮度调节",
                            "        语言/language", "        设置日期时间","        OTA升级" };

    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
    
    /* lv_obj_set_style_bg_color(cont, lv_color_hex(FONT_BLUE_COLOR), LV_PART_MAIN);*/

    /* ===== 选中状态样式 ===== */
    static lv_style_t style_sel;
    lv_style_init(&style_sel);
    lv_style_set_bg_color(&style_sel, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_text_color(&style_sel, lv_color_white());

    static lv_style_t style_btn;
    lv_style_init(&style_btn);
    lv_style_set_border_width(&style_btn, 0);
    lv_style_set_outline_width(&style_btn, 0);
    lv_style_set_pad_row(&style_btn, 12);
    lv_style_set_text_color(&style_btn, lv_color_hex(FONT_GRAY_COLOR));

    //创建菜单列表
    lv_obj_t* list1 = lv_list_create(cont);
    lv_obj_set_size(list1, 480, 230);
    lv_obj_center(list1);
    lv_obj_add_style(list1, &style_btn, LV_STATE_DEFAULT);
    //添加按钮
    for (int i = 0; i < 5; i++) {
        lv_obj_t* btn = lv_list_add_button(list1, NULL, items[i]);
        lv_obj_set_style_text_font(btn, basic_widget.Chinise_Font_Btn, LV_PART_MAIN);
        lv_obj_add_style(btn, &style_btn, LV_STATE_DEFAULT);
        lv_obj_add_style(btn, &style_sel, LV_STATE_FOCUSED | LV_PART_MAIN);
        lv_obj_add_event_cb(btn, SetPage_List_event_handler, LV_EVENT_ALL, widget);
        widget->Btn[i] = btn;
        
    }

    /* ===== [我们] 4 个列表按钮注册到 keypad group（UP/DOWN 切换 + ENTER 进入 + ESC 返回） ===== */
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        for (int i = 0; i < 5; i++) lv_group_add_obj(g, widget->Btn[i]);
        lv_group_focus_obj(widget->Btn[Btn_Volume]);
    }
}

/* [我们] 修正：0~23 两位补零（原 1~24 无法选 0 时、含非法 24 点） */
const char* Hour = "00时\n01时\n02时\n03时\n04时\n05时\n06时\n07时\n08时\n09时\n10时\n11时\n12时\n13时"
                   "\n14时\n15时\n16时\n17时\n18时\n19时\n20时\n21时\n22时\n23时";
/* [我们] 修正：0~59 两位补零（原含 140分/58缺分/60分 等笔误） */
const char* Minu = "00分\n01分\n02分\n03分\n04分\n05分\n06分\n07分\n08分\n09分\n10分\n11分\n12分\n13分\n14分\n15分"
                  "\n16分\n17分\n18分\n19分\n20分\n21分\n22分\n23分\n24分\n25分\n26分\n27分\n28分\n29分\n30分\n31分"
                  "\n32分\n33分\n34分\n35分\n36分\n37分\n38分\n39分\n40分\n41分\n42分\n43分\n44分\n45分\n46分\n47分"
                  "\n48分\n49分\n50分\n51分\n52分\n53分\n54分\n55分\n56分\n57分\n58分\n59分";
const char* Year = "2026年\n2027年\n2028年\n2029年\n2030年\n2031年\n2032年\n2033年\n2034年\n";
const char* Month = "01月\n02月\n03月\n04月\n05月\n06月\n07月\n08月\n09月\n10月\n11月\n12月";
const char* day =   "01日\n02日\n03日\n04日\n05日\n06日\n07日\n08日\n09日\n10日\n11日\n12日\n13日\n14日\n15日\n16日"
                    "\n17日\n18日\n19日\n20日\n21日\n22日\n23日\n24日\n25日\n26日\n27日\n28日\n29日\n30日\n31日";
const char* list = "音量调节\n亮度调节\n语言/language\n设置日期时间\n";
/**
 * @brief 创建基本滚轮
 * @param page_cont 
 * @param w 
 * @param h 
 * @param option 
 * @param space 
 * @return 
 */
static lv_obj_t* Set_Create_Roller(lv_obj_t* page_cont, int32_t w, int32_t h, const char* option, int32_t space)
{
    /*默认时的样式*/
    static lv_style_t style;
    lv_style_init(&style);
   /* lv_style_set_bg_color(&style, lv_color_black());*/
    lv_style_set_text_color(&style, lv_color_hex(FONT_GRAY_COLOR));
    lv_style_set_border_width(&style, 0);
    lv_style_set_radius(&style, 0);
    lv_style_set_text_line_space(&style, 26);

    static lv_style_t style2;
    lv_style_init(&style2);
    /*lv_style_set_bg_color(&style2, lv_color_hex(0x181D2F));*/
    lv_style_set_radius(&style2, 3);
    lv_style_set_text_font(&style2, basic_widget.Chinise_Font_Title);
    lv_style_set_text_color(&style2, lv_color_hex(FONT_BLACK_COLOR));
    lv_style_set_bg_opa(&style2, 0);

    lv_obj_t* roller = lv_roller_create(page_cont);
    lv_obj_set_size(roller, w, h);
    lv_obj_add_style(roller, &style, 0);
    lv_obj_add_style(roller, &style2, LV_PART_SELECTED);
    /*lv_obj_set_style_bg_opa(roller, LV_OPA_50, LV_PART_SELECTED);*/
    lv_obj_set_style_bg_opa(roller, 100, LV_PART_MAIN);
    lv_roller_set_options(roller, option, LV_ROLLER_MODE_INFINITE);
    lv_obj_set_style_text_font(roller, basic_widget.Chinise_Font_Title, LV_PART_MAIN);
    lv_obj_set_style_radius(roller, 0, LV_PART_MAIN);
    lv_roller_set_selected(roller, 3, LV_ANIM_ON);
  /*  lv_roller_set_visible_row_count(roller, 4);*/
    /*lv_obj_set_style_text_line_space(roller,25, LV_PART_MAIN);*/
    return roller;
}
/* [我们] 滚轮初值 = 对时草稿（首次=当前 RTC 时间；未读到用默认值）
 * 年滚轮 2026 起共 9 项；月/日/时/分按选项表索引 = 值-1 / 值 */
static void Set_Roller_SyncFromPending(Set_Widget_t* widget)
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
 * @brief 时间选择滚轮
 * @param widget 
 * @param page_cont 
 */
static void Time_Roller(Set_Widget_t* widget, lv_obj_t* page_cont)
{
    static lv_point_precise_t line[] = { {0, 0},{0, 20} };
    /*小时滚轮*/
    lv_obj_t* hour_roller1 = Set_Create_Roller(page_cont, 100, 180, Hour, 40);
    lv_obj_set_pos(hour_roller1, 100, 16);
    lv_obj_add_event_cb(hour_roller1, roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Hour= hour_roller1;
    //竖线
    lv_obj_t* line1 = creat_line(page_cont, 1, 33, line, 2);
    lv_obj_set_pos(line1, 225, 95);
    lv_obj_set_style_line_color(line1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*分钟滚轮*/
    lv_obj_t* minu_roller1 = Set_Create_Roller(page_cont, 100, 180, Minu, 40);
    lv_obj_set_pos(minu_roller1, 250, 16);
    lv_obj_add_event_cb(minu_roller1, roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Min = minu_roller1;

    /* [我们] 初值 = 对时草稿（首次=当前 RTC 时间） */
    Set_Roller_SyncFromPending(widget);
 

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
 * @brief 日期选择滚轮
 * @param widget 
 * @param page_cont 
 */
static void Date_Roller(Set_Widget_t* widget, lv_obj_t* page_cont)
{
    static lv_point_precise_t line[] = { {0, 0},{0, 20} };
    /*年 滚轮*/
    lv_obj_t* year_roller1 = Set_Create_Roller(page_cont, 100, 180, Year, 10);
    lv_obj_set_pos(year_roller1, 60, 16);
    lv_obj_add_event_cb(year_roller1, roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Year = year_roller1;
    //竖线
    lv_obj_t* line1 = creat_line(page_cont, 1, 33, line, 2);
    lv_obj_set_pos(line1, 170, 95);
    lv_obj_set_style_line_color(line1, lv_color_hex(FONT_GRAY_COLOR),LV_PART_MAIN);

    /*月 滚轮*/
    lv_obj_t* month_roller1 = Set_Create_Roller(page_cont, 100, 180, Month, 20);
    lv_obj_set_pos(month_roller1, 175, 16);
    lv_obj_add_event_cb(month_roller1, roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Month = month_roller1;
    //竖线
    lv_obj_t* line2 = creat_line(page_cont, 1, 33, line, 2);
    lv_obj_set_pos(line2, 280, 95);
    lv_obj_set_style_line_color(line2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
    /*日 滚轮*/
    lv_obj_t* day_roller1 = Set_Create_Roller(page_cont, 100, 180, day, 20);
    lv_obj_set_pos(day_roller1, 290, 16);
    lv_obj_add_event_cb(day_roller1, roller_event_cb, LV_EVENT_ALL, widget);
    widget->Roller_Day = day_roller1;

    /* [我们] 初值 = 对时草稿（首次=当前 RTC 时间） */
    Set_Roller_SyncFromPending(widget);
    /*lv_obj_add_event_cb(year_roller1, roller_event_cb, LV_EVENT_VALUE_CHANGED, NULL);*/
    /*顶部模糊图层*/
    lv_obj_t* Obj2 = Create_Obj(page_cont, 480, 30, 0x2195f6, 0);
    lv_obj_set_pos(Obj2,0, 16);
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

