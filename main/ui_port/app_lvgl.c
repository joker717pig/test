/**
  ******************************************************************************
  * @文件名称   app_lvgl.c
  * @文件描述   LVGL 初始化实现（阶段 1.5，经 esp_lvgl_port 接入 ST7365）
  *            配置：
  *              - 颜色格式 RGB565
  *              - 全帧双缓冲 PSRAM（480*320*2B ×2 = 600KB，buff_spiram=true）
  *              - swap_bytes=false：阶段 0 panel 已在 draw_bitmap 内转大端（MSB first）
  *              - trans_size 对 SPI 面板无效（panel 内部 8行×4 环形 SRAM 缓冲做 DMA 中转）
  *              - LVGL 渲染任务栈 8192
  * @说明      flush 完成通知：esp_lvgl_port 注册 io 的 on_color_trans_done →
  *            lv_disp_flush_ready。draw_bitmap 分带(每带一次 tx_color)会触发多次
  *            ready，对 LVGL 幂等；整帧最后一带 DMA 结束的那次 ready 才真正标志
  *            flush 完成。draw_bitmap 常态只排队、不每帧阻塞排空（DMA 源为内部 RAM
  *            s_swap_buf，非 PSRAM draw buffer）。
  *            ⚠ 但 OTA 擦写内部 flash 会关 cache/屏蔽非 IRAM 中断，而 LCD SPI 完成
  *            中断与回调链（esp_lcd post_cb / 本组件 flush_ready 回调）都不在 IRAM，
  *            SPI LCD 也无 IRAM 安全配置；关 cache 窗口里在途 flush 的完成通知会丢失
  *            → 刷新永久冻死。故擦 flash 前必须 app_lvgl_begin_flash_op()（取锁停渲染
  *            + app_lcd_drain 排空 DMA），擦完 app_lvgl_end_flash_op()。详见 doc 0D 坑 #9。
  ******************************************************************************
  */

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "app_lvgl.h"
#include "app_lcd.h"
#include "app_log.h"

static const char *TAG = "app_lvgl";

static lv_display_t *s_disp = NULL;

esp_err_t app_lvgl_init(void)
{
    if (s_disp != NULL) {
        return ESP_OK;
    }

    /* 1) LVGL port 初始化：创建 LVGL 渲染任务 + tick 定时器 */
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 8192,               /* 计划要求：LVGL 渲染任务栈 */
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .task_stack_caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_DEFAULT,
        .timer_period_ms = 5,
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl port init failed");

    /* 2) 接入 ST7365 显示（自定义 panel，LVGL-agnostic） */
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = app_lcd_get_io(),            /* flush 完成回调注册用（esp_lvgl_port assert 非空） */
        .panel_handle = app_lcd_get_panel(),      /* 自定义 ST7365 panel（draw_bitmap） */
        .buffer_size = CONFIG_LCD_H_RES * CONFIG_LCD_V_RES,  /* 全帧 480*320 */
        .double_buffer = true,                    /* PSRAM 双缓冲 300KB×2 */
        .trans_size = 0,                          /* 对 SPI 面板无效（panel 内 SRAM 中转） */
        .hres = CONFIG_LCD_H_RES,
        .vres = CONFIG_LCD_V_RES,
        .monochrome = false,
        .rotation = {
            /* 关键：esp_lvgl_port 的 rotation_update 会按此配置重设 panel 的
             * swap_xy / mirror（ROTATION_0 时），因此必须反映硬件横屏基线
             * MADCTL=0xE8（MV|MX|MY|BGR）：swap_xy/mirror 均为 true 才会
             * 在 ROTATION_0 时保持横屏；若全 false 会清掉 MV/MX/MY → 变竖屏。 */
            .swap_xy = true,    /* MADCTL 0xE8 含 MV（已 swap=横屏），保持 */
            .mirror_x = true,   /* 0xE8 含 MX，保持横屏方向 */
            .mirror_y = true,   /* 0xE8 含 MY，保持横屏方向 */
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = false,      /* PSRAM buffer 非 DMA 可寻址 */
            .buff_spiram = true,    /* draw buffer 在 PSRAM */
            .sw_rotate = false,     /* 硬件旋转（面板固定横屏） */
            .swap_bytes = false,    /* panel 内部已转大端，LVGL 侧不换字节 */
            .full_refresh = false,
            .direct_mode = false,
        },
    };
    /* 开机白屏修复关键：lvgl_port_add_disp 内部会 lvgl_port_lock(0)→lv_display_create
     * （创建默认白底活动屏幕）→lvgl_port_unlock(0)。而 LVGL 渲染任务优先级(4)高于
     * main(1)，unlock 瞬间渲染任务即抢跑，把白底 flush 上屏 → 背光已亮（app_lcd_init
     * 已点灯）→ 白闪。
     * 正解：在 add_disp 之前先持递归锁（重入计数 +1），add_disp 内部的 lock/unlock
     * 只把计数降回 1、不被真正释放，LVGL 渲染任务取不到锁，不会抢跑。趁这窗口把默
     * 认屏幕背景改成医疗蓝，再 unlock：此后渲染任务 flush 的第一帧即医疗蓝，无缝衔接
     * app_lcd_init 用硬件刷的医疗蓝，全程无白帧、无白闪。 */
    lvgl_port_lock(0);
    s_disp = lvgl_port_add_disp(&disp_cfg);
    if (s_disp == NULL) {
        lvgl_port_unlock();
        ESP_LOGE(TAG, "lvgl add disp failed");
        return ESP_FAIL;
    }
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000521), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);
    lvgl_port_unlock();

    ESP_LOGI(TAG, "LVGL init OK (disp=%p, %dx%d, RGB565, double PSRAM buf %uKB)",
             s_disp, CONFIG_LCD_H_RES, CONFIG_LCD_V_RES,
             (CONFIG_LCD_H_RES * CONFIG_LCD_V_RES * 2) / 1024);
    return ESP_OK;
}

