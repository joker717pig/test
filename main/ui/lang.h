/**
  ******************************************************************************
  * @文件名称   lang.h
  * @文件描述   多语言模块（同事代码依赖；ESP32 阶段 B1 仅用中文）
  *            移植自 EDA_EMG_LVGL_PC\lang.h，仅保留接口，str 表未移植
  ******************************************************************************
  */

#ifndef __LANG_H__
#define __LANG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

    typedef enum {
        STR_ID_INVALID = 0,
        /* [我们] B4 PosReh 说明页标签（同事 lang.h 原装枚举） */
        POSREH_PAGE1_ID_TITLE,
        POSREH_PAGE1_ID_TEXT,
        POSREH_PAGE1_ID_DIA,
        POSREH_PAGE1_ID_UTE,
        POSREH_PAGE1_ID_LAC,
        POSREH_PAGE1_ID_SELECT,
        STR_ID_MAX
    } str_id_t;

    typedef enum {
        LANG_EN = 0,
        LANG_ZH,
        LANG_MAX
    } app_lang_t;

    extern app_lang_t g_app_lang;

    void lang_set(app_lang_t lang);
    const char* lang_get_str(uint16_t str_id);
    void ui_switch_language(app_lang_t lang);
    void ui_load_screen(lv_obj_t* scr);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* __LANG_H__ */
