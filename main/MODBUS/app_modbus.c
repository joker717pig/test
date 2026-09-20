/**
  ******************************************************************************
  * @文件名称   app_modbus.c
  * @文件描述   EDA_EMG_ESP32 阶段 A：Modbus RTU 主站驱动（手写轻量）
  *            手写主站（非 esp-modbus 组件），理由：
  *              - 对齐 STM32 从站手写风格（MB_Slave.c / MB_RegMap.c）
  *              - 协议含主动推送帧（采集 20ms + 按键帧），非标准请求/响应
  *            结构：
  *              - RX 任务：UART 字节 → 帧定界（空闲超时）→ CRC → 分发
  *                * 有挂起请求 → 作为响应给信号量（代际计数防陈旧响应竞态）
  *                * 无挂起请求 → 作为推送帧解析（按键帧→INPUT_EVENT / 采集帧→环形缓冲）
  *              - 事务层：FC 03/06/10，互斥串行 + 超时重试；push 模式拒绝事务
  *            参考：
  *              - PC 端 EDA_EMG_PC/src/modbus_client.py（minimalmodbus + 推送监听，已验证）
  *              - 下位机 EDA_EMG/code/MODBUS/MB_RegMap.h（寄存器定义）
  ******************************************************************************
  */

#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "esp_event.h"
#include "app_modbus.h"
#include "app_modbus_reg.h"
#include "app_events.h"

static const char *TAG = "app_modbus";

/* ---- 硬件/协议参数（Kconfig） ---- */
#define MB_UART_NUM     CONFIG_MODBUS_UART_NUM
#define MB_TX_PIN       CONFIG_MODBUS_UART_TX
#define MB_RX_PIN       CONFIG_MODBUS_UART_RX
#define MB_BAUD         CONFIG_MODBUS_BAUD
#define MB_SLAVE_ID     CONFIG_MODBUS_SLAVE_ID
#define MB_RESP_TIMEOUT_MS CONFIG_MODBUS_RESP_TIMEOUT_MS
#define MB_RETRIES      CONFIG_MODBUS_RETRIES
#define MB_FRAME_GAP_MS CONFIG_MODBUS_FRAME_GAP_MS

#define MB_RX_BUF_SIZE   512
#define MB_RESP_BUF_SIZE 256

/* ================================================================
 *  运行状态
 * ================================================================ */
static bool            s_init = false;
static bool            s_frame_log = false;     /* 默认关, 避免刷屏 (需调试用 mb log on 开启) */
static volatile bool   s_push_mode = false;
static volatile uint8_t s_push_channel = 3;

/* STM32 OTA 升级服务 (Boot 主动拉取) */
static volatile bool      s_ota_serve = false;   /* 服务模式 */
static const uint8_t     *s_ota_fw = NULL;        /* 预加载的 stm32 固件缓冲 */
static uint32_t           s_ota_fw_size = 0;
static volatile int       s_ota_done = -1;        /* -1 未完成, 0 成功, 1 失败 */
static volatile uint32_t  s_ota_served = 0;       /* [我们] 已下发最大字节偏移(进度) */

/* 最近一次收到 STM32 有效帧(CRC 对 + 地址匹配)的时刻 (esp_timer_get_time, us)。
   0 = 从未收到。供心跳做"总线静默超时才探活"判断: OTA 时 Boot 不断发 REQ、
   采集时 STM32 不断推数据, 总线一直活跃 → 无需再周期性下发心跳。 */
static volatile int64_t   s_last_rx_us = 0;

static SemaphoreHandle_t s_tx_mutex;    /* 串行化请求（同一时刻只有一个事务） */
static SemaphoreHandle_t s_resp_sem;    /* RX 任务收到响应时给信号量 */

/* 事务共享状态（RX 任务 ↔ 事务函数） */
static volatile bool     s_pending = false;      /* 有请求等待响应 */
static volatile uint8_t  s_pending_fc;           /* 期望响应功能码 */
static volatile uint16_t s_pending_bc;           /* 期望字节数（FC03 用） */
static volatile uint32_t s_resp_gen = 0;         /* 每次新响应 +1（防陈旧响应竞态） */
static volatile mb_err_t s_resp_err;
static uint8_t  s_resp_buf[MB_RESP_BUF_SIZE];
static volatile uint16_t s_resp_len;

/* RX 帧累积 */
static uint8_t  s_rx_buf[MB_RX_BUF_SIZE];
static uint16_t s_rx_len;

