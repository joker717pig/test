#include "PelFlo_The_ui.h"
#include "PelFlo_The_ui_event.h"
#include "Child_ThePage.h"      /* Child_ThePage_ui / Child_ThePage_LeaveGroup */
#include "Child_MaiPage.h"      /* [我们] B6-A 保养格 Child_MaiPage_ui / Child_MaiPage_LeaveGroup */
#include "Chlid_DysPage.h"      /* [我们] B6-B 障碍格 Child_DysPage_ui / Child_DysPage_LeaveGroup */
#include "Chlid_KgePage.h"      /* [我们] B6-C 凯格尔格 Child_KgePage_ui / Child_KgePage_LeaveGroup */
#include "menu_ui.h"            /* Show_StubPage / 滚动补偿坐标 extern */
#include "app_keypad.h"         /* [我们] keypad group 注册 */
#include "app_therapy.h"        /* [我们] 阶段C：切页安全网（停治疗引擎） */

/* [我们] 保养/障碍/凯格尔 = stub（本会话范围外，各自后续会话）；不再 extern Child_Mai/Dys/Kge */

static void PelFloThe_Menu_widget(The_Widget_t* ui, lv_obj_t* page_cont);

/* [我们] 切页前把 4 个四宫格按钮移出 keypad group（坑 6.8 同族）
 * 非 static：PelFlo_The_ui_event.c 的四宫格 ESC 回调也要用 */
void The_LeaveGroup(void)
{
    lv_group_t *g = app_keypad_get_group();
    if (g == NULL) return;
    if (lv_obj_is_valid(The_Widget.ZL_Btn))  lv_group_remove_obj(The_Widget.ZL_Btn);
    if (lv_obj_is_valid(The_Widget.BY_Btn))  lv_group_remove_obj(The_Widget.BY_Btn);
    if (lv_obj_is_valid(The_Widget.ZA_Btn))  lv_group_remove_obj(The_Widget.ZA_Btn);
    if (lv_obj_is_valid(The_Widget.KGE_Btn)) lv_group_remove_obj(The_Widget.KGE_Btn);
}


The_Widget_t The_Widget;
Treat_Stage_t Cur_Stage = STAGE_1;        //初始化当前疗程阶段
ThePage_ID_t  Cur_ChildPage = ThePage;    //初始化当前页面
uint8_t Cur_Time = 1;                     //初始化治疗次数
uint8_t Cur_Treat = 1;                   //初始化治疗次数


/**
 * @brief 获取当前页面索引
 * @param
 * @return
 */
ThePage_ID_t Get_CurPage_Idx(void)
{
    return Cur_ChildPage;
}

/**
 * @brief 设置当前页面
 * @param idx
 */
void Set_CurPage_Idx(ThePage_ID_t idx)
{
    Cur_ChildPage = idx;
}

/**
 * @brief 获取当前疗程阶段索引
 * @param  
 * @return 
 */
Treat_Stage_t Get_CurStage_Idx(void)
{
    return Cur_Stage;
}

/**
 * @brief 设置当前疗程阶段
 * @param idx 
 */
void Set_CurStage_Idx(Treat_Stage_t idx)
{
    Cur_Stage = idx;
}
/**
 * @brief 获取当前治疗次数
 * @param
 * @return
 */
uint8_t  Get_CurTime_Idx(void)
{
    return Cur_Time;
}

/**
 * @brief 设置当前治疗次数
 * @param idx
 */
void Set_CurTime_Idx(uint8_t time)
{
    Cur_Time = time;
}

/**
 * @brief 获取当前治疗
 * @param
 * @return
 */
uint8_t  Get_CurTreat_Idx(void)
{
    return Cur_Treat;
}

/**
 * @brief 设置当前治疗
 * @param idx
 */
void Set_CurTreat_Idx(uint8_t treat)
{
    
    Cur_Treat = treat;
}

/**
 * @brief 盆底治疗页面创建入口
 * @param page_id
 */
void PelFlo_ThePage_Load(ThePage_ID_t page_id)
{
    therapy_stop();              /* [我们] 阶段C：切页安全网——任何四宫格切换停治疗引擎（停波+复位；未运行时无操作） */
    The_LeaveGroup();              /* [我们] 四宫格按钮先移出 group（坑 6.8） */
    Child_ThePage_LeaveGroup();    /* [我们] 治疗格子页对象先移出 group（坑 6.8 同族） */
    Child_MaiPage_LeaveGroup();    /* [我们] B6-A 保养格子页对象先移出 group（坑 6.8 同族） */
    Child_DysPage_LeaveGroup();    /* [我们] B6-B 障碍格子页对象先移出 group（坑 6.8 同族） */
    Child_KgePage_LeaveGroup();    /* [我们] B6-C 凯格尔格子页对象先移出 group（坑 6.8 同族） */
    Page_Clean();
    switch (page_id) {
    case ThePage:
        Child_ThePage_ui();
        break;
    case MaiPage:
        Child_MaiPage_ui();                       /* [我们] B6-A 保养格真页 */
        break;
    case DysPage:
        Child_DysPage_ui();                       /* [我们] B6-B 障碍格真页 */
        break;
    case KgePage:
        Child_KgePage_ui();                       /* [我们] B6-C 凯格尔格真页 */
        break;
    case MenPage:
        PelFlo_The_ui();
        break;
    default:
        break;
    }

}

/**
 * @brief 主界面ui设计
 * @param  none
 */
