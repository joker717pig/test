/**
  ******************************************************************************
  * @文件名称   app_boot_splash.h
  * @文件描述   开机过渡画面（boot splash）：医疗蓝底 + 公司 logo + "Starting..."
  *            解决「背光延迟点亮导致 6 秒黑屏」的体验问题——屏幕背光在 LCD 就绪
  *            即点亮（约 1.8s），此时先显示本过渡画面；完整 UI（主菜单）后台加载
  *            完成后隐藏本画面、落入主菜单。
  * @说明       本模块只在 LVGL 任务上下文（lv_timer 回调）中调用，无需加锁。
  ******************************************************************************
  */

#ifndef APP_BOOT_SPLASH_H
#define APP_BOOT_SPLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief  显示开机过渡画面（医疗蓝底 + logo + "Starting..."）
 * @note   必须在 LVGL 任务上下文调用；依赖 'C' 盘 FS 驱动已注册（app_fs_init）
 */
void app_boot_splash_show(void);

/**
 * @brief  隐藏并销毁开机过渡画面（主菜单构建完成后调用）
 */
void app_boot_splash_hide(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_BOOT_SPLASH_H */