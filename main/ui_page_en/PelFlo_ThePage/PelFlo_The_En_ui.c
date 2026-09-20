#include "PelFlo_The_En_ui.h"
#include "PelFlo_The_En_ui_event.h"
#include "Child_ThePage_En.h"      /* Child_ThePage_En_ui / Child_ThePage_En_LeaveGroup */
#include "Child_MaiPage_En.h"      /* Child_MaiPage_En_ui / Child_MaiPage_En_LeaveGroup */
#include "Chlid_DysPage_En.h"      /* Child_DysPage_En_ui / Child_DysPage_En_LeaveGroup */
#include "Chlid_KgePage_En.h"      /* Child_KgePage_En_ui / Child_KgePage_En_LeaveGroup */
#include "menu_ui.h"               /* Show_StubPage / 滚动补偿坐标 extern */
#include "app_keypad.h"            /* [我们] keypad group 注册 */
#include "app_therapy.h"           /* [我们] 阶段C：切页安全网（停治疗引擎） */

static void PelFloThe_En_Menu_widget(The_En_Widget_t* ui, lv_obj_t* page_cont);

/* [我们] 切页前把 4 个英文四宫格按钮移出 keypad group（坑 6.8 同族）
 * 非 static：PelFlo_The_En_ui_event.c 的四宫格 ESC 回调也要用 */
void The_En_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g == NULL) return;
    if (lv_obj_is_valid(The_En_Widget.ZL_Btn))  lv_group_remove_obj(The_En_Widget.ZL_Btn);
    if (lv_obj_is_valid(The_En_Widget.BY_Btn))  lv_group_remove_obj(The_En_Widget.BY_Btn);
    if (lv_obj_is_valid(The_En_Widget.ZA_Btn))  lv_group_remove_obj(The_En_Widget.ZA_Btn);
    if (lv_obj_is_valid(The_En_Widget.KGE_Btn)) lv_group_remove_obj(The_En_Widget.KGE_Btn);
}

The_En_Widget_t The_En_Widget;

/**
 * @brief 盆底治疗页面创建入口（英文版）
 * @param page_id ThePage_ID_t（复用中文枚举，Get/Set_CurPage_Idx 同型）
 */
void PelFlo_ThePage_En_Load(ThePage_ID_t page_id)
{
    therapy_stop();              /* [我们] 阶段C：切页安全网——任何四宫格切换停治疗引擎 */
    The_En_LeaveGroup();          /* [我们] 四宫格按钮先移出 group（坑 6.8） */
    Child_ThePage_En_LeaveGroup();/* [我们] 治疗格子页对象先移出 group */
    Child_MaiPage_En_LeaveGroup();/* [我们] 保养格子页对象先移出 group */
    Child_DysPage_En_LeaveGroup();/* [我们] 障碍格子页对象先移出 group */
    Child_KgePage_En_LeaveGroup();/* [我们] 凯格尔格子页对象先移出 group */
    Page_Clean();
    switch (page_id) {
    case ThePage:
        Child_ThePage_En_ui();
        break;
    case MaiPage:
        Child_MaiPage_En_ui();
        break;
    case DysPage:
        Child_DysPage_En_ui();
        break;
    case KgePage:
        Child_KgePage_En_ui();
        break;
    case MenPage:
        PelFlo_The_En_ui();
        break;
    default:
        break;
    }
}

/**
 * @brief 主界面ui设计（英文版四宫格）
 * @param  none
 */
void PelFlo_The_En_ui(void)
{
    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, MAIN_BG_COLOR, 0);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(page_cont);

    lv_obj_clear_flag(page_cont, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    lv_obj_set_scrollbar_mode(page_cont, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/

    PelFloThe_En_Menu_widget(&The_En_Widget, page_cont);

    /* ===== [我们] 四宫格按钮注册 keypad group（默认焦点 = 治疗 ZL） ===== */
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        lv_group_add_obj(g, The_En_Widget.ZL_Btn);
        lv_group_add_obj(g, The_En_Widget.BY_Btn);
        lv_group_add_obj(g, The_En_Widget.ZA_Btn);
        lv_group_add_obj(g, The_En_Widget.KGE_Btn);
        lv_group_focus_obj(The_En_Widget.ZL_Btn);
    }
}

