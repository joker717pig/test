/**
  ******************************************************************************
  * @文件名称   menu_En_ui.c
  * @文件描述   英文主菜单（两套独立 UI 架构验证，阶段 E2）
  *            布局参考用户提供的 Menu_widget_EN；事件/导航与中文菜单一致
  *            （FOCUSED 高亮 + KEY 2D 导航；CLICKED 暂不跳转，等英文页面）。
  *            命名：*.c/.h 统一 *_En，符号统一 Menu_En_*（防与中文链接冲突）。
  ******************************************************************************
  */

#include "menu_En_ui.h"
#include "set_En_ui.h"   /* 英文设置页（Setting 跳转） */
#include "PosReh_En_ui.h"   /*  英文产后康复页（Postnatal Rehabilitation 跳转） */
#include "Ass_En_ui.h"      /* 英文盆底评估页（Pelvic Floor Assessment 跳转） */
#include "PelFlo_The_En_ui.h"   /* [我们] 英文盆底治疗页（Pelvic Floor Therapy 跳转） */
#include "app_keypad.h"   /* keypad group 注册 */
#include "esp_log.h"

static const char *TAG = "menu_en";

Menu_En_Widget_t Menu_En_Widget;

/* 隐藏全部选中效果图片（lv_obj_is_valid 防悬垂，坑 B2-4 族） */
static void Menu_En_Set_UnSel(void)
{
    if (lv_obj_is_valid(Menu_En_Widget.Ass_seleckImg))
        lv_obj_add_flag(Menu_En_Widget.Ass_seleckImg, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(Menu_En_Widget.The_seleckImg))
        lv_obj_add_flag(Menu_En_Widget.The_seleckImg, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(Menu_En_Widget.PostRec_seleckImg))
        lv_obj_add_flag(Menu_En_Widget.PostRec_seleckImg, LV_OBJ_FLAG_HIDDEN);
    if (lv_obj_is_valid(Menu_En_Widget.Set_seleckImg))
        lv_obj_add_flag(Menu_En_Widget.Set_seleckImg, LV_OBJ_FLAG_HIDDEN);
}

/* 切页前把 4 个英文菜单按钮移出 keypad group（坑 6.8 通用守则） */
void Menu_En_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g == NULL) return;
    if (lv_obj_is_valid(Menu_En_Widget.PelFlo_Ass_Btn)) lv_group_remove_obj(Menu_En_Widget.PelFlo_Ass_Btn);
    if (lv_obj_is_valid(Menu_En_Widget.PelFlo_The_Btn)) lv_group_remove_obj(Menu_En_Widget.PelFlo_The_Btn);
    if (lv_obj_is_valid(Menu_En_Widget.PostRec_Btn))    lv_group_remove_obj(Menu_En_Widget.PostRec_Btn);
    if (lv_obj_is_valid(Menu_En_Widget.Set_Btn))        lv_group_remove_obj(Menu_En_Widget.Set_Btn);
}

/**
 * @brief 英文菜单按钮事件回调（FOCUSED 高亮 + KEY 2D 导航 + CLICKED 预留）
 *        与中文 Menu_button_event_cb 逻辑一致；CLICKED 暂不跳转（英文页面未做）
 */
