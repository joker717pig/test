/**
  ******************************************************************************
  * @文件名称   app_lcd.c
  * @文件描述   ST7365 显示驱动实现（阶段 0.D，LVGL-agnostic）
  *            自定义 panel 实现 esp_lcd_panel_t 接口：
  *              reset / init / del / draw_bitmap / mirror / swap_xy /
  *              set_gap / invert_color / disp_on_off / disp_sleep
  *            初始化序列移植自 d:\GitLab\EMG\code\CLCD.c (金逸晨 ST7365 例程)
  * @说明
  *            - 像素格式 RGB565，字节序 MSB first（大端）
  *            - MADCTL=0xE8：MX|MY|MV|BGR 硬件横屏 480x320
  *            - draw_bitmap 逻辑坐标：x(0..479)->CASET, y(0..319)->RASET
  *            - trans_queue_depth=4：tx_color 队列+SPI 中断异步完成，4 块内部 RAM
  *              交换缓冲环形流水线；draw_bitmap 常态只排队不每帧排空（整帧最后一带
  *              DMA 结束由 on_color_trans_done → lv_disp_flush_ready 通知 LVGL）。
  *              ⚠ OTA 擦写内部 flash 会关 cache/屏蔽非 IRAM 中断，而 LCD SPI 完成中断
  *              与回调链非 IRAM，在途 flush 的完成通知会丢失 → 显示永久冻死。正解：
  *              擦 flash 前 app_lvgl_begin_flash_op()（停渲染 + app_lcd_drain 排空 DMA），
  *              擦完 app_lvgl_end_flash_op()。详见 doc 0D 坑 #9。
  ******************************************************************************
  */

#include <string.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_commands.h"
#include "esp_lcd_panel_dev.h"
#include "esp_lcd_panel_interface.h"
#include "sdkconfig.h"
#include "app_lcd.h"
#include "app_log.h"

static const char *TAG = "app_lcd";

/* ---------------- ST7365 自定义 panel ---------------- */

#define ST7365_MADCTL_LANDSCAPE  0xE8   /* MX|MY|MV|BGR：硬件横屏 480x320, 180°旋转 */

typedef struct {
    esp_lcd_panel_t base;            /* 必须为第一个成员（容器宏依赖） */
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    int x_gap;
    int y_gap;
    uint8_t madctl_val;
} st7365_panel_t;

#define ST7365_FROM(panel) ((st7365_panel_t *)((char *)(panel) - offsetof(st7365_panel_t, base)))

/* 大端交换缓冲：环形 4 块，每块 8 行（480*8*2=7680B，内部 RAM BSS，共 ~30KB）。
 * 配合 trans_queue_depth=4 形成流水线：CPU 转字节序 与 SPI DMA 传输重叠。
 * 环形安全性：esp_lcd 的 tx_color 在队列满(=4)时按 FIFO get_trans_result 回收最旧
 * 事务（等其 DMA 完成）才复用槽位；DMA 按序完成，因此写 buf[i%4] 时 buf[i%4] 的上一次
 * DMA 必已结束，跨 draw_bit_map 调用同理。颜色事务异步，最后一带结束由 SPI 中断
 * on_color_trans_done 通知 LVGL（draw_bitmap 不阻塞排空）。 */
#define ST7365_BAND_ROWS     8
#define ST7365_BAND_BUF_NUM  4
static uint8_t s_swap_buf[ST7365_BAND_BUF_NUM][CONFIG_LCD_H_RES * ST7365_BAND_ROWS * 2];

static esp_err_t st7365_del(esp_lcd_panel_t *panel);
static esp_err_t st7365_reset(esp_lcd_panel_t *panel);
static esp_err_t st7365_init(esp_lcd_panel_t *panel);
static esp_err_t st7365_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start,
                                    int x_end, int y_end, const void *color_data);
static esp_err_t st7365_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y);
static esp_err_t st7365_swap_xy(esp_lcd_panel_t *panel, bool swap_axes);
static esp_err_t st7365_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap);
static esp_err_t st7365_invert_color(esp_lcd_panel_t *panel, bool invert_color_data);
static esp_err_t st7365_disp_on_off(esp_lcd_panel_t *panel, bool on_off);
static esp_err_t st7365_sleep(esp_lcd_panel_t *panel, bool sleep);