lv_display_t *app_lvgl_get_disp(void)
{
    return s_disp;
}

/* ============================================================================
 * 内部 flash 擦写安全区（OTA 图片/ESP32 升级用）
 * ----------------------------------------------------------------------------
 * 背景：擦写内部 flash 会 spi_flash_disable_interrupts_caches_and_other_cpu：
 * 关 cache、屏蔽非 IRAM 中断、停住另一核。LCD 的 SPI 完成回调链
 * （esp_lcd lcd_spi_post_trans_color_cb / esp_lvgl_port flush_io_ready_cb）
 * 都不在 IRAM，且 SPI LCD 无 IRAM 安全 Kconfig；若此刻有在途 flush/DMA，其
 * trans_done 中断被屏蔽，完成通知与 SPI 驱动"中断惰性使能"状态机竞争，会导致
 * LCD SPI 队列永久停摆、LVGL 刷新冻死（进度条停住）。
 *
 * 做法：begin 时取 LVGL 递归锁（LVGL 任务循环里 lvgl_port_lock(0) 实为
 * portMAX_DELAY，持锁后它阻塞在锁上、不再调 lv_timer_handler/发新 flush），
 * 再 app_lcd_drain() 排空在途颜色事务（等 DMA 全部完成、flush_ready 已投递）。
 * 此后关 cache 窗口里无任何在飞 LCD 传输。end 释放锁，LVGL 任务恢复渲染。
 * 期间屏幕保持最后一帧静态。
 * ==========================================================================*/
void app_lvgl_begin_flash_op(void)
{
    if (s_disp == NULL) {
        return;   /* LVGL 未初始化（如纯命令行单目标升级），无需保护 */
    }
    /* 1) 持锁：等 LVGL 任务退出当前 lv_timer_handler，并阻止它再发起新 flush */
    lvgl_port_lock(0);   /* 0 = portMAX_DELAY，递归锁，本任务可重入 */
    /* 2) 排空在途 SPI 颜色事务：等最后一带 DMA 完成、flush_ready 已投递 */
    app_lcd_drain();
}

void app_lvgl_end_flash_op(void)
{
    if (s_disp == NULL) {
        return;
    }
    lvgl_port_unlock();
}

/* ----------------------------------------------------------------------------
 * 安全区内临时放行 LVGL 刷新一帧（OTA 图片/ESP32 长擦写 + 下载期间维持进度条动画）
 *
 * begin_flash_op 持着 LVGL 递归锁，LVGL 任务被阻塞、不会重画进度条；若整片擦除/
 * 整段下载都持锁，进度条会定在 0%。本函数周期性：解锁 → 让出 CPU 让 LVGL 任务跑
 * 一轮 lv_timer_handler（读 g_ota_ui 重画进度条/百分比、发 flush）→ 重新取锁 →
 * app_lcd_drain() 排空在途 DMA。drain 保证返回时总线空闲，调用方随后擦/写 flash
 * （关 cache）时无在飞 LCD 传输，冻死修复的安全前提不变。
 * --------------------------------------------------------------------------*/
void app_lvgl_pump_flash_ui(void)
{
    if (s_disp == NULL) {
        return;   /* LVGL 未初始化（纯命令行单目标升级），无需刷新 */
    }
    /* 1) 释放本任务持有的递归锁，LVGL 任务才能取到锁跑渲染 */
    lvgl_port_unlock();
    /* 2) 让出 CPU：本任务 vTaskDelay 阻塞期间，同优先级的 LVGL 任务被调度，
 *    取锁、跑 lv_timer_handler、把进度条 flush 排入 SPI DMA。 */
    vTaskDelay(pdMS_TO_TICKS(30));
    /* 3) 重新取锁（portMAX_DELAY：若 LVGL 仍在渲染则等它退出），再 drain 排空
     *    在途颜色 DMA，确保返回后擦/写 flash 的关 cache 窗口里无在飞 LCD 传输。 */
    lvgl_port_lock(0);
    app_lcd_drain();
}
