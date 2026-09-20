/**
  ******************************************************************************
  * @文件名称   app_main.c
  * @文件描述   EDA_EMG_ESP32 应用程序入口（阶段 0.3：构建冒烟）
  *            最小程序：打印启动 banner 后进入死循环。
  * @说明       当前仅验证工具链与烧录链路；后续阶段在此扩展事件框架与显示驱动。
  ******************************************************************************
  */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "app_console.h"
#include "app_debug.h"
#include "app_events.h"
#include "app_lcd.h"
#include "app_lvgl.h"
#include "app_fs.h"      /* ui_port: 'C'盘路径翻译+内存加载 */
#include "basic.h"       /* ui_page: 控件工厂（同事原装） */
#include "Frame.h"       /* ui_page: 顶栏 */
#include "menu_ui.h"     /* ui_page/MenuPage: 主菜单 */
#include "app_keypad.h"  /* ui_port: keypad 输入 */
#include "elec_off_ui.h" /* ui: 全局电极脱落提示弹窗 */
#include "app_log.h"
#include "app_modbus.h"
#include "app_modbus_reg.h"   /* Modbus 寄存器映射 (含系统复位命令) */
#include "emg_force.h"   /* 阶段 3.2：肌电力度引擎 */
#include "app_heartbeat.h"  /* 心跳轮询（RTC 时间 + 电池信息，1s） */
#include "voice.h"          /* 阶段 V：语音播放（Modbus 控制 STM32 WTN6170） */
#include "app_therapy.h"    /* 阶段 C：治疗控制（处方3疗程1 下发 + 30min 会话示例） */
#include "app_spiffs.h"
#include "app_params.h"    /* 阶段 E：参数存储（NVS） */
#include "app_ota.h"       /* 阶段 D：OTA 独立任务（HTTP 升级/版本清单，16KB 栈） */
#include "lang.h"         /* 阶段 E：语言参数加载 */
#include "PosReh_Page_ui.h" /* 阶段 E：治疗参数加载 */
#include "app_boot_splash.h" /* 开机过渡画面（医疗蓝底 + logo + Starting...） */

static const char *TAG = "app_main";

/* ---- 前向声明 ---- */
static void ui_init_timer_cb(lv_timer_t *timer);
static void ui_load_timer_cb(lv_timer_t *timer);

