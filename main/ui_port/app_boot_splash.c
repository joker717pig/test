/**
  ******************************************************************************
  * @文件名称   app_boot_splash.c
  * @文件描述   开机过渡画面实现（boot splash）：医疗蓝底 + 公司 logo + "Starting..."
  * @说明       医疗蓝 = TIME_BG_COLOR(0x000521)，与主菜单顶栏同色系，视觉平滑过渡。
  *             - logo 复用顶栏 log.bin（90×16，SPIFFS 小图，加载 ~几十 ms 可忽略）
  *               经 lv_image_set_scale 放大居中显示。
  *             - "Starting..." 用 LVGL 内置 montserrat 字体，零资源加载延迟，立即显
  *               示（中文需等字库 ~600ms，故用英文提示）。
  ******************************************************************************
  */

#include <string.h>
#include "app_boot_splash.h"
#include "basic.h"          /* TIME_BG_COLOR 医疗蓝 */
#include "app_log.h"

static const char *TAG = "app_boot_splash";

/* logo 路径与 Frame.c 顶栏一致（由 ui_port/app_fs 'C' 盘翻译到 SPIFFS） */
#define SPLASH_LOGO_PATH  "C:/Users/zqt/Desktop/PDJ_LVGL/png/menu/log.bin"
#define SPLASH_LOGO_SCALE 4        /* 90×16 → 360×64，居中放大增强感知 */

static lv_obj_t *s_splash = NULL;

void app_boot_splash_show(void)
{
    if (s_splash != NULL) {
        return;   /* 已显示 */
    }

    /* 过渡画面容器：全屏医疗蓝底，覆盖在默认屏幕之上 */
    s_splash = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_splash);
    lv_obj_set_size(s_splash, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_splash, lv_color_hex(TIME_BG_COLOR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_splash, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_splash, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_splash, 0, LV_PART_MAIN);
    lv_obj_clear_flag(s_splash, LV_OBJ_FLAG_SCROLLABLE);

    /* 公司 logo（放大居中） */
    lv_obj_t *logo = lv_image_create(s_splash);
    lv_image_set_src(logo, SPLASH_LOGO_PATH);
    lv_image_set_scale(logo, SPLASH_LOGO_SCALE * LV_SCALE_NONE);

    /* "Starting..." 提示（内置字体，零延迟） */
    lv_obj_t *label = lv_label_create(s_splash);
    lv_label_set_text(label, "Starting...");
    lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), LV_PART_MAIN);

    /* 垂直居中布局：logo 上方、提示文字下方 */
    lv_obj_align(logo, LV_ALIGN_CENTER, 0, -24);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 44);

    /* 置顶：盖住一切，等 UI 加载完再隐藏 */
    lv_obj_move_foreground(s_splash);

    ESP_LOGI(TAG, "boot splash shown (medical blue + logo + Starting...)");
}

void app_boot_splash_hide(void)
{
    if (s_splash == NULL) {
        return;
    }
    lv_obj_del(s_splash);
    s_splash = NULL;
    ESP_LOGI(TAG, "boot splash hidden");
}