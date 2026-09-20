#include "PelFlo_The_ui_event.h"
#include "PelFlo_The_ui.h"
#include "basic.h"
#include "esp_log.h"      /* [我们] 日志 */

static const char *TAG = "ThePage_evt";

/* [我们] 将四个按钮照片全部设未选中（lv_obj_is_valid 防悬垂，坑 B2-4） */
static void The_Set_UnSel(void)
{
    if (lv_obj_is_valid(The_Widget.ZL_Img))  lv_image_set_src(The_Widget.ZL_Img,  ZL_unSel_ICON);
    if (lv_obj_is_valid(The_Widget.BY_Img))  lv_image_set_src(The_Widget.BY_Img,  BY_unSel_ICON);
    if (lv_obj_is_valid(The_Widget.ZA_Img))  lv_image_set_src(The_Widget.ZA_Img,  ZA_unSel_ICON);
    if (lv_obj_is_valid(The_Widget.KGE_Img)) lv_image_set_src(The_Widget.KGE_Img, KG_unSel_ICON);
}

/**
 * @author zqt
 * @brief 菜单切换按钮事件回调函数
 *        [我们] 新增 LV_EVENT_FOCUSED（选中图标高亮）+ LV_EVENT_KEY（2D 导航）
 *        （PC 端 FOCUSED 高亮被注释，C1 要求删除注释残留，已按菜单页模式实现）
 */
void ThePage_MenuBtn_event_cb(lv_event_t* e)
{

    lv_obj_t* obj = lv_event_get_target(e);                     // 触发的控件
    lv_event_code_t code = lv_event_get_code(e);                // 触发的事件
    The_Widget_t* data = (The_Widget_t*)lv_event_get_user_data(e);            // 用户传进来的数据

    if (code == LV_EVENT_FOCUSED) {                             /* [我们] 选中高亮 */
        The_Set_UnSel();
        if (obj == data->ZL_Btn && lv_obj_is_valid(data->ZL_Img))        lv_image_set_src(data->ZL_Img,  ZL_Sel_ICON);
        else if (obj == data->BY_Btn && lv_obj_is_valid(data->BY_Img))   lv_image_set_src(data->BY_Img,  BY_Sel_ICON);
        else if (obj == data->ZA_Btn && lv_obj_is_valid(data->ZA_Img))   lv_image_set_src(data->ZA_Img,  ZA_Sel_ICON);
        else if (obj == data->KGE_Btn && lv_obj_is_valid(data->KGE_Img)) lv_image_set_src(data->KGE_Img, KG_Sel_ICON);
    }
    else if (code == LV_EVENT_KEY) {                            /* [我们] 2D 导航 + ESC 回主菜单 */
        uint32_t key = lv_event_get_key(e);
        if (key == LV_KEY_ESC) {
            /* [我们] 四宫格 ESC 回主菜单（坑 B4-3：删对象路径，调用后禁止再访问对象） */
            The_LeaveGroup();      /* 先移出 group 再 Page_Clean（坑 6.8） */
            Goto_MenuPage();
            return;
        }
        lv_obj_t* target = NULL;
        if (obj == data->ZL_Btn) {                              /* 治疗 左上 */
            if (key == LV_KEY_RIGHT) target = data->BY_Btn;
            else if (key == LV_KEY_DOWN) target = data->ZA_Btn;
        }
        else if (obj == data->BY_Btn) {                         /* 保养 右上 */
            if (key == LV_KEY_LEFT) target = data->ZL_Btn;
            else if (key == LV_KEY_DOWN) target = data->KGE_Btn;
        }
        else if (obj == data->ZA_Btn) {                         /* 障碍 左下 */
            if (key == LV_KEY_RIGHT) target = data->KGE_Btn;
            else if (key == LV_KEY_UP) target = data->ZL_Btn;
        }
        else if (obj == data->KGE_Btn) {                        /* 凯格尔 右下 */
            if (key == LV_KEY_LEFT) target = data->ZA_Btn;
            else if (key == LV_KEY_UP) target = data->BY_Btn;
        }
        if (target && lv_obj_is_valid(target)) {
            ESP_LOGI(TAG, "nav: LV_KEY=%lu", (unsigned long)key);
            lv_group_focus_obj(target);
            lv_event_stop_processing(e);   /* [我们] 纯导航分支非删对象，stop 无害（坑 B3-3 只禁删对象路径） */
        }
    }
    else if (code == LV_EVENT_CLICKED)                  //按钮点击事件
    {
        lv_obj_remove_state(obj, LV_STATE_CHECKED);
        if (obj == data->ZL_Btn)          //盆底治疗
        {
            Set_CurPage_Idx(ThePage);
            PelFlo_ThePage_Load(ThePage);
        }
        else if (obj == data->BY_Btn)     //盆底保养
        {
            Set_CurPage_Idx(MaiPage);
            PelFlo_ThePage_Load(MaiPage);
        }
        else if (obj == data->ZA_Btn)        //障碍
        {
            Set_CurPage_Idx(DysPage);
            PelFlo_ThePage_Load(DysPage);

        }
        else if (obj == data->KGE_Btn)            //凯格尔
        {
            Set_CurPage_Idx(KgePage);
            PelFlo_ThePage_Load(KgePage);
        }
    }
}
