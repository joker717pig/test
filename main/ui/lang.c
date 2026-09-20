/**
  ******************************************************************************
  * @文件名称   lang.c
  * @文件描述   多语言模块 stub（阶段 B1 仅中文，函数留空占位）
  ******************************************************************************
  */

#include "lang.h"
#include "app_params.h"   /* [我们] 参数存储：语言持久化 */
#include "basic.h"        /* [我们] Ui_EnterMenu（切语言重建） */
#include "esp_log.h"

static const char *TAG = "lang";

app_lang_t g_app_lang = LANG_ZH;

void lang_set(app_lang_t lang)
{
    g_app_lang = lang;
    ESP_LOGI(TAG, "lang set to %d (stub)", (int)lang);
}

const char* lang_get_str(uint16_t str_id)
{
    /* [我们] B4 PosReh 说明页标签（仅中文；同事 lang.c 原装中文串） */
    switch (str_id) {
    case POSREH_PAGE1_ID_TITLE:  return "产后康复说明";
    case POSREH_PAGE1_ID_TEXT:   return "产后康复涵盖催乳、子宫复旧、腹直肌分离修复三大核心项目。";
    case POSREH_PAGE1_ID_DIA:    return "腹直肌分离         剩余8次";
    case POSREH_PAGE1_ID_UTE:    return "催乳               剩余8次";
    case POSREH_PAGE1_ID_LAC:    return "子宫复旧           剩余8次";
    case POSREH_PAGE1_ID_SELECT: return "选择";
    default:                     return "";
    }
}

void ui_switch_language(app_lang_t lang)
{
    /* [我们] 参数存储：切换语言 = 设置全局 + 立即落盘（语言是全局项，无“离开页”时机） */
    lang_set(lang);
    app_params_set_lang((int32_t)lang);
    app_params_save_settings();
    /* [我们] 两套 UI：切语言立即重建回主菜单（自动按新语言进入对应菜单） */
    Ui_EnterMenu();
}

void ui_load_screen(lv_obj_t* scr)
{
    (void)scr;
}