/* 便捷：发一条命令（可带参数） */
#define TX_PARAM(io, cmd, param, size) \
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param((io), (cmd), (param), (size)), \
                        TAG, "tx param 0x%02X failed", (cmd))

static esp_err_t st7365_new_panel(esp_lcd_panel_io_handle_t io,
                                  const esp_lcd_panel_dev_config_t *dev_config,
                                  esp_lcd_panel_handle_t *ret_panel)
{
    st7365_panel_t *p = calloc(1, sizeof(st7365_panel_t));
    if (p == NULL) {
        return ESP_ERR_NO_MEM;
    }
    if (dev_config->reset_gpio_num >= 0) {
        gpio_config_t io_conf = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << dev_config->reset_gpio_num,
        };
        gpio_config(&io_conf);
    }
    p->io = io;
    p->reset_gpio_num = dev_config->reset_gpio_num;
    p->madctl_val = ST7365_MADCTL_LANDSCAPE;   /* 固定硬件横屏 */
    p->base.del = st7365_del;
    p->base.reset = st7365_reset;
    p->base.init = st7365_init;
    p->base.draw_bitmap = st7365_draw_bitmap;
    p->base.mirror = st7365_mirror;
    p->base.swap_xy = st7365_swap_xy;
    p->base.set_gap = st7365_set_gap;
    p->base.invert_color = st7365_invert_color;
    p->base.disp_on_off = st7365_disp_on_off;
    p->base.disp_sleep = st7365_sleep;
    *ret_panel = &p->base;
    return ESP_OK;
}

static esp_err_t st7365_del(esp_lcd_panel_t *panel)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    if (p->reset_gpio_num >= 0) {
        gpio_reset_pin(p->reset_gpio_num);
    }
    free(p);
    return ESP_OK;
}

static esp_err_t st7365_reset(esp_lcd_panel_t *panel)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    if (p->reset_gpio_num >= 0) {
        /* 复位时序：CS=H, RST=L 20ms -> RST=H 200ms（同 CLCD.c） */
        gpio_set_level(p->reset_gpio_num, 0);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(p->reset_gpio_num, 1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    return ESP_OK;
}

static esp_err_t st7365_init(esp_lcd_panel_t *panel)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    esp_lcd_panel_io_handle_t io = p->io;

    /* ---- 进入扩展命令集 ---- */
    TX_PARAM(io, 0xF0, ((uint8_t[]){0xC3}), 1);   /* Command Set Control Part I */
    TX_PARAM(io, 0xF0, ((uint8_t[]){0x96}), 1);   /* Command Set Control Part II */

    /* ---- 显示功能设置 ---- */
    TX_PARAM(io, 0xB4, ((uint8_t[]){0x01}), 1);   /* Display Function Control */
    TX_PARAM(io, 0xB7, ((uint8_t[]){0xC6}), 1);   /* Gate Control */
    TX_PARAM(io, 0xB9, ((uint8_t[]){0x02, 0xE0}), 2); /* VCOM Control */

    /* ---- 电源控制 ---- */
    TX_PARAM(io, 0xC0, ((uint8_t[]){0x00, 0x00}), 2); /* Power Control 1 */
    TX_PARAM(io, 0xC1, ((uint8_t[]){0x1B}), 1);       /* Power Control 2 */
    TX_PARAM(io, 0xC2, ((uint8_t[]){0xA7}), 1);       /* Power Control 3 */
    TX_PARAM(io, 0xC5, ((uint8_t[]){0x00}), 1);       /* VCOM Control */

    /* ---- 驱动时序控制 ---- */
    TX_PARAM(io, 0xE8, ((uint8_t[]){0x40, 0x8A, 0x00, 0x00, 0x33, 0x19, 0xA5, 0x33}), 8);

    /* ---- 正 Gamma 校正 ---- */
    TX_PARAM(io, 0xE0, ((uint8_t[]){0xF0, 0x09, 0x0F, 0x0D, 0x0D, 0x1C, 0x3D, 0x44,
                                    0x55, 0x39, 0x18, 0x18, 0x36, 0x39}), 14);
    /* ---- 负 Gamma 校正 ---- */
    TX_PARAM(io, 0xE1, ((uint8_t[]){0xF0, 0x09, 0x0F, 0x09, 0x08, 0x01, 0x31, 0x33,
                                    0x46, 0x09, 0x13, 0x13, 0x2A, 0x31}), 14);

    /* ---- 退出扩展命令集 ---- */
    TX_PARAM(io, 0xF0, ((uint8_t[]){0x3C}), 1);   /* Disable Command Set Part I */
    TX_PARAM(io, 0xF0, ((uint8_t[]){0x69}), 1);   /* Disable Command Set Part II */
    vTaskDelay(pdMS_TO_TICKS(120));

    /* ---- 标准命令 ---- */
    TX_PARAM(io, LCD_CMD_SLPOUT, NULL, 0);      /* Sleep Out */
    vTaskDelay(pdMS_TO_TICKS(120));

    TX_PARAM(io, LCD_CMD_MADCTL, ((uint8_t[]){p->madctl_val}), 1); /* 方向 */
    TX_PARAM(io, LCD_CMD_COLMOD, ((uint8_t[]){0x05}), 1);          /* RGB565 */
    TX_PARAM(io, LCD_CMD_TEON, ((uint8_t[]){0x00}), 1);            /* TE on (vsync 预留) */
    TX_PARAM(io, LCD_CMD_INVON, NULL, 0);                        /* 反色 on */
    TX_PARAM(io, LCD_CMD_DISPON, NULL, 0);                       /* 显示 on */
    vTaskDelay(pdMS_TO_TICKS(50));

    return ESP_OK;
}