void app_main(void)
{
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ESP_LOGI(TAG, "=============================================");
    ESP_LOGI(TAG, " EDA_EMG_ESP32  (build smoke test)");
    ESP_LOGI(TAG, "=============================================");
    ESP_LOGI(TAG, "IDF version  : %s", esp_get_idf_version());
    ESP_LOGI(TAG, "Chip         : %s", CONFIG_IDF_TARGET);
    ESP_LOGI(TAG, "Cores        : %d", chip_info.cores);
    ESP_LOGI(TAG, "Features     : %s%s%s%s%s",
             (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi " : "",
             (chip_info.features & CHIP_FEATURE_BLE) ? "BLE " : "",
             (chip_info.features & CHIP_FEATURE_BT) ? "BT " : "",
             (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "EmbeddedFlash " : "",
             (chip_info.features & CHIP_FEATURE_EMB_PSRAM) ? "EmbeddedPSRAM" : "");
    ESP_LOGI(TAG, "App FW ver   : %u.%u",
             (unsigned)(APP_FW_VERSION >> 8), (unsigned)(APP_FW_VERSION & 0xFF));

    uint32_t flash_size = 0;
    esp_flash_get_size(NULL, &flash_size);
    ESP_LOGI(TAG, "Flash size   : %u MB", flash_size / (1024 * 1024));
    ESP_LOGI(TAG, "PSRAM size   : %u MB", (uint32_t)(esp_psram_get_size() / (1024 * 1024)));
    ESP_LOGI(TAG, "Free heap    : %u KB", (uint32_t)(esp_get_free_heap_size() / 1024));

    ESP_LOGI(TAG, "banner OK. Entering idle loop.");

    /* SPIFFS 文件系统（阶段 S0：字体/图片资源） */
    ESP_ERROR_CHECK(app_spiffs_init());

    /* 阶段 E：参数存储（NVS 初始化 + 加载全部参数到内存副本） */
    ESP_ERROR_CHECK(app_params_init());

    /* 0.6 事件框架：创建默认事件循环 + 注册测试 handler */
    ESP_ERROR_CHECK(app_event_loop_init());
    /* 发测试事件验证事件通路（monitor 观察） */
    ESP_ERROR_CHECK(app_event_post_test_events());

    /* 0.C 增强：串口命令 console（help/stack/mem/psram/part/info/lcd） */
    ESP_ERROR_CHECK(app_console_start());

    /* 阶段 D：独立 OTA 任务（ota manifest/esp32/image/stm32 异步执行，16KB 栈）。
     * HTTP 下载/JSON 解析/sha256/写 flash 是重活，不能同步跑在 console(4KB) 里 */
    ESP_ERROR_CHECK(app_ota_task_start());

    /* 阶段 A：Modbus RTU 主站（UART1 TX=17 RX=18 @115200；参考 PC modbus_client.py）
     * 按键帧 → INPUT_EVENT_BUTTON_PRESSED；采集帧 → 环形缓冲 */
    ESP_ERROR_CHECK(app_modbus_init());

    /* 阶段 3.2：肌电力度引擎（50ms 泵消费采集环形缓冲 → 力度包络/RMS） */
    ESP_ERROR_CHECK(emg_force_init());

    /* 心跳轮询：1s 读 STM32 RTC + 电池（顶栏时间/电量；需在 modbus 之后） */
    ESP_ERROR_CHECK(app_heartbeat_init());

    /* 阶段 V：语音控制（播放/停止/音量/预览；需在 modbus 之后） */
    ESP_ERROR_CHECK(voice_init());

    /* 阶段 C：治疗控制（处方3疗程1 下发 + 30min 会话；需在 modbus 之后） */
    ESP_ERROR_CHECK(app_therapy_init());

    /* 0.D ST7365 显示驱动：初始化（清屏黑 + 背光亮，硬件横屏 480x320） */
    ESP_ERROR_CHECK(app_lcd_init());

    /* 1.5 LVGL 初始化：esp_lvgl_port 接入 ST7365（monitor 见 LVGL init OK） */
    ESP_ERROR_CHECK(app_lvgl_init());

    /* 阶段 B1：UI 创建用一次性 LVGL timer 延迟执行（放在 LVGL 任务上下文，
     * 避免 main 任务中同步加载 SPIFFS 图片阻塞系统→触发 task watchdog） */
    lv_timer_create(ui_init_timer_cb, 50, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
  * @brief  延迟 UI 初始化 timer 回调（在 LVGL 任务上下文中执行）
  *         一次性 timer：执行后自删除
  *         分两步（ui_init → ui_load）：先弹开机过渡画面（医疗蓝+logo+Starting...），
  *         让用户 ~1.8s 就看到画面；过渡画面由医疗蓝兜底（app_lcd_init 硬件刷屏 +
  *         app_lvgl_init 默认屏幕背景同为医疗蓝），加载期无白闪；再进入耗时加载。
  */
static void ui_init_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    /* 阶段 B1：'C' 盘 FS 驱动（同事 Windows 路径 → SPIFFS + 内存缓存）
     * 必须先于 chinese_font_init / Frame_ui / Menu_ui / splash logo（它们用 C:/ 路径） */
    app_fs_init();

    /* 先弹开机过渡画面：LVGL 任务本轮回调结束、渲染循环下一轮即把它刷上屏，
     * 背光(已由 app_lcd_init 点亮)下用户立即可见，无需黑屏干等。 */
    app_boot_splash_show();

    /* 让出 LVGL 任务：确保过渡画面至少先 flush 上屏一帧，再进入同步耗时加载
     * （否则 splash 与随后的 4~6s 加载在同一个 lv_timer_handler 里，永远轮不到刷新） */
    lv_timer_create(ui_load_timer_cb, 30, NULL);

    ESP_LOGI(TAG, "boot splash shown, entering UI load");

    /* 一次性 timer：自删除 */
    lv_timer_del(timer);
}

/**
 * @brief  耗时 UI 加载回调（在 LVGL 任务上下文中执行）
 *         承接 ui_init_timer_cb：此时过渡画面已上屏，可放心同步加载。加载完成后
 *         隐藏过渡画面，露出主菜单。
 */
static void ui_load_timer_cb(lv_timer_t *timer)
{
    /* 阶段 B1.0：加载中文字库（同事 C:/ 路径，经 app_fs 翻译）+ 全局样式 */
    chinese_font_init();
    style_checked_init();

    /* 阶段 B1.3：LVGL keypad 输入设备 */
    app_keypad_init();

    /* 阶段 E：参数存储 —— 把 NVS 已存参数应用到 UI（语言 + 治疗参数） */
    lang_set((app_lang_t)app_params_get_lang());
    PosReh_Param_ApplySaved();

    /* 阶段 B1.1~B1.2：Frame 顶栏（建 g_Ui.page_container）+ 主菜单（[我们] 按语言进中文/英文） */
    Frame_ui();
    Ui_EnterMenu();

    /* 全局电极脱落提示弹窗（挂 lv_layer_top, 需在 Frame/keypad/菜单之后） */
    elec_off_ui_init();

    /* 主菜单已构建完，隐藏开机过渡画面，露出主菜单 */
    app_boot_splash_hide();

    ESP_LOGI(TAG, "UI init complete (Frame + Menu + keypad)");

    /* 一次性 timer：自删除 */
    lv_timer_del(timer);
}
