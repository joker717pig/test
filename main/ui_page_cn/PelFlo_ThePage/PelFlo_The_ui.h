#ifndef __PELFO_THE_UI_H__
#define __PELFO_THE_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#include "lvgl.h"
#include "basic.h"
#include "menu_ui.h"

/* 同事原装 Windows 路径：由 ui_port/app_fs 'C' 盘驱动翻译到 SPIFFS（ThePage/ 29 bin 已入镜像） */
#define     ZL_unSel_ICON         "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/ZL_unSel.bin"    //治疗未选中图标
#define     BY_unSel_ICON         "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/BY_unSel.bin"    //保养未选中图标
#define     ZA_unSel_ICON        "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/ZA_unSel.bin"     //障碍未选中图标
#define     KG_unSel_ICON         "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/KGE_unSel.bin"   //凯格尔未选中图标
#define     ZL_Sel_ICON           "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/ZL_Sel.bin"      //治疗选中图标
#define     BY_Sel_ICON           "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/BY_Sel.bin"      //保养选中图标
#define     ZA_Sel_ICON           "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/ZA_Sel.bin"      //障碍选中图标
#define     KG_Sel_ICON           "C:/Users/zqt/Desktop/PDJ_LVGL/png/ThePage/KGE_Sel.bin"     //凯格尔选中图标


typedef struct {
    lv_obj_t* ZL_Obj;               //盆底治疗容器
    lv_obj_t* BY_Obj;               //盆底保养容器
    lv_obj_t* ZA_Obj;               //盆底功能障碍容器
    lv_obj_t* KGE_Obj;              //凯格尔训练容器
    lv_obj_t* ZL_Img;               //盆底治疗图片
    lv_obj_t* BY_Img;               //盆底保养图片
    lv_obj_t* ZA_Img;               //盆底功能障碍图片
    lv_obj_t* KGE_Img;              //凯格尔训练图片
    lv_obj_t* ZL_Btn;               //治疗按钮
    lv_obj_t* BY_Btn;               //保养按钮
    lv_obj_t* ZA_Btn;               //功能障碍按钮
    lv_obj_t* KGE_Btn;              //凯格尔按钮

}The_Widget_t;


extern The_Widget_t The_Widget;

/* 页面的数组保存位置 */
typedef enum {
    ThePage,       //盆底肌治疗
    MaiPage,       //盆底肌保养
    DysPage,       //功能障碍
    KgePage,       //凯格尔训练
    MenPage,       //菜单页（四宫格）
    TheMenPage_SUM     // 最大界面容量
}ThePage_ID_t;


void PelFlo_The_ui(void);
void PelFlo_ThePage_Load(ThePage_ID_t page_id);
Treat_Stage_t Get_CurStage_Idx(void);
void Set_CurStage_Idx(Treat_Stage_t idx);
ThePage_ID_t Get_CurPage_Idx(void);
void Set_CurPage_Idx(ThePage_ID_t idx);
uint8_t  Get_CurTime_Idx(void);
void Set_CurTime_Idx(uint8_t time);
uint8_t  Get_CurTreat_Idx(void);
void Set_CurTreat_Idx(uint8_t threat);
/* [我们] B6-C 四宫格 ESC 回主菜单：切页前把 4 个按钮移出 group（坑 6.8） */
void The_LeaveGroup(void);



#endif /*LV_USE_BUTTON*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*MY_UI_PAGE1_H*/