/**
 * @brief 盆底肌治疗菜单页 治疗、保养、功能障碍、凯格尔（英文文字/坐标对齐客户定稿）
 * @param  none
 */
static void PelFloThe_En_Menu_widget(The_En_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);

    /*Pelvic Floor Therapy 左上*/
    widget->ZL_Obj = Create_Obj(cont, 200, 110, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(widget->ZL_Obj, 21, 20);
    widget->ZL_Btn = Create_Button(widget->ZL_Obj, 100, 70, FONT_WHITE_COLOR, 10, 1);
    lv_obj_align(widget->ZL_Btn, LV_ALIGN_CENTER, 0, -15);
    widget->ZL_Img = Creat_Image(widget->ZL_Btn, ZL_unSel_ICON);
    lv_obj_center(widget->ZL_Img);
    lv_obj_add_event_cb(widget->ZL_Btn, ThePage_En_MenuBtn_event_cb, LV_EVENT_ALL, widget);
    lv_obj_t* Label1 = Creat_Label(widget->ZL_Obj, "Pelvic Floor Muscle Therapy", &lv_font_montserrat_14);
    lv_obj_align(Label1, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_text_color(Label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /*Pelvic Floor Maintenance 右上*/
    widget->BY_Obj = Create_Obj(cont, 200, 110, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(widget->BY_Obj, 244, 20);
    widget->BY_Btn = Create_Button(widget->BY_Obj, 100, 70, FONT_WHITE_COLOR, 10, 1);
    lv_obj_align(widget->BY_Btn, LV_ALIGN_CENTER, 0, -15);
    widget->BY_Img = Creat_Image(widget->BY_Btn, BY_unSel_ICON);
    lv_obj_center(widget->BY_Img);
    lv_obj_add_event_cb(widget->BY_Btn, ThePage_En_MenuBtn_event_cb, LV_EVENT_ALL, widget);
    lv_obj_t* Label2 = Creat_Label(widget->BY_Obj, "Pelvic Floor Maintenance", &lv_font_montserrat_14);
    lv_obj_align(Label2, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_text_color(Label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /*High-Tone Pelvic Floor Dysfunction 左下*/
    widget->ZA_Obj = Create_Obj(cont, 200, 110, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(widget->ZA_Obj, 21, 147);
    widget->ZA_Btn = Create_Button(widget->ZA_Obj, 100, 70, FONT_WHITE_COLOR, 10, 1);
    lv_obj_align(widget->ZA_Btn, LV_ALIGN_CENTER, 0, -15);
    widget->ZA_Img = Creat_Image(widget->ZA_Btn, ZA_unSel_ICON);
    lv_obj_center(widget->ZA_Img);
    lv_obj_add_event_cb(widget->ZA_Btn, ThePage_En_MenuBtn_event_cb, LV_EVENT_ALL, widget);
    lv_obj_t* Label3 = Creat_Label(widget->ZA_Obj, "High-Tone Pelvic Floor\nDysfunction", &lv_font_montserrat_14);
    lv_obj_set_width(Label3, 200);
    lv_obj_set_style_text_align(Label3, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(Label3, LV_ALIGN_BOTTOM_MID, 0, 17);
    lv_obj_set_style_text_color(Label3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

    /*Kegels 右下*/
    widget->KGE_Obj = Create_Obj(cont, 200, 110, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(widget->KGE_Obj, 244, 147);
    widget->KGE_Btn = Create_Button(widget->KGE_Obj, 100, 70, FONT_WHITE_COLOR, 10, 1);
    lv_obj_align(widget->KGE_Btn, LV_ALIGN_CENTER, 0, -15);
    widget->KGE_Img = Creat_Image(widget->KGE_Btn, KG_unSel_ICON);
    lv_obj_center(widget->KGE_Img);
    lv_obj_add_event_cb(widget->KGE_Btn, ThePage_En_MenuBtn_event_cb, LV_EVENT_ALL, widget);
    lv_obj_t* Label4 = Creat_Label(widget->KGE_Obj, "Kegels", &lv_font_montserrat_14);
    lv_obj_set_width(Label4, 200);
    lv_obj_set_style_text_align(Label4, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(Label4, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_text_color(Label4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);
}
