/**
  ******************************************************************************
  * @文件名称   Ass_En_ui.h
  * @文件描述   英文盆底评估页（Pelvic Floor Assessment，对应中文
  *            ui_page_cn/PelFlo_AssPage）。复用中文全局状态（Ass_Widget / chart
  *            系列 / 滚动偏移 / ecg 采样数组），仅 UI 文本翻译为英文。
  *            事件回调：语言无关的中文回调直接复用（BackToMenu / Ass_Page_Esc /
  *            scroll / Draw_EcgTable / Mydraw），跳转相关的用 Ass_En_* 英文回调。
  *            命名：符号统一 Ass_En_*（防链接冲突）。
  ******************************************************************************
  */

#ifndef __ASS_EN_UI_H__
#define __ASS_EN_UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "PelFlo_Ass_ui.h"   /* Ass_Widget_t / Ass_PageID_t / ecg 采样数组 / extern 全局 */

    /* 英文评估页子页面创建入口（对应中文 Ass_Page_Load） */
    void Ass_En_Page_Load(Ass_PageID_t page_id);

    /* 英文记录表格（健康档案子页数据，供 Ass_En_Record_Menu_event_cb 调用） */
    void Record_En_wdiget(Ass_Widget_t* widget, lv_obj_t* page_cont);
    void ass_en_guide_update(uint8_t phase);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __ASS_EN_UI_H__ */