static esp_err_t st7365_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start,
                                    int x_end, int y_end, const void *color_data)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    esp_lcd_panel_io_handle_t io = p->io;

    x_start += p->x_gap;
    x_end   += p->x_gap;
    y_start += p->y_gap;
    y_end   += p->y_gap;

    /* 窗口：x->CASET, y->RASET（同旧工程 lv_port flush，硬件横屏） */
    TX_PARAM(io, LCD_CMD_CASET, ((uint8_t[]){(x_start >> 8) & 0xFF, x_start & 0xFF,
                                             ((x_end - 1) >> 8) & 0xFF, (x_end - 1) & 0xFF}), 4);
    TX_PARAM(io, LCD_CMD_RASET, ((uint8_t[]){(y_start >> 8) & 0xFF, y_start & 0xFF,
                                             ((y_end - 1) >> 8) & 0xFF, (y_end - 1) & 0xFF}), 4);
    const uint16_t *src = (const uint16_t *)color_data;
    int width  = x_end - x_start;
    int height = y_end - y_start;
    bool first = true;
    int band_idx = 0;

    /* 按带处理 + 环形缓冲流水线：把主机序 uint16_t（小端）转面板 MSB first（大端）。
     * 队列深 4：最多 4 带在途，CPU 转下一带时 DMA 在传上一带 */
    for (int yy = 0; yy < height; yy += ST7365_BAND_ROWS) {
        int rows = (height - yy < ST7365_BAND_ROWS) ? (height - yy) : ST7365_BAND_ROWS;
        uint8_t *dst = s_swap_buf[band_idx % ST7365_BAND_BUF_NUM];
        for (int r = 0; r < rows; r++) {
            const uint16_t *line = src + (size_t)(yy + r) * width;
            for (int cc = 0; cc < width; cc++) {
                uint16_t px = line[cc];
                *dst++ = (uint8_t)(px >> 8);   /* MSB first */
                *dst++ = (uint8_t)(px & 0xFF);
            }
        }
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(io, first ? LCD_CMD_RAMWR : -1,
                                                      s_swap_buf[band_idx % ST7365_BAND_BUF_NUM],
                                                      (size_t)rows * width * 2),
                            TAG, "tx color failed");
        first = false;
        band_idx++;
    }
    /* 不在此每帧排空/阻塞等待：颜色事务走"队列+SPI 中断"异步完成，整帧最后一带 DMA 结束
     * 时由 SPI 中断回调 on_color_trans_done → lv_disp_flush_ready 通知 LVGL（esp_lvgl_port
     * 对 SPI 面板不在 flush_cb 末尾主动 ready，全靠该回调）。每帧加 NOP polling 排空只会
     * 让 LVGL 任务多一次总线阻塞、拖慢帧率（见 doc 0D 坑 #8），故常态不排空。
     * 缓冲安全：s_swap_buf 环形 4 槽与 trans_queue_depth=4 对齐，tx_color 在队列满时按
     * FIFO get_trans_result 回收最旧事务（等其 DMA 完成）才复用槽位；DMA 按序完成，故写
     * buf[i%4] 时其上一次 DMA 必已结束，跨 draw_bitmap 调用同理。
     *
     * ⚠️ OTA 冻死真根因不在此（曾误以为是每帧 NOP 排空，删掉后真机仍冻死）：擦写内部
     * flash 会关 cache/屏蔽非 IRAM 中断，而 LCD 的 SPI 完成中断与回调链（esp_lcd post_cb、
     * esp_lvgl_port flush_ready 回调）都不在 IRAM，SPI LCD 也无 IRAM 安全配置；关 cache
     * 窗口里若有在途 flush，其完成通知丢失 → LVGL 刷新永久冻死。正解是"擦 flash 前统一
     * 静默 LCD"：app_lvgl_begin_flash_op()（取 LVGL 锁停渲染 + app_lcd_drain 排空在途 DMA），
     * 擦完 app_lvgl_end_flash_op() 恢复。OTA 图片/ESP32/STM32 落盘段已包裹。详见
     * doc/阶段0_笔记/0D_ST7365显示驱动.md 坑 #9。 */
    return ESP_OK;
}

