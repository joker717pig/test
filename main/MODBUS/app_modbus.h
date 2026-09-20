/**
  ******************************************************************************
  * @文件名称   app_modbus.h
  * @文件描述   EDA_EMG_ESP32 阶段 A：Modbus RTU 主站驱动接口
  *            - 请求/响应事务：FC 03 / 06 / 10（超时 + 重试）
  *            - 主动推送监听：按键帧 → INPUT_EVENT；采集帧 → 环形缓冲
  *            - 采集期间（push 模式）事务被拒绝（对齐 PC「采集期间不发请求」）
  ******************************************************************************
  */

#ifndef APP_MODBUS_H
#define APP_MODBUS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 返回码 ---- */
typedef enum {
    MB_OK            = 0,   /* 成功                       */
    MB_ERR_NOT_READY = -1,  /* 未初始化                   */
    MB_ERR_BUSY      = -2,  /* 事务忙 / push 模式被拒绝   */
    MB_ERR_TIMEOUT   = -3,  /* 响应超时                   */
    MB_ERR_CRC       = -4,  /* 响应 CRC 错误              */
    MB_ERR_EXCEPTION = -5,  /* 从站异常响应               */
    MB_ERR_RESP      = -6,  /* 响应格式不匹配             */
    MB_ERR_TX        = -7,  /* 发送失败                   */
    MB_ERR_PARAM     = -8,  /* 参数错误                   */
} mb_err_t;

/* 采集样本（来自推送帧，big-endian int16） */
typedef struct {
    int16_t ch1;
    int16_t ch2;
} mb_sample_t;

/* 采集样本环形缓冲容量 */
#define MB_ACQ_RING_CAP 4096

/**
  * @brief  初始化 Modbus RTU 主站（UART 驱动 + RX 任务；从站参数走 Kconfig）
  * @note   需在事件循环初始化之后调用（推送解析会 post INPUT_EVENT）
  * @retval ESP_OK 成功
  */
esp_err_t app_modbus_init(void);

/** @brief 打印当前配置/运行状态（console `mb info` 用） */
void app_modbus_print_info(void);

/** @brief 返回码 → 可读字符串 */
const char *app_modbus_err_str(int e);

/* ---- 请求/响应事务（返回 mb_err_t） ---- */
int app_modbus_read_registers(uint16_t addr, uint16_t count, uint16_t *out);
int app_modbus_write_register(uint16_t addr, uint16_t value);
int app_modbus_write_registers(uint16_t addr, const uint16_t *values, uint16_t count);
/** @brief 写单个寄存器、不等待响应（fire-and-forget，用于语音等实时命令；
 *  采集 push 期间可直接下发、不发重试，避免响应被推流淹没导致超时重试连播两次） */
int app_modbus_write_register_nowait(uint16_t addr, uint16_t value);

/* ---- 主动推送监听 ---- */
esp_err_t app_modbus_start_push(uint8_t channel);   /* 1=CH1, 2=CH2, 3=双通道 */
esp_err_t app_modbus_stop_push(void);
bool app_modbus_push_active(void);
uint8_t app_modbus_push_channel(void);

/* ---- STM32 OTA 升级服务 (Boot 主动拉取) ---- */
void app_modbus_ota_serve_start(const uint8_t *fw, uint32_t size);
void app_modbus_ota_serve_stop(void);
bool app_modbus_ota_serve_active(void);
int  app_modbus_ota_done_status(void);   /* -1=未完成, 0=成功, 1=失败 */
uint32_t app_modbus_ota_progress(void);  /* [我们] 已下发 Boot 的最大字节数(拉块进度); serve 未激活=0 */

/**
 * @brief  距上次收到 STM32 有效帧(CRC 对+地址匹配, 含 OTA REQ/DONE/响应/推送)的毫秒数
 * @retval 静默毫秒数; 从未收到帧返回 INT64_MAX
 * @note   心跳据此做"总线静默超时才探活": OTA/采集期间总线活跃则不下发心跳
 */
int64_t app_modbus_ms_since_last_rx(void);

/* ---- 采集样本环形缓冲（阶段 D 波形显示用） ---- */
uint16_t app_modbus_acq_available(void);            /* 缓冲中可读样本数 */
uint16_t app_modbus_acq_read(int16_t *ch1, int16_t *ch2, uint16_t max); /* 读走并移除 */

/* ---- 电极脱落状态快照 (0x0301 位图, 供 UI 全局弹窗读取) ----
 * 采集期由 STM32 状态推帧(CNT=0xFFFE)写入, 非采集期由心跳 FC03 读 0x0301 写入。 */
bool app_modbus_get_elec_status(uint16_t *out);  /* 返回 false=尚未取得过有效状态 */
void app_modbus_set_elec_status(uint16_t value); /* 心跳读到寄存器后回填 */

/* ---- 调试 ---- */
void app_modbus_set_frame_log(bool on);
bool app_modbus_frame_log_enabled(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_MODBUS_H */