/* 采集样本环形缓冲（阶段 D 波形用） */
static mb_sample_t s_acq_ring[MB_ACQ_RING_CAP];
static volatile uint16_t s_acq_head;
static volatile uint16_t s_acq_count;

/* 电极脱落状态统一快照 (0x0301 位图, 0=正常, 非0=有电极脱落)。
   两条来源汇入同一快照, 供 UI 全局弹窗读取:
   - 采集期: STM32 周期推 CNT=0xFFFE 状态帧 → mb_parse_push 写入;
   - 非采集期: 心跳 FC03 读 0x0301 → app_modbus_set_elec_status 写入。 */
static volatile uint16_t s_elec_status = 0;
static volatile bool     s_elec_valid = false;

/* ================================================================
 *  CRC16 (Modbus 标准多项式 0xA001)
 * ================================================================ */
static uint16_t mb_crc16(const uint8_t *pData, uint16_t uiLen)
{
    uint16_t uiCrc = 0xFFFF;
    uint16_t i;
    for (i = 0; i < uiLen; i++)
    {
        uiCrc ^= (uint16_t)pData[i];
        for (uint8_t j = 0; j < 8; j++)
        {
            if (uiCrc & 0x0001)
                uiCrc = (uiCrc >> 1) ^ 0xA001;
            else
                uiCrc >>= 1;
        }
    }
    return uiCrc;
}

/** 在 buf[len] 处追加 CRC（低字节在前，Modbus 线序） */
static void mb_append_crc(uint8_t *buf, uint16_t len)
{
    uint16_t crc = mb_crc16(buf, len);
    buf[len]     = (uint8_t)(crc & 0xFF);
    buf[len + 1] = (uint8_t)(crc >> 8);
}

/* ================================================================
 *  调试：十六进制帧日志
 * ================================================================ */
static void mb_log_frame(const char *dir, const uint8_t *p, uint16_t len)
{
    char hex[128 * 3 + 4];
    uint16_t off = 0;

    if (!s_frame_log)
        return;

    while (off < len)
    {
        uint16_t n = len - off;
        char *q = hex;
        if (n > 128) n = 128;
        for (uint16_t i = 0; i < n; i++)
            q += sprintf(q, "%02X ", p[off + i]);
        if (off + n < len)
            q += sprintf(q, "...");
        ESP_LOGI(TAG, "%s(%d): %s", dir, len, hex);
        off += n;
    }
}

/* ================================================================
 *  采集样本环形缓冲
 * ================================================================ */
static void mb_ring_push(int16_t ch1, int16_t ch2)
{
    if (s_acq_count == MB_ACQ_RING_CAP)
    {
        /* 满：覆盖最旧 */
        s_acq_ring[s_acq_head].ch1 = ch1;
        s_acq_ring[s_acq_head].ch2 = ch2;
        s_acq_head = (s_acq_head + 1) % MB_ACQ_RING_CAP;
    }
    else
    {
        uint16_t idx = (s_acq_head + s_acq_count) % MB_ACQ_RING_CAP;
        s_acq_ring[idx].ch1 = ch1;
        s_acq_ring[idx].ch2 = ch2;
        s_acq_count++;
    }
}

/* ================================================================
 *  推送帧解析（无挂起请求时的 01 03 帧）
 *   帧格式: [01][03][BC][CNT_H][CNT_L][DATA...][CRCL][CRCH]
 *   CNT=0   → 按键帧: [01][03][02][00][00][KS_H][KS_L][CRCL][CRCH] (9B)
 *   CNT=1~50 → 采集帧: BC = 2 + CNT×bps（双通道4 / 单通道2）
 * ================================================================ */
static void mb_handle_keys(uint16_t ks)
{
    for (uint8_t b = 0; b < 9; b++)     /* bit0~8 = END/UP/RIGHT/LEFT/DOWN/ESC/PWR_OFF/PWR_ON/SHUTDOWN_STATE */
    {
        if (ks & (1u << b))
        {
            if (b == MB_KEY_PWR_OFF)
            {
                /* 关机键(运行态长按上报) → 发 PWR_OFF 给 STM32 (STM32 软关机并拉低 ESP32_EN 断电) */
                ESP_LOGI(TAG, "PWR_OFF key -> send PWR_OFF");
                app_modbus_write_register_nowait(MB_REG_PWR_OFF, 1);
                continue;
            }
            if (b == MB_KEY_PWR_ON)
            {
                /* 开机键(关机保持态长按上报) → esp_restart 开机 */
                ESP_LOGI(TAG, "PWR_ON key -> esp_restart (power on)");
                esp_restart();
                continue;
            }
            input_event_data_t ev = { .key_id = b, .arg = 0 };
            esp_event_post(INPUT_EVENT, INPUT_EVENT_BUTTON_PRESSED, &ev, sizeof(ev), 0);
            ESP_LOGI(TAG, "KEY bit=%u -> INPUT_EVENT_BUTTON_PRESSED", b);
        }
    }
}