void Menu_En_button_event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    Menu_En_Widget_t* user_data = (Menu_En_Widget_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {
        Menu_En_Set_UnSel();
        if (obj == user_data->PelFlo_Ass_Btn && lv_obj_is_valid(user_data->Ass_seleckImg))
            lv_obj_remove_flag(user_data->Ass_seleckImg, LV_OBJ_FLAG_HIDDEN);
        else if (obj == user_data->PelFlo_The_Btn && lv_obj_is_valid(user_data->The_seleckImg))
            lv_obj_remove_flag(user_data->The_seleckImg, LV_OBJ_FLAG_HIDDEN);
        else if (obj == user_data->PostRec_Btn && lv_obj_is_valid(user_data->PostRec_seleckImg))
            lv_obj_remove_flag(user_data->PostRec_seleckImg, LV_OBJ_FLAG_HIDDEN);
        else if (obj == user_data->Set_Btn && lv_obj_is_valid(user_data->Set_seleckImg))
            lv_obj_remove_flag(user_data->Set_seleckImg, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_KEY) {                  /* 2D 导航（与中文菜单一致） */
        uint32_t key = lv_event_get_key(e);
        lv_obj_t* target = NULL;
        if (obj == user_data->PelFlo_Ass_Btn) {           /* 左上 */
            if (key == LV_KEY_RIGHT) target = user_data->PelFlo_The_Btn;
            else if (key == LV_KEY_DOWN) target = user_data->PostRec_Btn;
        }
        else if (obj == user_data->PelFlo_The_Btn) {      /* 右上 */
            if (key == LV_KEY_LEFT) target = user_data->PelFlo_Ass_Btn;
            else if (key == LV_KEY_DOWN) target = user_data->Set_Btn;
        }
        else if (obj == user_data->PostRec_Btn) {         /* 左下 */
            if (key == LV_KEY_RIGHT) target = user_data->Set_Btn;
            else if (key == LV_KEY_UP) target = user_data->PelFlo_Ass_Btn;
        }
        else if (obj == user_data->Set_Btn) {             /* 右下 */
            if (key == LV_KEY_LEFT) target = user_data->PostRec_Btn;
            else if (key == LV_KEY_UP) target = user_data->PelFlo_The_Btn;
        }
        if (target) lv_group_focus_obj(target);
    }
    else if (code == LV_EVENT_CLICKED) {
        /* 英文页面：Setting / Postnatal Rehabilitation / Pelvic Floor
         * Assessment 已接英文页；Therapy（盆底治疗）暂不跳转（待补） */
        if (obj == user_data->Set_Btn) {
            Menu_En_LeaveGroup();   /* 先移出 group，防 Page_Clean 时 refocus 悬垂（坑 6.8） */
            Set_En_ui();
        }
        else if (obj == user_data->PostRec_Btn) {
            Menu_En_LeaveGroup();   /* 先移出 group，防 Page_Clean 时 refocus 悬垂（坑 6.8） */
            PosReh_En_Page_Load(PosRehPage1);
        }
        else if (obj == user_data->PelFlo_Ass_Btn) {
            Menu_En_LeaveGroup();   /* 先移出 group，防 Page_Clean 时 refocus 悬垂（坑 6.8） */
            Ass_En_Page_Load(AssPage1);
        }
        else if (obj == user_data->PelFlo_The_Btn) {
            Menu_En_LeaveGroup();   /* 先移出 group，防 Page_Clean 时 refocus 悬垂（坑 6.8） */
            PelFlo_The_En_ui();     /* 进入英文治疗页四宫格 */
        }
        else {
            ESP_LOGI(TAG, "EN menu button clicked (page not ready yet)");
        }
    }
}

/**
 * @brief 英文主菜单创建入口（布局参考用户 Menu_widget_EN）
 * @param parent g_Ui.page_container
 */
void Menu_En_ui(lv_obj_t* parent)
{
    lv_obj_t* page_cont = Create_Obj(parent, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_center(page_cont);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);

    /* —— Pelvic Floor Assessment（盆底评估，左上） —— */
    /*盆底评估按钮*/
    Menu_En_Widget.PelFlo_Ass_Btn = Create_ImaButton(page_cont, 200, 110, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/green.bin");
    lv_obj_set_pos(Menu_En_Widget.PelFlo_Ass_Btn, 21, 10);
    /*盆底评估按钮照片*/
    lv_obj_t* PelFlo_Ass_Img = Creat_Image(Menu_En_Widget.PelFlo_Ass_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/PDPG.bin");
    lv_obj_align(PelFlo_Ass_Img, LV_ALIGN_CENTER, 0, -15);
    lv_obj_add_event_cb(Menu_En_Widget.PelFlo_Ass_Btn, Menu_En_button_event_cb, LV_EVENT_ALL, &Menu_En_Widget);
    /*盆底评估按钮标签*/
    lv_obj_t* PelFlo_Ass_Lab = Creat_Label(Menu_En_Widget.PelFlo_Ass_Btn, "Pelvic Floor Assessment", &lv_font_montserrat_14);
    lv_obj_align(PelFlo_Ass_Lab, LV_ALIGN_BOTTOM_MID, 0, 5);
    /*选中效果图片*/
    lv_obj_t* seleckImg1 = Creat_Image(Menu_En_Widget.PelFlo_Ass_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/seleck.bin");
    lv_obj_set_pos(seleckImg1, 5, 5);
    Menu_En_Widget.Ass_seleckImg = seleckImg1;

    /* —— Pelvic Floor Therapy（盆底治疗，右上） —— */
    /*盆底治疗按钮*/
    Menu_En_Widget.PelFlo_The_Btn = Create_ImaButton(page_cont, 200, 110, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/red.bin");
    lv_obj_set_pos(Menu_En_Widget.PelFlo_The_Btn, 244, 10);
    /*盆底治疗按钮照片*/
    lv_obj_t* PelFlo_The_Img = Creat_Image(Menu_En_Widget.PelFlo_The_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/PDZL.bin");
    lv_obj_align(PelFlo_The_Img, LV_ALIGN_CENTER, 0, -15);
    lv_obj_add_event_cb(Menu_En_Widget.PelFlo_The_Btn, Menu_En_button_event_cb, LV_EVENT_ALL, &Menu_En_Widget);
    /*盆底治疗按钮标签*/
    lv_obj_t* PelFlo_The_Lab = Creat_Label(Menu_En_Widget.PelFlo_The_Btn, "Pelvic Floor Therapy", &lv_font_montserrat_14);
    lv_obj_align(PelFlo_The_Lab, LV_ALIGN_BOTTOM_MID, 0, 5);
    /*选中效果图片*/
    lv_obj_t* seleckImg2 = Creat_Image(Menu_En_Widget.PelFlo_The_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/seleck.bin");
    lv_obj_set_pos(seleckImg2, 5, 5);
    lv_obj_add_flag(seleckImg2, LV_OBJ_FLAG_HIDDEN);
    Menu_En_Widget.The_seleckImg = seleckImg2;

    /* —— Postnatal Rehabilitation（产后修复，左下） —— */
    /*盆底修复按钮*/
    Menu_En_Widget.PostRec_Btn = Create_ImaButton(page_cont, 200, 110, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/yellow.bin");
    lv_obj_set_pos(Menu_En_Widget.PostRec_Btn, 21, 137);
    /*盆底修复按钮照片*/
     lv_obj_t* PostRec_Img = Creat_Image(Menu_En_Widget.PostRec_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/CHXF.bin");
    lv_obj_align(PostRec_Img, LV_ALIGN_CENTER, 0, -15);
    lv_obj_add_event_cb(Menu_En_Widget.PostRec_Btn, Menu_En_button_event_cb, LV_EVENT_ALL, &Menu_En_Widget);
    /*盆底修复按钮标签*/
    lv_obj_t* PostRec_Lab = Creat_Label(Menu_En_Widget.PostRec_Btn, "Postnatal Rehabilitation", &lv_font_montserrat_14);
    lv_obj_set_pos(PostRec_Lab, 65, 75);
    lv_obj_align(PostRec_Lab, LV_ALIGN_BOTTOM_MID, 0, 5);
    /*选中效果图片*/
    lv_obj_t* seleckImg4 = Creat_Image(Menu_En_Widget.PostRec_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/seleck.bin");
    lv_obj_set_pos(seleckImg4, 5, 5);
    lv_obj_add_flag(seleckImg4, LV_OBJ_FLAG_HIDDEN);
    Menu_En_Widget.PostRec_seleckImg = seleckImg4;

    /* —— Setting（设置，右下） —— */
    /*设置按钮*/
    Menu_En_Widget.Set_Btn = Create_ImaButton(page_cont, 200, 110, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/blue.bin");
    lv_obj_set_pos(Menu_En_Widget.Set_Btn, 244, 137);
    /*设置按钮照片*/
    lv_obj_t* Set_Img = Creat_Image(Menu_En_Widget.Set_Btn,"C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/SZ.bin");
    lv_obj_align(Set_Img, LV_ALIGN_CENTER, 0, -15);
    lv_obj_add_event_cb(Menu_En_Widget.Set_Btn, Menu_En_button_event_cb, LV_EVENT_ALL, &Menu_En_Widget);
    /*设置按钮标签*/
    lv_obj_t* Set_Lab = Creat_Label(Menu_En_Widget.Set_Btn, "Setting", &lv_font_montserrat_14);
    lv_obj_align(Set_Lab, LV_ALIGN_BOTTOM_MID, 0, 5);
    /*选中效果图片*/
    lv_obj_t* seleckImg3 = Creat_Image(Menu_En_Widget.Set_Btn, "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/seleck.bin");
    lv_obj_set_pos(seleckImg3, 5, 5);
    lv_obj_add_flag(seleckImg3, LV_OBJ_FLAG_HIDDEN);
    Menu_En_Widget.Set_seleckImg = seleckImg3;

    /* =====  注册按钮到 keypad group（按键导航，与中文一致） ===== */
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        lv_group_add_obj(g, Menu_En_Widget.PelFlo_Ass_Btn);
        lv_group_add_obj(g, Menu_En_Widget.PelFlo_The_Btn);
        lv_group_add_obj(g, Menu_En_Widget.PostRec_Btn);
        lv_group_add_obj(g, Menu_En_Widget.Set_Btn);
    }
}
