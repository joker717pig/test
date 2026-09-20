#include "PelFlo_The_En_ui_event.h"
#include "PelFlo_The_En_ui.h"
#include "basic.h"
#include "esp_log.h"      /* [我们] 日志 */

static const char *TAG = "ThePageEn_evt";

/* [我们] 将四个按钮照片全部设未选中（lv_obj_is_valid 防悬垂，坑 B2-4） */
static void The_En_Set_UnSel(void)
{
    if (lv_obj_is_valid(The_En_Widget.ZL_Img))  lv_image_set_src(The_En_Widget.ZL_Img,  ZL_unSel_ICON);
    if (lv_obj_is_valid(The_En_Widget.BY_Img))  lv_image_set_src(The_En_Widget.BY_Img,  BY_unSel_ICON);
    if (lv_obj_is_valid(The_En_Widget.ZA_Img))  lv_image_set_src(The_En_Widget.ZA_Img,  ZA_unSel_ICON);
    if (lv_obj_is_valid(The_En_Widget.KGE_Img)) lv_image_set_src(The_En_Widget.KGE_Img, KG_unSel_ICON);
}

/**
 * @author zqt
 * @brief 英文四宫格菜单切换按钮事件回调
 *        [我们] 逻辑与中文 ThePage_MenuBtn_event_cb 一致，但：
 *        - FOCUSED 高亮操作英文 The_En_Widget
 *        - CLICKED 跳英文 PelFlo_ThePage_En_Load（否则会误进中文子页）
 *        - ESC 经 Goto_MenuPage()（按 g_app_lang 自动回英文主菜单）
 */
void ThePage_En_MenuBtn_event_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    The_En_Widget_t* data = (The_En_Widget_t*)lv_event_get_user_data(e);

    if (code == LV_EVENT_FOCUSED) {                             /* [我们] 选中高亮 */
        The_En_Set_UnSel();
        if (obj == data->ZL_Btn && lv_obj_is_valid(data->ZL_Img))        lv_image_set_src(data->ZL_Img,  ZL_Sel_ICON);
        else if (obj == data->BY_Btn && lv_obj_is_valid(data->BY_Img))   lv_image_set_src(data->BY_Img,  BY_Sel_ICON);
        else if (obj == data->ZA_Btn && lv_obj_is_valid(data->ZA_Img))   lv_image_set_src(data->ZA_Img,  ZA_Sel_ICON);
        else if (obj == data->KGE_Btn && lv_obj_is_valid(data->KGE_Img)) lv_image_set_src(data->KGE_Img, KG_Sel_ICON);
    }
    else if (code == LV_EVENT_KEY) {                            /* [我们] 2D 导航 + ESC 回主菜单 */
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ESC) {
            The_En_LeaveGroup();      /* 先移出 group 再回菜单（坑 6.8） */
            Goto_MenuPage();
            return;
        }
        lv_obj_t* target = NULL;
        if (obj == data->ZL_Btn) {                              /* Therapy 左上 */
            if (key == LV_KEY_RIGHT) target = data->BY_Btn;
            else if (key == LV_KEY_DOWN) target = data->ZA_Btn;
        }
        else if (obj == data->BY_Btn) {                         /* Maintenance 右上 */
            if (key == LV_KEY_LEFT) target = data->ZL_Btn;
            else if (key == LV_KEY_DOWN) target = data->KGE_Btn;
        }
        else if (obj == data->ZA_Btn) {                         /* Dysfunction 左下 */
            if (key == LV_KEY_RIGHT) target = data->KGE_Btn;
            else if (key == LV_KEY_UP) target = data->ZL_Btn;
        }
        else if (obj == data->KGE_Btn) {                        /* Kegels 右下 */
            if (key == LV_KEY_LEFT) target = data->ZA_Btn;
            else if (key == LV_KEY_UP) target = data->BY_Btn;
        }
        if (target && lv_obj_is_valid(target)) {
            ESP_LOGI(TAG, "nav: LV_KEY=%lu", (unsigned long)key);
            lv_group_focus_obj(target);
            lv_event_stop_processing(e);
        }
    }
    else if (code == LV_EVENT_CLICKED)                  //按钮点击事件
    {
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
        if (obj == data->ZL_Btn)          //Pelvic Floor Therapy
        {
            Set_CurPage_Idx(ThePage);
            PelFlo_ThePage_En_Load(ThePage);
        }
        else if (obj == data->BY_Btn)     //Pelvic Floor Maintenance
        {
            Set_CurPage_Idx(MaiPage);
            PelFlo_ThePage_En_Load(MaiPage);
        }
        else if (obj == data->ZA_Btn)     //Dysfunction
        {
            Set_CurPage_Idx(DysPage);
            PelFlo_ThePage_En_Load(DysPage);
        }
        else if (obj == data->KGE_Btn)    //Kegels
        {
            Set_CurPage_Idx(KgePage);
            PelFlo_ThePage_En_Load(KgePage);
        }
    }
}