static int16_t mb_be16(const uint8_t *p)
{
    return (int16_t)((uint16_t)((p[0] << 8) | p[1]));
}

static void mb_handle_data(const uint8_t *f, uint16_t len, uint16_t cnt)
{
    int16_t ch1[MB_REG_ACQ_DATA_MAX];
    int16_t ch2[MB_REG_ACQ_DATA_MAX];
    uint16_t i, off;

    if (cnt > MB_REG_ACQ_DATA_MAX)
        return;

    if (s_push_channel == 3)            /* 双通道：每样本 4B (CH1[2B]+CH2[2B]) */
    {
        if (len < 5u + cnt * 4u + 2u)
            return;
        for (i = 0; i < cnt; i++)
        {
            off = 5 + i * 4;
            ch1[i] = mb_be16(f + off);
            ch2[i] = mb_be16(f + off + 2);
        }
    }
    else if (s_push_channel == 1)       /* 单通道 CH1 */
    {
        if (len < 5u + cnt * 2u + 2u)
            return;
        for (i = 0; i < cnt; i++)
        {
            off = 5 + i * 2;
            ch1[i] = mb_be16(f + off);
            ch2[i] = 0;
        }
    }
    else if (s_push_channel == 2)       /* 单通道 CH2 */
    {
        if (len < 5u + cnt * 2u + 2u)
            return;
        for (i = 0; i < cnt; i++)
        {
            off = 5 + i * 2;
            ch1[i] = 0;
            ch2[i] = mb_be16(f + off);
        }
    }
    else
    {
        return;
    }

    for (i = 0; i < cnt; i++)
        mb_ring_push(ch1[i], ch2[i]);

    ESP_LOGD(TAG, "ACQ frame CNT=%u CH1[0]=%d CH2[0]=%d (ring=%u)",
             cnt, (int)ch1[0], (int)ch2[0], (unsigned)s_acq_count);
}

static void mb_ota_serve_req(const uint8_t *f, uint16_t len)
{
    static uint8_t resp[10 + MB_OTA_BLK_LEN + 2];
    uint32_t off;
    uint16_t blk_len;
    uint16_t crc;

    if (len < 10 || s_ota_fw == NULL)
        return;

    off     = ((uint32_t)f[2] << 24) | ((uint32_t)f[3] << 16) |
              ((uint32_t)f[4] << 8)  |  (uint32_t)f[5];
    blk_len = (uint16_t)((f[6] << 8) | f[7]);

    if (blk_len == 0 || blk_len > MB_OTA_BLK_LEN || (off + blk_len) > s_ota_fw_size)
    {
        ESP_LOGW(TAG, "[ota] bad REQ off=%lu len=%u (fw=%lu)",
                 (unsigned long)off, blk_len, (unsigned long)s_ota_fw_size);
        return;
    }

    resp[0] = MB_SLAVE_ID;
    resp[1] = MB_FC_OTA_DATA;
    resp[2] = (uint8_t)(off >> 24);
    resp[3] = (uint8_t)(off >> 16);
    resp[4] = (uint8_t)(off >> 8);
    resp[5] = (uint8_t)(off);
    resp[6] = (uint8_t)(blk_len >> 8);
    resp[7] = (uint8_t)(blk_len);
    memcpy(&resp[8], s_ota_fw + off, blk_len);

    crc = mb_crc16(resp, (uint16_t)(8 + blk_len));
    resp[8 + blk_len] = (uint8_t)(crc & 0xFF);
    resp[9 + blk_len] = (uint8_t)(crc >> 8);

    uart_write_bytes(MB_UART_NUM, resp, 10 + blk_len);

    /* [我们] 记录已下发最大偏移（Boot 顺序拉块，off+blk_len 单调递增；取 max 保险） */
    uint32_t served = off + blk_len;
    if (served > s_ota_served) s_ota_served = served;
}