static esp_err_t st7365_invert_color(esp_lcd_panel_t *panel, bool invert_color_data)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    return esp_lcd_panel_io_tx_param(p->io, invert_color_data ? LCD_CMD_INVON : LCD_CMD_INVOFF, NULL, 0);
}

static esp_err_t st7365_mirror(esp_lcd_panel_t *panel, bool mirror_x, bool mirror_y)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    if (mirror_x) p->madctl_val |= LCD_CMD_MX_BIT; else p->madctl_val &= ~LCD_CMD_MX_BIT;
    if (mirror_y) p->madctl_val |= LCD_CMD_MY_BIT; else p->madctl_val &= ~LCD_CMD_MY_BIT;
    return esp_lcd_panel_io_tx_param(p->io, LCD_CMD_MADCTL, (uint8_t[]){p->madctl_val}, 1);
}

static esp_err_t st7365_swap_xy(esp_lcd_panel_t *panel, bool swap_axes)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    if (swap_axes) p->madctl_val |= LCD_CMD_MV_BIT; else p->madctl_val &= ~LCD_CMD_MV_BIT;
    return esp_lcd_panel_io_tx_param(p->io, LCD_CMD_MADCTL, (uint8_t[]){p->madctl_val}, 1);
}

static esp_err_t st7365_set_gap(esp_lcd_panel_t *panel, int x_gap, int y_gap)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    p->x_gap = x_gap;
    p->y_gap = y_gap;
    return ESP_OK;
}

static esp_err_t st7365_disp_on_off(esp_lcd_panel_t *panel, bool on_off)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    return esp_lcd_panel_io_tx_param(p->io, on_off ? LCD_CMD_DISPON : LCD_CMD_DISPOFF, NULL, 0);
}

