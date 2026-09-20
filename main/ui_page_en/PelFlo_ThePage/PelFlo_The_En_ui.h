#ifndef __PELFLO_THE_EN_UI_H__
#define __PELFLO_THE_EN_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#if 1

#include "lvgl.h"
#include "basic.h"
#include "menu_ui.h"
/* [我们] 复用中文 PelFlo_The_ui.h：ThePage_ID_t 类型 + extern 状态函数
 * （Cur_Stage/Cur_ChildPage/Cur_Time 与 Get/Set_Cur* 由中文 PelFlo_The_ui.c 定义，
 *   被 therapy 模块依赖，英文版 extern 复用不加 _En）+ 四宫格图标路径宏 */
#include "PelFlo_The_ui.h"

/* 英文四宫格控件结构体（独立于中文 The_Widget_t） */
typedef struct {
    lv_obj_t* ZL_Obj;               //Pelvic Floor Therapy container
    lv_obj_t* BY_Obj;               //Pelvic Floor Maintenance container
    lv_obj_t* ZA_Obj;               //Dysfunction container
    lv_obj_t* KGE_Obj;              //Kegels container
    lv_obj_t* ZL_Img;               //Pelvic Floor Therapy image
    lv_obj_t* BY_Img;               //Pelvic Floor Maintenance image
    lv_obj_t* ZA_Img;               //Dysfunction image
    lv_obj_t* KGE_Img;              //Kegels image
    lv_obj_t* ZL_Btn;               //Therapy button
    lv_obj_t* BY_Btn;               //Maintenance button
    lv_obj_t* ZA_Btn;               //Dysfunction button
    lv_obj_t* KGE_Btn;              //Kegels button
}The_En_Widget_t;

extern The_En_Widget_t The_En_Widget;

void PelFlo_The_En_ui(void);
void PelFlo_ThePage_En_Load(ThePage_ID_t page_id);

/* [我们] B6-C 四宫格 ESC 回主菜单：切页前把 4 个按钮移出 group（坑 6.8） */
void The_En_LeaveGroup(void);

#endif /*LV_USE_BUTTON*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*__PELFLO_THE_EN_UI_H__*/