static void mb_parse_push(const uint8_t *f, uint16_t len)
{
    if (f[0] != MB_SLAVE_ID || f[1] != MB_FC_READ_HOLDING_REGS)
    {
        ESP_LOGW(TAG, "unexpected push frame (id=0x%02X fc=0x%02X)", f[0], f[1]);
        return;
    }
    uint16_t cnt = (uint16_t)((f[3] << 8) | f[4]);
    if (cnt == 0)
    {
        /* 按键帧：KS = f[5..6] */
        uint16_t ks = (uint16_t)((f[5] << 8) | f[6]);
        mb_handle_keys(ks);
    }
    else if (cnt == MB_PUSH_CNT_ELEC)
    {
        /* 电极脱落状态帧 (采集期): [01][03][04][FF][FE][ELEC_H][ELEC_L][CRC] */
        if (len >= 7)
        {
            uint16_t elec = (uint16_t)((f[5] << 8) | f[6]);
            s_elec_status = elec;
            s_elec_valid = true;
            ESP_LOGD(TAG, "elec push status=0x%04X", elec);
        }
    }
    else if (cnt <= MB_REG_ACQ_DATA_MAX)
    {
        mb_handle_data(f, len, cnt);
    }
    else
    {
        ESP_LOGW(TAG, "invalid push CNT=%u", cnt);
    }
}

/* ================================================================
 *  RX 帧处理（CRC 校验 → 响应/推送分发）
 * ================================================================ */
static void mb_process_frame(const uint8_t *f, uint16_t len)
{
    uint16_t rx_crc, calc;

    if (len < 4)
        return;

    /* CRC 校验 */
    rx_crc = (uint16_t)((f[len - 1] << 8) | f[len - 2]);
    calc   = mb_crc16(f, len - 2);
    if (rx_crc != calc)
    {
        ESP_LOGW(TAG, "RX bad CRC (len=%u)", len);
        if (s_pending)
        {
            s_resp_err = MB_ERR_CRC;
            s_resp_gen++;
            xSemaphoreGive(s_resp_sem);
        }
        return;
    }
    mb_log_frame("RX", f, len);

    /* 从站地址过滤（含广播地址 0x00） */
    if (f[0] != MB_SLAVE_ID && f[0] != 0x00)
        return;

    /* 记录"总线活跃"时刻: 任何 CRC 对、地址匹配的帧(OTA REQ/DONE、响应、推送)都算。
       心跳据此判断总线是否静默, 静默超时才探活, 避免 OTA/采集期间无谓下发。 */
    s_last_rx_us = esp_timer_get_time();

    /* STM32 OTA 主动拉取帧 (Boot 主动发, 非本主站请求的响应) */
    if (f[1] == MB_FC_OTA_REQ)
    {
        if (s_ota_serve)
            mb_ota_serve_req(f, len);
        return;
    }
    if (f[1] == MB_FC_OTA_DONE)
    {
        if (len >= 5)
        {
            s_ota_done = (int)f[2];
            ESP_LOGI(TAG, "[ota] done status=%d", s_ota_done);
        }
        return;
    }

    if (s_pending)
    {
        /* 有挂起请求：先判是否为期望响应（功能码匹配） */
        if (f[1] == s_pending_fc)
        {
            if (len > MB_RESP_BUF_SIZE)
                len = MB_RESP_BUF_SIZE;
            memcpy(s_resp_buf, f, len);
            s_resp_len = len;
            s_resp_err = MB_OK;
            s_resp_gen++;
            xSemaphoreGive(s_resp_sem);
            return;
        }
        /* 功能码不匹配：当作推送帧处理，继续等响应 */
    }

    /* 无挂起请求 / 功能码不匹配 → 推送帧 */
    mb_parse_push(f, len);
}

/* ================================================================
 *  RX 任务：读字节 → 空闲定帧 → 处理
 * ================================================================ */
/* 追加字节到累积缓冲（溢出则丢整帧重新同步） */
static void mb_rx_append(const uint8_t *p, int n)
{
    for (int i = 0; i < n; i++)
    {
        if (s_rx_len < MB_RX_BUF_SIZE)
            s_rx_buf[s_rx_len++] = p[i];
        else
        {
            ESP_LOGW(TAG, "RX overflow, drop frame");
            s_rx_len = 0;
        }
    }
}

