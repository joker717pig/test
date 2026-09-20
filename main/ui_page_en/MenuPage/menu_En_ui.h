/**
  ******************************************************************************
  * @文件名称   menu_En_ui.h
  * @文件描述   英文主菜单（两套独立 UI 架构验证，阶段 E2）
  *            英文 UI 独立于同事中文 ui_page/：布局/文字各自独立，按 g_app_lang
  *            经 basic.c::Ui_EnterMenu() 分叉进入；跳转/返回逻辑与中文一致。
  *            命名规则：.c/.h 统一 *_En（避免与中文同名头 include 歧义），
  *            内部符号统一 Menu_En_* 前缀（防链接冲突）。
  ******************************************************************************
  */

#ifndef __MENU_EN_UI_H__
#define __MENU_EN_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "basic.h"

    /* 英文菜单项页面 ID（与中文 Menu_PageID_t 对应，保证跳转/返回逻辑一致；
     * 当前英文子页面未做，仅作跳转预留） */
    typedef enum {
        EN_PAGE_MENU = 0,      //主菜单页
        EN_PAGE_SETTING,       //设置页
        EN_PAGE_PELFLO_ASS,    //盆底评估页
        EN_PAGE_PELFLO_THE,    //盆底治疗页
        EN_PAGE_POSREH,        //产后康复页
        EN_PAGE_MAX
    } Menu_En_PageID_t;

    /* 英文菜单控件结构体（独立于中文 Menu_Widget_t） */
    typedef struct {
        lv_obj_t* PelFlo_Ass_Btn;    //盆底评估按钮
        lv_obj_t* PelFlo_The_Btn;    //盆底治疗按钮
        lv_obj_t* Set_Btn;           //设置按钮
        lv_obj_t* PostRec_Btn;       //产后修复按钮
        lv_obj_t* Ass_seleckImg;     //盆底评估选中效果图片
        lv_obj_t* The_seleckImg;     //盆底治疗选中效果图片
        lv_obj_t* PostRec_seleckImg; //产后修复选中效果图片
        lv_obj_t* Set_seleckImg;     //设置选中效果图片
    } Menu_En_Widget_t;

    extern Menu_En_Widget_t Menu_En_Widget;   /* 定义在 menu_En_ui.c */

    /* 英文主菜单创建入口（等价于中文 Menu_ui） */
    void Menu_En_ui(lv_obj_t* parent);

    /* [我们] 切页前把英文菜单按钮移出 keypad group（通用守则，坑 6.8 族） */
    void Menu_En_LeaveGroup(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __MENU_EN_UI_H__ */