static esp_err_t st7365_sleep(esp_lcd_panel_t *panel, bool sleep)
{
    st7365_panel_t *p = ST7365_FROM(panel);
    esp_lcd_panel_io_tx_param(p->io, sleep ? LCD_CMD_SLPIN : LCD_CMD_SLPOUT, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

/* ---------------- 应用层 ---------------- */

static spi_host_device_t s_spi_host = (spi_host_device_t)CONFIG_LCD_SPI_HOST_ID;
static esp_lcd_panel_io_handle_t s_io = NULL;
static esp_lcd_panel_handle_t s_panel = NULL;
static bool s_init = false;

esp_err_t app_lcd_init(void)
{
    if (s_init) {
        return ESP_OK;
    }

    /* SPI 总线 */
    spi_bus_config_t buscfg = {
        .sclk_io_num = CONFIG_LCD_PIN_SCK,
        .mosi_io_num = CONFIG_LCD_PIN_MOSI,
        .miso_io_num = CONFIG_LCD_PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CONFIG_LCD_H_RES * ST7365_BAND_ROWS * 2,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(s_spi_host, &buscfg, SPI_DMA_CH_AUTO),
                        TAG, "spi bus init failed");

    /* panel IO（CS/DC，时钟 40MHz，Mode0） */
    esp_lcd_panel_io_spi_config_t io_config = {
        .cs_gpio_num = CONFIG_LCD_PIN_CS,
        .dc_gpio_num = CONFIG_LCD_PIN_DC,
        .spi_mode = 0,
        .pclk_hz = CONFIG_LCD_SPI_CLK_HZ,
        .trans_queue_depth = ST7365_BAND_BUF_NUM,  /* 4 带流水线：CPU 转字节序 与 DMA 传输重叠 */
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)s_spi_host,
                                                 &io_config, &s_io),
                        TAG, "new panel io failed");

    /* ST7365 面板 */
    esp_lcd_panel_dev_config_t dev_config = {
        .reset_gpio_num = CONFIG_LCD_PIN_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(st7365_new_panel(s_io, &dev_config, &s_panel),
                        TAG, "new st7365 panel failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(s_panel), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(s_panel), TAG, "panel init failed");

    app_lcd_fill(APP_LCD_COLOR_MED_BLUE);  /* 清屏：医疗蓝开机过渡底色（不是白，杜绝白闪） */
    app_lcd_backlight(true);               /* 立即点亮背光：用户 ~1.8s 即看到过渡画面，不再黑屏干等 */
    s_init = true;

    ESP_LOGI(TAG, "ST7365 LCD init OK (%dx%d @ %d Hz, SPI2)",
             CONFIG_LCD_H_RES, CONFIG_LCD_V_RES, CONFIG_LCD_SPI_CLK_HZ);
    return ESP_OK;
}

esp_lcd_panel_io_handle_t app_lcd_get_io(void)
{
    return s_io;
}

esp_lcd_panel_handle_t app_lcd_get_panel(void)
{
    return s_panel;
}

void app_lcd_backlight(bool on)
{
    if (CONFIG_LCD_PIN_BL < 0) {
        return;
    }
    static bool configured = false;
    if (!configured) {
        gpio_config_t cfg = {
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << CONFIG_LCD_PIN_BL,
        };
        gpio_config(&cfg);
        configured = true;
    }
    gpio_set_level(CONFIG_LCD_PIN_BL, on ? 1 : 0);
}

esp_err_t app_lcd_fill(uint16_t color)
{
    /* 恒定颜色：预构建一条大端带，整屏连续复用发送（不再逐带排空） */
    static uint8_t band[CONFIG_LCD_H_RES * ST7365_BAND_ROWS * 2];
    if (s_io == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t hi = (uint8_t)(color >> 8), lo = (uint8_t)(color & 0xFF);
    size_t px = CONFIG_LCD_H_RES * ST7365_BAND_ROWS;
    for (size_t i = 0; i < px; i++) {
        band[i * 2] = hi;      /* MSB first */
        band[i * 2 + 1] = lo;
    }

    /* 全屏窗口设置一次 */
    TX_PARAM(s_io, LCD_CMD_CASET, ((uint8_t[]){0, 0, (CONFIG_LCD_H_RES - 1) >> 8, (CONFIG_LCD_H_RES - 1) & 0xFF}), 4);
    TX_PARAM(s_io, LCD_CMD_RASET, ((uint8_t[]){0, 0, (CONFIG_LCD_V_RES - 1) >> 8, (CONFIG_LCD_V_RES - 1) & 0xFF}), 4);

    /* 流水线流式整屏：首带带 RAMWR，其余 -1；队列深 4 保持 DMA 持续忙 */
    bool first = true;
    for (int y = 0; y < CONFIG_LCD_V_RES; y += ST7365_BAND_ROWS) {
        int rows = CONFIG_LCD_V_RES - y;
        if (rows > ST7365_BAND_ROWS) {
            rows = ST7365_BAND_ROWS;
        }
        ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_color(s_io, first ? LCD_CMD_RAMWR : -1,
                                                      band, (size_t)rows * CONFIG_LCD_H_RES * 2),
                            TAG, "fill tx color failed");
        first = false;
    }
    /* 尾部一次排空 */
    ESP_RETURN_ON_ERROR(esp_lcd_panel_io_tx_param(s_io, LCD_CMD_NOP, NULL, 0), TAG, "fill drain failed");
    return ESP_OK;
}

esp_err_t app_lcd_drain(void)
{
    if (s_io == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    /* 一次 polling NOP：esp_lcd 的 tx_param 会先 get_trans_result 回收所有在途
     * 排队事务（等每条 DMA 完成、ISR 已执行 post_cb → on_color_trans_done →
     * lv_disp_flush_ready），再发命令。返回时总线空闲、无 DMA 在飞、本帧 flush
     * 完成通知已投递。用于擦写内部 flash 前静默 LCD（见 app_lvgl_flash_safe_pause）。 */
    return esp_lcd_panel_io_tx_param(s_io, LCD_CMD_NOP, NULL, 0);
}

/* 8 条彩条 */
static void draw_color_bars(void)
{
    static uint16_t line[CONFIG_LCD_H_RES];
    const uint16_t bars[8] = {
        APP_LCD_COLOR_RED, APP_LCD_COLOR_GREEN, APP_LCD_COLOR_BLUE, APP_LCD_COLOR_WHITE,
        APP_LCD_COLOR_BLACK, APP_LCD_COLOR_YELLOW, APP_LCD_COLOR_CYAN, APP_LCD_COLOR_MAGENTA,
    };
    int bar_w = CONFIG_LCD_H_RES / 8;
    for (int y = 0; y < CONFIG_LCD_V_RES; y++) {
        for (int x = 0; x < CONFIG_LCD_H_RES; x++) {
            line[x] = bars[x / bar_w];
        }
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, CONFIG_LCD_H_RES, y + 1, line);
    }
}

/* 红->黑 横向渐变 + 绿 纵向渐变 */
static void draw_gradient(void)
{
    static uint16_t line[CONFIG_LCD_H_RES];
    for (int y = 0; y < CONFIG_LCD_V_RES; y++) {
        uint8_t g = (y * 255) / (CONFIG_LCD_V_RES - 1);
        for (int x = 0; x < CONFIG_LCD_H_RES; x++) {
            uint8_t r = (x * 255) / (CONFIG_LCD_H_RES - 1);
            line[x] = (uint16_t)(((uint16_t)(r & 0xF8) << 8) | ((uint16_t)(g & 0xFC) << 3));
        }
        esp_lcd_panel_draw_bitmap(s_panel, 0, y, CONFIG_LCD_H_RES, y + 1, line);
    }
}

void app_lcd_bare_test(void)
{
    if (s_panel == NULL) {
        ESP_LOGE(TAG, "LCD not initialized");
        return;
    }
    ESP_LOGI(TAG, "bare test: black -> white -> red -> color bars -> gradient");

    app_lcd_fill(APP_LCD_COLOR_BLACK);   vTaskDelay(pdMS_TO_TICKS(300));
    app_lcd_fill(APP_LCD_COLOR_WHITE);   vTaskDelay(pdMS_TO_TICKS(300));
    app_lcd_fill(APP_LCD_COLOR_RED);     vTaskDelay(pdMS_TO_TICKS(300));

    draw_color_bars();                   vTaskDelay(pdMS_TO_TICKS(800));
    draw_gradient();                     /* 停在渐变 */
    ESP_LOGI(TAG, "bare test done (final: gradient)");
}

void app_lcd_bench(void)
{
    if (s_panel == NULL) {
        ESP_LOGE(TAG, "LCD not initialized");
        return;
    }
    const int n = 30;
    int64_t t0 = esp_timer_get_time();
    for (int i = 0; i < n; i++) {
        app_lcd_fill((i & 1) ? APP_LCD_COLOR_RED : APP_LCD_COLOR_BLUE);
    }
    int64_t dt = esp_timer_get_time() - t0;
    double ms = (double)dt / n / 1000.0;
    ESP_LOGI(TAG, "bench: %d full-screen fills in %lld ms -> %.2f ms/frame (~%.1f fps) @ %d Hz",
             n, (long long)dt, ms, 1000.0 / ms, CONFIG_LCD_SPI_CLK_HZ);
}