/* 从累积缓冲按「帧头 + 功能码长度规则」提取完整帧并处理
 * 读响应/推送帧: 01 03 BC...  → 3+BC+2
 * 写响应:        01 06/10 ... → 8
 * 异常响应:      01 [fc|0x80] → 5
 * 帧头地址不符 / 未知功能码 → 逐字节重同步（对齐 PC 的 01 03 定帧思路） */
static void mb_rx_drain(void)
{
    while (1)
    {
        uint8_t  fc;
        uint16_t need;

        if (s_rx_len < 4)
            return;

        if (s_rx_buf[0] != MB_SLAVE_ID && s_rx_buf[0] != 0x00)
            goto drop1;

        fc = s_rx_buf[1];
        if (fc == MB_FC_READ_HOLDING_REGS)
        {
            if (s_pending)
            {
                /* 读响应：BC 即数据字节数，长度 = 3+BC+2 */
                need = 3u + s_rx_buf[2] + 2u;
            }
            else
            {
                /* 推送帧：按键帧(CNT=0)多 2 字节 CNT 字段 → 9B；
                 * 采集帧(CNT>0) BC 含 CNT → 3+BC+2 */
                if (s_rx_len < 5)
                    return;
                if ((s_rx_buf[3] | s_rx_buf[4]) == 0)
                    need = 9;                       /* 按键帧 [01][03][02][00][00][KS_H][KS_L][CRC] */
                else
                    need = 3u + s_rx_buf[2] + 2u;   /* 采集帧 */
            }
        }
        else if (fc == MB_FC_WRITE_SINGLE_REG || fc == MB_FC_WRITE_MULTIPLE_REGS)
            need = 8;                       /* 写响应 */
        else if (fc == MB_FC_OTA_REQ)
            need = 10;                      /* Boot 请求块: 01 41 off(4) len(2) crc(2) */
        else if (fc == MB_FC_OTA_DONE)
            need = 5;                       /* Boot 完成通知: 01 43 status(1) crc(2) */
        else if ((fc & 0x80) && ((fc & 0x7F) == MB_FC_READ_HOLDING_REGS ||
                                 (fc & 0x7F) == MB_FC_WRITE_SINGLE_REG ||
                                 (fc & 0x7F) == MB_FC_WRITE_MULTIPLE_REGS))
            need = 5;                       /* 异常响应 */
        else
            goto drop1;                     /* 未知功能码：重同步 */

        if (need > 255)
            goto drop1;
        if (s_rx_len < need)
            return;                         /* 不完整，等更多字节 */

        {
            uint8_t frame[256];
            memcpy(frame, s_rx_buf, need);
            memmove(s_rx_buf, s_rx_buf + need, s_rx_len - need);
            s_rx_len -= need;
            mb_process_frame(frame, need);
        }
        continue;

    drop1:
        memmove(s_rx_buf, s_rx_buf + 1, s_rx_len - 1);
        s_rx_len--;
    }
}

/* RX 任务：读字节 → 累积 → 帧头+长度提取处理 */
static void mb_rx_task(void *arg)
{
    uint8_t tmp[64];
    (void)arg;

    while (1)
    {
        int n = uart_read_bytes(MB_UART_NUM, tmp, sizeof(tmp), pdMS_TO_TICKS(50));
        if (n > 0)
        {
            mb_rx_append(tmp, n);
            mb_rx_drain();
        }
    }
}

/* ================================================================
 *  事务层（请求/响应，互斥串行 + 超时重试）
 * ================================================================ */
