#ifndef VOICE_H
#define VOICE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ================================================================
 *  语音段地址（对齐唯创烧录工程 WTN6260624-35-V2.08-E，二线串口）
 *  完整清单见 EDA_EMG/doc/产品需求/语音段地址枚举.md
 * ================================================================ */
typedef enum {
    VOICE_ID_SILENCE_20MS        = 0x00,  /* 20ms 静音 (保留) */

    /* 开机设置 (中文) */
    VOICE_ID_SELECT_LANG_CN      = 0x01,  /* 请选择语言 */
    VOICE_ID_SET_DATE_CN         = 0x02,  /* 请设置日期 */
    VOICE_ID_SET_TIME_CN         = 0x03,  /* 请设置时间 */
    VOICE_ID_SETUP_DONE_CN       = 0x04,  /* 首次开机设置完成 */
    VOICE_ID_MAX_INTENSITY_CN    = 0x05,  /* 请...调至您能承受的最大值 */

    /* 引导语 (中文) */
    VOICE_ID_RELAX_ABDOMEN_CN    = 0x06,  /* 放松腹部 */
    VOICE_ID_QUICK_CONTRACT_CN   = 0x07,  /* 快收快放 */
    VOICE_ID_RELAX_CN            = 0x08,  /* 放松 */
    VOICE_ID_STAY_RELAX_CN       = 0x09,  /* 保持放松 */
    VOICE_ID_HOLD_CONTRACT_CN    = 0x0A,  /* 保持用力 */
    VOICE_ID_REG_BREATH_CN       = 0x0B,  /* 调整呼吸 */
    VOICE_ID_CONTRACT_6S_CN      = 0x0C,  /* 收缩并保持6秒 */
    VOICE_ID_QUICK_BY_GRAPH_CN   = 0x0D,  /* 按图形快收快放 */
    VOICE_ID_CONTRACT_HOLD_CN    = 0x0E,  /* 收缩并保持 */
    VOICE_ID_CONTRACT_CN         = 0x0F,  /* 用力 */
    VOICE_ID_BY_GRAPH_CN         = 0x10,  /* 按图形发力 */
    VOICE_ID_CONTRACT_GRAPH_CN   = 0x11,  /* 收缩并按图形发力 */
    VOICE_ID_CONTRACT2_CN        = 0x12,  /* 收缩 */

    /* 数字 1~10 (中文) */
    VOICE_ID_NUM1_CN             = 0x13,
    VOICE_ID_NUM2_CN             = 0x14,
    VOICE_ID_NUM3_CN             = 0x15,
    VOICE_ID_NUM4_CN             = 0x16,
    VOICE_ID_NUM5_CN             = 0x17,
    VOICE_ID_NUM6_CN             = 0x18,
    VOICE_ID_NUM7_CN             = 0x19,
    VOICE_ID_NUM8_CN             = 0x1A,
    VOICE_ID_NUM9_CN             = 0x1B,
    VOICE_ID_NUM10_CN            = 0x1C,

    /* 评估/通用 (中文) */
    VOICE_ID_BASELINE_EMG_CN     = 0x1D,  /* 采集腹部基础肌电位 */
    VOICE_ID_PEAK_EMG_CN         = 0x1E,  /* 采集盆底最大肌电位 */
    VOICE_ID_COLLECT_DONE_CN     = 0x1F,  /* 采集完成 */
    VOICE_ID_ASSESS_START_CN     = 0x20,  /* 开始评估 */
    VOICE_ID_NOW_CN              = 0x21,  /* 现在 */
    VOICE_ID_READY_CN            = 0x22,  /* 准备 */
    VOICE_ID_START_CN            = 0x23,  /* 开始 */
    VOICE_ID_FINISH_CN           = 0x24,  /* 结束 */
    VOICE_ID_DONE_CN             = 0x25,  /* 完成 */
    VOICE_ID_PAUSE_CN            = 0x26,  /* 暂停 */
    VOICE_ID_STOP_CN             = 0x27,  /* 停止 */
    VOICE_ID_NEXT_CN             = 0x28,  /* 下一步 */
    VOICE_ID_ELEC_OFF_CN         = 0x29,  /* 电极片掉落 */
    VOICE_ID_LINE_LOSS_CN        = 0x2A,  /* 线材脱落 */
    /* 0x29~0x2D 留空 */

    /* 开机设置 (英文) */
    VOICE_ID_SELECT_LANG_EN      = 0x2E,  /* Select language */
    VOICE_ID_SET_DATE_EN         = 0x2F,  /* Please set the date */
    VOICE_ID_SET_TIME_EN         = 0x30,  /* Please set the time */
    VOICE_ID_SETUP_DONE_EN       = 0x31,  /* First-time setup complete */
    VOICE_ID_MAX_INTENSITY_EN    = 0x32,  /* Increase the stimulation intensity ... */

    /* 引导语 (英文) */
    VOICE_ID_RELAX_ABDOMEN_EN    = 0x33,  /* Relax your abdomen */
    VOICE_ID_QUICK_CONTRACT_EN   = 0x34,  /* Quickly contract and release */
    VOICE_ID_RELAX_EN            = 0x35,  /* Relax */
    VOICE_ID_STAY_RELAX_EN       = 0x36,  /* Stay relaxed */
    VOICE_ID_HOLD_CONTRACT_EN    = 0x37,  /* Hold the contraction */
    VOICE_ID_REG_BREATH_EN       = 0x38,  /* Regulate your breathing */
    VOICE_ID_CONTRACT_6S_EN      = 0x39,  /* Contract and hold for 6 seconds */
    VOICE_ID_QUICK_BY_GRAPH_EN   = 0x3A,  /* Follow the graph - quickly contract and release */
    VOICE_ID_CONTRACT_HOLD_EN    = 0x3B,  /* Contract and hold */
    VOICE_ID_CONTRACT_EN         = 0x3C,  /* Contract */
    VOICE_ID_BY_GRAPH_EN         = 0x3D,  /* Contract following the graph */
    VOICE_ID_CONTRACT_GRAPH_EN   = 0x3E,  /* Contract and engage as shown on the graph */
    VOICE_ID_CONTRACT2_EN        = 0x3F,  /* Contract */

    /* 数字 1~10 (英文 one~ten) */
    VOICE_ID_NUM1_EN             = 0x40,  /* one  */
    VOICE_ID_NUM2_EN             = 0x41,  /* two  */
    VOICE_ID_NUM3_EN             = 0x42,  /* three */
    VOICE_ID_NUM4_EN             = 0x43,  /* four */
    VOICE_ID_NUM5_EN             = 0x44,  /* five */
    VOICE_ID_NUM6_EN             = 0x45,  /* six  */
    VOICE_ID_NUM7_EN             = 0x46,  /* seven */
    VOICE_ID_NUM8_EN             = 0x47,  /* eight */
    VOICE_ID_NUM9_EN             = 0x48,  /* nine */
    VOICE_ID_NUM10_EN            = 0x49,  /* ten  */

    /* 评估/通用 (英文) */
    VOICE_ID_BASELINE_EMG_EN     = 0x4A,  /* Collecting baseline abdominal EMG */
    VOICE_ID_PEAK_EMG_EN         = 0x4B,  /* Collecting peak pelvic floor EMG */
    VOICE_ID_COLLECT_DONE_EN     = 0x4C,  /* Collection complete */
    VOICE_ID_ASSESS_START_EN     = 0x4D,  /* Begin assessment */
    VOICE_ID_NOW_EN              = 0x4E,  /* Now */
    VOICE_ID_READY_EN            = 0x4F,  /* Ready */
    VOICE_ID_START_EN            = 0x50,  /* Start */
    VOICE_ID_FINISH_EN           = 0x51,  /* Finish */
    VOICE_ID_DONE_EN             = 0x52,  /* Done */
    VOICE_ID_PAUSE_EN            = 0x53,  /* Pause */
    VOICE_ID_STOP_EN             = 0x54,  /* Stop */
    VOICE_ID_NEXT_EN             = 0x55,  /* Next */
    VOICE_ID_ELEC_OFF_EN         = 0x56,  /* Electrode pad fell off*/
    VOICE_ID_LINE_LOSS_EN        = 0x57,  /* Wire falling off */
} voice_id_t;