void PelFlo_The_ui(void)
{

    lv_obj_t* page_cont = Create_Obj(g_Ui.page_container, 480, 290, MAIN_BG_COLOR, 0);
    lv_obj_set_style_radius(page_cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(page_cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(page_cont);
    
    lv_obj_clear_flag(page_cont, LV_OBJ_FLAG_SCROLLABLE);                 /*禁用滑动条*/
    lv_obj_set_scrollbar_mode(page_cont, LV_SCROLLBAR_MODE_OFF);          /*不显示滚动条*/

    PelFloThe_Menu_widget(&The_Widget, page_cont);

    /* ===== [我们] 四宫格按钮注册 keypad group（默认焦点 = 治疗 ZL） ===== */
    lv_group_t *g = app_keypad_get_group();
    if (g) {
        lv_group_add_obj(g, The_Widget.ZL_Btn);
        lv_group_add_obj(g, The_Widget.BY_Btn);
        lv_group_add_obj(g, The_Widget.ZA_Btn);
        lv_group_add_obj(g, The_Widget.KGE_Btn);
        lv_group_focus_obj(The_Widget.ZL_Btn);
    }
}




/**
 * @brief 盆底肌治疗菜单页 治疗、保养、功能障碍、凯格尔
 * @param  none
 */
static void PelFloThe_Menu_widget(The_Widget_t* widget, lv_obj_t* page_cont)
{
    lv_obj_t* cont = Create_Obj(page_cont, 480, 290, FONT_GRAY_COLOR, 0);
    lv_obj_set_style_radius(cont, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_STATE_DEFAULT);         /* 去除边框 */
    lv_obj_center(cont);
 

    /*盆底治疗容器*/
    widget->ZL_Obj = Create_Obj(cont, 200, 110, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(widget->ZL_Obj, 15, 10);
    /*盆底治疗按钮*/
    widget->ZL_Btn = Create_Button(widget->ZL_Obj, 100, 70,FONT_WHITE_COLOR,10,1);
    lv_obj_align(widget->ZL_Btn, LV_ALIGN_CENTER, 0, -15);
    widget->ZL_Img = Creat_Image(widget->ZL_Btn, ZL_unSel_ICON);

    lv_obj_add_event_cb(widget->ZL_Btn, ThePage_MenuBtn_event_cb, LV_EVENT_ALL, widget);//LV_EVENT_ALL
    /*盆底评估标签*/
    lv_obj_t* Label1 = Creat_Label(widget->ZL_Obj, "盆底肌治疗", basic_widget.Chinise_Font_Btn);
    lv_obj_align(Label1, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_text_color(Label1, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);


    /*盆底肌保养容器*/
    widget->BY_Obj = Create_Obj(cont, 200, 110, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(widget->BY_Obj, 235, 10);
    /*盆底肌保养按钮*/
    widget->BY_Btn = Create_Button(widget->BY_Obj, 100, 70, FONT_WHITE_COLOR, 10, 1);
    lv_obj_align(widget->BY_Btn, LV_ALIGN_CENTER, 0, -15);
    widget->BY_Img = Creat_Image(widget->BY_Btn, BY_unSel_ICON);
    
    lv_obj_add_event_cb(widget->BY_Btn, ThePage_MenuBtn_event_cb, LV_EVENT_ALL, widget);//LV_EVENT_ALL
    /*盆底肌保养标签*/
    lv_obj_t* Label2 = Creat_Label(widget->BY_Obj, "盆底肌保养", basic_widget.Chinise_Font_Btn);
    lv_obj_align(Label2, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_text_color(Label2, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);


    /*功能障碍容器*/
    widget->ZA_Obj = Create_Obj(cont, 200, 110, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(widget->ZA_Obj, 15, 142);
    /*功能障碍按钮*/
    widget->ZA_Btn = Create_Button(widget->ZA_Obj, 100, 70, FONT_WHITE_COLOR, 10, 1);
    lv_obj_align(widget->ZA_Btn, LV_ALIGN_CENTER, 0, -15);
    widget->ZA_Img = Creat_Image(widget->ZA_Btn, ZA_unSel_ICON);
    
    lv_obj_add_event_cb(widget->ZA_Btn, ThePage_MenuBtn_event_cb, LV_EVENT_ALL, widget);//LV_EVENT_ALL
    /*功能障碍标签*/
    lv_obj_t* Label3 = Creat_Label(widget->ZA_Obj, "高张力盆底功能障碍", basic_widget.Chinise_Font_Btn);
    lv_obj_align(Label3, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_text_color(Label3, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);


    /*凯格尔训练容器*/
    widget->KGE_Obj = Create_Obj(cont, 200, 110, FONT_BLUE_COLOR, 0);
    lv_obj_set_pos(widget->KGE_Obj, 235, 142);
    /*凯格尔训练按钮*/
    widget->KGE_Btn = Create_Button(widget->KGE_Obj, 100, 70, FONT_WHITE_COLOR, 10, 1);
    lv_obj_align(widget->KGE_Btn, LV_ALIGN_CENTER, 0, -15);
    widget->KGE_Img = Creat_Image(widget->KGE_Btn, KG_unSel_ICON);

    lv_obj_add_event_cb(widget->KGE_Btn, ThePage_MenuBtn_event_cb, LV_EVENT_ALL, widget);//LV_EVENT_ALL
    /*凯格尔训练标签*/
    lv_obj_t* Label4 = Creat_Label(widget->KGE_Obj, "凯格尔训练", basic_widget.Chinise_Font_Btn);
    lv_obj_align(Label4, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_text_color(Label4, lv_color_hex(FONT_GRAY_COLOR), LV_PART_MAIN);

}