static mb_err_t mb_transact(const uint8_t *req, uint16_t req_len,
                            uint8_t fc, uint16_t expect_bc,
                            uint8_t *resp, uint16_t *resp_len)
{
    mb_err_t last = MB_ERR_TIMEOUT;

    if (!s_init)
        return MB_ERR_NOT_READY;
    if (s_push_mode)
        return MB_ERR_BUSY;

    if (xSemaphoreTake(s_tx_mutex, pdMS_TO_TICKS(200)) != pdTRUE)
        return MB_ERR_BUSY;

    for (uint8_t retry = 0; retry <= MB_RETRIES; retry++)
    {
        uint32_t gen_before = s_resp_gen;

        s_pending    = true;
        s_pending_fc = fc;
        s_pending_bc = expect_bc;
        s_resp_err   = MB_OK;
        s_resp_len   = 0;

        mb_log_frame("TX", req, req_len);
        if (uart_write_bytes(MB_UART_NUM, req, req_len) != req_len)
        {
            s_pending = false;
            last = MB_ERR_TX;
            break;
        }

        /* 等待新鲜响应（代际计数过滤陈旧信号量） */
        mb_err_t got = MB_ERR_TIMEOUT;
        for (;;)
        {
            if (xSemaphoreTake(s_resp_sem, pdMS_TO_TICKS(MB_RESP_TIMEOUT_MS)) != pdTRUE)
            {
                got = MB_ERR_TIMEOUT;
                break;
            }
            if (s_resp_gen > gen_before)    /* 是本请求之后的新响应 */
            {
                got = s_resp_err;
                break;
            }
            /* 陈旧响应：丢弃计数，继续等 */
        }
        s_pending = false;

        if (got == MB_OK && s_resp_len > 0)
        {
            const uint8_t *p = s_resp_buf;
            if (p[0] == MB_SLAVE_ID && p[1] == fc)
            {
                if (expect_bc != 0 && p[2] != (uint8_t)expect_bc)
                {
                    last = MB_ERR_RESP;
                    continue;               /* 形状不符：重试 */
                }
                if (resp)
                    memcpy(resp, p, s_resp_len);
                if (resp_len)
                    *resp_len = s_resp_len;
                xSemaphoreGive(s_tx_mutex);
                return MB_OK;
            }
            if ((p[0] == MB_SLAVE_ID) && (p[1] & 0x80) && ((p[1] & 0x7F) == fc))
            {
                /* 从站异常响应（不重试） */
                last = MB_ERR_EXCEPTION;
                ESP_LOGW(TAG, "slave exception 0x%02X (fc=0x%02X)", p[2], fc);
                xSemaphoreGive(s_tx_mutex);
                return last;
            }
            last = MB_ERR_RESP;
            continue;
        }

        last = (got == MB_ERR_CRC) ? MB_ERR_CRC : MB_ERR_TIMEOUT;
        if (retry < MB_RETRIES)
            ESP_LOGW(TAG, "tx retry %u/%u fc=0x%02X err=%s",
                     retry + 1, MB_RETRIES, fc, app_modbus_err_str(last));
    }

    xSemaphoreGive(s_tx_mutex);
    return last;
}

/* ================================================================
 *  公开 API
 * ================================================================ */