/* 语音段数据结构  */ 
typedef struct {
    uint8_t  segment_id;
    uint16_t duration_ms;
    char     name[32];
} audio_segment_t;

/* 语音段范围 / 音量档（对齐 STM32 CWTN6170.h） */
#define VOICE_SEG_MIN     1
#define VOICE_SEG_MAX     0x55
#define VOICE_VOL_MAX     5

/* ================================================================
 *  语音控制 API（封装 Modbus 写 STM32 语音寄存器）
 * ================================================================ */
/** @brief 初始化：注册预览事件 handler + 建 esp_timer + 开机把 NVS 音量静默下发
 *  @note  需在 app_modbus_init() 之后调用 */
esp_err_t voice_init(void);

/** @brief 播放段 1~0x55（STM32 驱动支持播放中打断） */
int  voice_play(uint8_t seg);
/** @brief 停止播放（0xFE） */
int  voice_stop(void);
/** @brief 音量档 0~5（静默，不播预览） */
int  voice_set_vol(uint8_t vol);
/** @brief 音量预览：调音量 + 播当前语言数字段（拖动出声；内部 300ms debounce + 事件异步） */
int  voice_preview(uint8_t vol);
/** @brief 查询播放忙状态（true=播放中） */
int  voice_is_busy(bool *busy);
/** @brief 音量档 → 芯片命令字节 0xE0+3v（调试/日志用） */
uint8_t voice_vol_to_cmd(uint8_t vol);
esp_err_t audio_play_request(uint8_t id, uint16_t ms);
#ifdef __cplusplus
}
#endif

#endif /* VOICE_H */