esp_err_t app_modbus_init(void)
{
    uart_config_t cfg;

    if (s_init)
        return ESP_OK;

    memset(&cfg, 0, sizeof(cfg));
    cfg.baud_rate  = MB_BAUD;
    cfg.data_bits  = UART_DATA_8_BITS;
    cfg.parity     = UART_PARITY_DISABLE;
    cfg.stop_bits  = UART_STOP_BITS_1;
    cfg.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    cfg.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_driver_install(MB_UART_NUM, 512, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(MB_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(MB_UART_NUM, MB_TX_PIN, MB_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    uart_flush_input(MB_UART_NUM);

    s_tx_mutex = xSemaphoreCreateMutex();
    s_resp_sem = xSemaphoreCreateBinary();
    if (s_tx_mutex == NULL || s_resp_sem == NULL)
    {
        ESP_LOGE(TAG, "create semaphore failed");
        return ESP_ERR_NO_MEM;
    }
    s_init = true;

    if (xTaskCreate(mb_rx_task, "modbus", 4096, NULL, 6, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "create modbus rx task failed");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "MODBUS init OK (UART%d TX=%d RX=%d @%d 8N1, slave=0x%02X, timeout=%dms x%d)",
             MB_UART_NUM, MB_TX_PIN, MB_RX_PIN, MB_BAUD, MB_SLAVE_ID,
             MB_RESP_TIMEOUT_MS, MB_RETRIES + 1);
    return ESP_OK;
}

void app_modbus_print_info(void)
{
    printf("MODBUS RTU master\r\n");
    printf("  uart   : UART%d\r\n", MB_UART_NUM);
    printf("  pins   : TX=%d RX=%d\r\n", MB_TX_PIN, MB_RX_PIN);
    printf("  baud   : %d 8N1\r\n", MB_BAUD);
    printf("  slave  : 0x%02X\r\n", MB_SLAVE_ID);
    printf("  state  : %s\r\n", s_init ? "ready" : "not init");
    printf("  push   : %s (channel=%u)\r\n", s_push_mode ? "active" : "off",
           (uint8_t)s_push_channel);
    printf("  frame  : log %s\r\n", s_frame_log ? "on" : "off");
    printf("  acq    : %u/%u samples\r\n", (unsigned)s_acq_count, (unsigned)MB_ACQ_RING_CAP);
}

const char *app_modbus_err_str(int e)
{
    switch (e)
    {
    case MB_OK:            return "OK";
    case MB_ERR_NOT_READY: return "not initialized";
    case MB_ERR_BUSY:      return "busy (push mode)";
    case MB_ERR_TIMEOUT:   return "timeout";
    case MB_ERR_CRC:       return "crc error";
    case MB_ERR_EXCEPTION: return "slave exception";
    case MB_ERR_RESP:      return "bad response";
    case MB_ERR_TX:        return "tx failed";
    case MB_ERR_PARAM:     return "param error";
    default:               return "?";
    }
}

int app_modbus_read_registers(uint16_t addr, uint16_t count, uint16_t *out)
{
    uint8_t req[8], resp[MB_RESP_BUF_SIZE];
    uint16_t rlen = 0;
    int e;

    if (!out || count == 0 || count > 125)
        return MB_ERR_PARAM;

    req[0] = MB_SLAVE_ID;
    req[1] = MB_FC_READ_HOLDING_REGS;
    req[2] = (uint8_t)(addr >> 8);
    req[3] = (uint8_t)(addr & 0xFF);
    req[4] = (uint8_t)(count >> 8);
    req[5] = (uint8_t)(count & 0xFF);
    mb_append_crc(req, 6);

    e = mb_transact(req, 8, MB_FC_READ_HOLDING_REGS, (uint16_t)(count * 2), resp, &rlen);
    if (e != MB_OK)
        return e;

    for (uint16_t i = 0; i < count; i++)
        out[i] = (uint16_t)((resp[3 + i * 2] << 8) | resp[3 + i * 2 + 1]);
    return MB_OK;
}

int app_modbus_write_register(uint16_t addr, uint16_t value)
{
    uint8_t req[8], resp[8];
    uint16_t rlen = 0;

    req[0] = MB_SLAVE_ID;
    req[1] = MB_FC_WRITE_SINGLE_REG;
    req[2] = (uint8_t)(addr >> 8);
    req[3] = (uint8_t)(addr & 0xFF);
    req[4] = (uint8_t)(value >> 8);
    req[5] = (uint8_t)(value & 0xFF);
    mb_append_crc(req, 6);

    return mb_transact(req, 8, MB_FC_WRITE_SINGLE_REG, 0, resp, &rlen);
}

/* 仅发送请求帧（不等待响应）：语音等 fire-and-forget 命令用，
 * 避免响应被推流帧淹没导致超时重试 → 语音被连播两次 */
static mb_err_t mb_send_only(const uint8_t *req, uint16_t req_len)
{
    if (!s_init)
        return MB_ERR_NOT_READY;

    if (xSemaphoreTake(s_tx_mutex, pdMS_TO_TICKS(200)) != pdTRUE)
        return MB_ERR_BUSY;

    mb_log_frame("TX", req, req_len);
    if (uart_write_bytes(MB_UART_NUM, req, req_len) != req_len)
    {
        xSemaphoreGive(s_tx_mutex);
        return MB_ERR_TX;
    }
    xSemaphoreGive(s_tx_mutex);
    return MB_OK;
}

int app_modbus_write_register_nowait(uint16_t addr, uint16_t value)
{
    uint8_t req[8];
    req[0] = MB_SLAVE_ID;
    req[1] = MB_FC_WRITE_SINGLE_REG;
    req[2] = (uint8_t)(addr >> 8);
    req[3] = (uint8_t)(addr & 0xFF);
    req[4] = (uint8_t)(value >> 8);
    req[5] = (uint8_t)(value & 0xFF);
    mb_append_crc(req, 6);
    return mb_send_only(req, 8);
}

int app_modbus_write_registers(uint16_t addr, const uint16_t *values, uint16_t count)
{
    uint8_t req[9 + 123 * 2], resp[8];
    uint16_t rlen = 0;
    uint16_t i;

    if (!values || count == 0 || count > 123)
        return MB_ERR_PARAM;

    req[0] = MB_SLAVE_ID;
    req[1] = MB_FC_WRITE_MULTIPLE_REGS;
    req[2] = (uint8_t)(addr >> 8);
    req[3] = (uint8_t)(addr & 0xFF);
    req[4] = (uint8_t)(count >> 8);
    req[5] = (uint8_t)(count & 0xFF);
    req[6] = (uint8_t)(count * 2);
    for (i = 0; i < count; i++)
    {
        req[7 + i * 2]     = (uint8_t)(values[i] >> 8);
        req[7 + i * 2 + 1] = (uint8_t)(values[i] & 0xFF);
    }
    mb_append_crc(req, 7 + count * 2);

    /* 帧长 = 7(头) + count*2(数据) + 2(CRC)；漏 +2 会发不带 CRC 的帧 → 从站丢弃 → 超时 */
    return mb_transact(req, 9 + count * 2, MB_FC_WRITE_MULTIPLE_REGS, 0, resp, &rlen);
}

/* ---- 推送监听 ---- */
esp_err_t app_modbus_start_push(uint8_t channel)
{
    if (channel < 1 || channel > 3)
        return ESP_ERR_INVALID_ARG;

    s_push_mode = true;
    s_push_channel = channel;
    s_acq_head  = 0;
    s_acq_count = 0;
    ESP_LOGI(TAG, "push listener started (channel=%u)", channel);
    return ESP_OK;
}

esp_err_t app_modbus_stop_push(void)
{
    s_push_mode = false;
    ESP_LOGI(TAG, "push listener stopped");
    return ESP_OK;
}

bool app_modbus_push_active(void)
{
    return s_push_mode;
}

uint8_t app_modbus_push_channel(void)
{
    return (uint8_t)s_push_channel;
}

/* ---- 采集样本环形缓冲 ---- */
uint16_t app_modbus_acq_available(void)
{
    return (uint16_t)s_acq_count;
}

uint16_t app_modbus_acq_read(int16_t *ch1, int16_t *ch2, uint16_t max)
{
    uint16_t n = 0;

    while (n < max && s_acq_count > 0)
    {
        ch1[n] = s_acq_ring[s_acq_head].ch1;
        ch2[n] = s_acq_ring[s_acq_head].ch2;
        s_acq_head = (s_acq_head + 1) % MB_ACQ_RING_CAP;
        s_acq_count--;
        n++;
    }
    return n;
}

/* ---- 电极脱落状态快照 (供 UI 全局弹窗读取) ---- */
bool app_modbus_get_elec_status(uint16_t *out)
{
    if (!s_elec_valid)
        return false;
    *out = (uint16_t)s_elec_status;
    return true;
}

/* 非采集期由心跳 FC03 读到 0x0301 后写入 (采集期由状态推帧写入, 二者汇同一快照) */
void app_modbus_set_elec_status(uint16_t value)
{
    s_elec_status = value;
    s_elec_valid = true;
}

/* ---- 调试 ---- */
void app_modbus_set_frame_log(bool on)
{
    s_frame_log = on;
    ESP_LOGI(TAG, "frame log %s", on ? "on" : "off");
}

bool app_modbus_frame_log_enabled(void)
{
    return s_frame_log;
}

/* ---- STM32 OTA 升级服务 (Boot 主动拉取) ---- */
void app_modbus_ota_serve_start(const uint8_t *fw, uint32_t size)
{
    s_ota_fw      = fw;
    s_ota_fw_size = size;
    s_ota_done    = -1;
    s_ota_served  = 0;
    s_ota_serve   = true;
    ESP_LOGI(TAG, "[ota] serve start (fw=%p size=%lu)", (void *)fw, (unsigned long)size);
}

void app_modbus_ota_serve_stop(void)
{
    s_ota_serve   = false;
    s_ota_fw      = NULL;
    s_ota_fw_size = 0;
    s_ota_served  = 0;
    ESP_LOGI(TAG, "[ota] serve stop");
}

bool app_modbus_ota_serve_active(void)
{
    return s_ota_serve;
}

int app_modbus_ota_done_status(void)
{
    return s_ota_done;
}

/* [我们] 已下发给 Boot 的最大字节数（拉块进度）；serve 未激活返回 0 */
uint32_t app_modbus_ota_progress(void)
{
    return s_ota_served;
}

/* 距上次收到 STM32 有效帧过了多久 (ms); 从未收到返回 INT64_MAX。
   供心跳做"静默超时才探活"判断。 */
int64_t app_modbus_ms_since_last_rx(void)
{
    int64_t last = s_last_rx_us;
    if (last == 0)
        return INT64_MAX;
    return (esp_timer_get_time() - last) / 1000;
}
