/**
  ******************************************************************************
  * @文件名称   app_modbus_reg.h
  * @文件描述   EDA_EMG_ESP32 阶段 A：Modbus 寄存器映射 / 功能码 / 位图常量
  *            对齐下位机 STM32 `EDA_EMG/code/MODBUS/MB_RegMap.h`
  *            从站: 0x01, 115200 8N1（本文件只含 ESP32 主站需要的部分）
  ******************************************************************************
  */

#ifndef APP_MODBUS_REG_H
#define APP_MODBUS_REG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Modbus 功能码 ---- */
#define MB_FC_READ_HOLDING_REGS     0x03
#define MB_FC_WRITE_SINGLE_REG      0x06
#define MB_FC_WRITE_MULTIPLE_REGS   0x10

/* ---- STM32 OTA 主动拉取帧 (自定义, 对齐 Boot user_main.c) ---- */
#define MB_FC_OTA_REQ               0x41    /* Boot->ESP32 请求块 */
#define MB_FC_OTA_DATA              0x42    /* ESP32->Boot 数据块 */
#define MB_FC_OTA_DONE              0x43    /* Boot->ESP32 完成通知 */
#define MB_OTA_BLK_LEN              240     /* 单块数据字节数 (4 的倍数) */

/* ---- 波形控制寄存器 (0x0100 ~ 0x0107) ---- */
#define MB_REG_CH1_FREQ         0x0100  /* R/W  CH1 频率 (Hz),    1~105    */
#define MB_REG_CH1_PW           0x0101  /* R/W  CH1 脉宽 (μs),   100~500  */
#define MB_REG_CH1_MA           0x0102  /* R/W  CH1 电流 (mA),    0~90     */
#define MB_REG_CH2_FREQ         0x0103  /* R/W  CH2 频率 (Hz)              */
#define MB_REG_CH2_PW           0x0104  /* R/W  CH2 脉宽 (μs)              */
#define MB_REG_CH2_MA           0x0105  /* R/W  CH2 电流 (mA)              */
#define MB_REG_WAVE_CTRL        0x0106  /* W    波形控制 bitmask            */
#define MB_REG_WAVE_STATUS      0x0107  /* R    波形运行状态                */

/* ---- 采集控制寄存器 (0x0200 ~ 0x0207) ---- */
#define MB_REG_ACQ_SPS          0x0200  /* R/W  采样率: 125/250/500/1000 SPS */
#define MB_REG_ACQ_SAMPLES      0x0201  /* R/W  每次返回样本数: 1~50        */
#define MB_REG_ACQ_CHANNEL      0x0202  /* R/W  读取通道 bitmask            */
#define MB_REG_ACQ_CTRL         0x0203  /* W    采集控制: 1=启动, 0=停止    */
#define MB_REG_ACQ_STATUS       0x0204  /* R    采集状态: bit0=采集中       */
#define MB_REG_ACQ_COUNT        0x0205  /* R    待取样本数 0~50             */
#define MB_REG_ACQ_PUSH_CNT     0x0206  /* R    已推送帧数 (自启采后累计)   */
#define MB_REG_ACQ_ADS_STAT     0x0207  /* R    ADS1292 内部状态机值        */
#define MB_REG_PWR_OFF          0x020A  /* W    关机: 写1=STM32 关机 (有外部电源=软关机, 纯电池=真断电) */
#define MB_REG_OTA_INIT         0x020C  /* W    OTA_Init: 写1=STM32 先写 SOTA 升级请求再跳 BootLoader (OTA 触发, 不受充电判断) */

#define MB_STREAM_PUSH_INTERVAL_MS 20   /* 两次推帧最小间隔 (ms)            */
#define MB_ELEC_PUSH_INTERVAL_MS  500   /* 采集期电极脱落状态帧推送间隔(ms)  */
#define MB_PUSH_CNT_ELEC        0xFFFE  /* 电极状态帧 CNT 魔数 (区别数据帧1~50/按键帧0) */

/* ---- 采集数据区 (0x0210 ~ 0x0281) ---- */
#define MB_REG_CH1_DATA_BASE    0x0210  /* R    CH1 数据[50]  (16bit/样本)  */
#define MB_REG_CH2_DATA_BASE    0x0250  /* R    CH2 数据[50]                */
#define MB_REG_ACQ_DATA_MAX     50      /*      最大样本数 (双通道时各 50)  */

/* ---- 状态 / 诊断寄存器 (0x0300 ~ 0x0304) ---- */
#define MB_REG_DEV_STATUS       0x0300  /* R    设备状态                */
#define MB_REG_ELEC_STATUS      0x0301  /* R    电极脱落状态            */
#define MB_REG_FW_VERSION       0x0302  /* R    固件版本                */
#define MB_REG_ERROR_CODE       0x0303  /* R    错误码                  */
#define MB_REG_KEY_STATUS       0x0304  /* R    按键触发位图            */

/* ---- ELEC_STATUS (0x0301) 电极脱落位图 bit 定义, 对齐 STM32 MB_RegMap.h ---- */
#define ELEC_STATUS_NNC_CH1_OFF  0x01   /* NNC6521 CH1 刺激电极脱落      */
#define ELEC_STATUS_NNC_CH2_OFF  0x02   /* NNC6521 CH2 刺激电极脱落      */
#define ELEC_STATUS_ADS_IN1P_OFF 0x04   /* ADS1292 IN1P 采集电极脱落     */
#define ELEC_STATUS_ADS_IN1N_OFF 0x08   /* ADS1292 IN1N 采集电极脱落     */
#define ELEC_STATUS_ADS_IN2P_OFF 0x10   /* ADS1292 IN2P 采集电极脱落     */
#define ELEC_STATUS_ADS_IN2N_OFF 0x20   /* ADS1292 IN2N 采集电极脱落     */

/* ---- RTC 时间寄存器 (0x0305 ~ 0x030A) ---- */
/* 对齐下位机 EDA_EMG/code/MODBUS/MB_RegMap.h；只传年/月/日/时/分，秒不上报 */
#define MB_REG_RTC_YEAR         0x0305  /* R/W  年 2000~2099 (4 位)     */
#define MB_REG_RTC_MONTH        0x0306  /* R/W  月 1~12                 */
#define MB_REG_RTC_DAY          0x0307  /* R/W  日 1~31                 */
#define MB_REG_RTC_HOUR         0x0308  /* R/W  时 0~23                 */
#define MB_REG_RTC_MIN          0x0309  /* R/W  分 0~59                 */
#define MB_REG_RTC_CTRL         0x030A  /* W    对时命令: 写1=按上面 5 字段设置 RTC */

/* ---- 电池状态寄存器 (0x030B ~ 0x030E, 只读) ---- */
#define MB_REG_BAT_SOC         0x030B  /* R    电量 0~100 (%)            */
#define MB_REG_BAT_VOLT        0x030C  /* R    电池电压 (mV)             */
#define MB_REG_BAT_STATUS      0x030D  /* R    电池状态枚举 (见下)        */
#define MB_REG_BAT_CHARGE_MA   0x030E  /* R    充电电流 (mA)             */

/* ---- 语音控制寄存器 (0x030F ~ 0x0312) ---- */
/* 对齐下位机 EDA_EMG/code/MODBUS/MB_RegMap.h */
#define MB_REG_VOICE_PLAY     0x030F  /* W    播放段地址 1~0x55 (WTN6170, 驱动支持播放中打断) */
#define MB_REG_VOICE_STOP     0x0310  /* W    停止播放: 写1=发 0xFE */
#define MB_REG_VOICE_VOL      0x0311  /* R/W  音量档 0~5 (映射 0xE0+3v) */
#define MB_REG_VOICE_BUSY     0x0312  /* R    播放忙: 0=空闲 1=播放中 */

/* ---- OTA 升级寄存器 (0x0313 ~ 0x0317, 对齐 STM32 APP) ---- */
#define MB_REG_OTA_TARGET     0x0313  /* R/W  升级目标: 1=APP (基址 0x08010000) */
#define MB_REG_OTA_FW_SIZE_H  0x0314  /* R/W  固件大小 高 16 位 (字节) */
#define MB_REG_OTA_FW_SIZE_L  0x0315  /* R/W  固件大小 低 16 位 (字节) */
#define MB_REG_OTA_FW_CRC_H   0x0316  /* R/W  固件 CRC32 高 16 位 */
#define MB_REG_OTA_FW_CRC_L   0x0317  /* R/W  固件 CRC32 低 16 位 */

/* 语音段范围 / 音量档（对齐 STM32 CWTN6170.h） */
#define VOICE_SEG_MIN         1
#define VOICE_SEG_MAX         0x57
#define VOICE_VOL_MAX         5

/* ---- BAT_STATUS 枚举 ---- */
#define BAT_STATUS_DISCHARGE   0       /* 未充电/放电中                  */
#define BAT_STATUS_CHARGING    1       /* 充电中                         */
#define BAT_STATUS_FULL        2       /* 充满                           */
#define BAT_STATUS_FAULT       3       /* 故障                           */

/* ---- DEV_STATUS bit 定义 ---- */
#define DEV_STATUS_CH1_STIM     0x01    /* CH1 刺激运行中               */
#define DEV_STATUS_CH2_STIM     0x02    /* CH2 刺激运行中               */
#define DEV_STATUS_ACQ_RUNNING  0x04    /* 采集中                       */
#define DEV_STATUS_HV_EN        0x08    /* 高压使能 (HVD_EN)            */
#define DEV_STATUS_ERROR        0x10    /* 错误                         */

/* ---- WAVE_CTRL 命令 bitmask ---- */
#define WAVE_CTRL_STOP          0x00    /* 全停                         */
#define WAVE_CTRL_CH1           0x01    /* CH1 启动                     */
#define WAVE_CTRL_CH2           0x02    /* CH2 启动                     */
#define WAVE_CTRL_BOTH          0x03    /* 双通道同时启动               */

/* ---- ACQ_CHANNEL bitmask ---- */
#define ACQ_CH_CH1              0x01    /* 返回 CH1 数据                */
#define ACQ_CH_CH2              0x02    /* 返回 CH2 数据                */
#define ACQ_CH_BOTH             0x03    /* 返回双通道数据               */

/* ---- 按键位图 bit（0x0304 / 按键推送帧 KS） ---- */
#define MB_KEY_END              0       /* 确认 (PE10)                  */
#define MB_KEY_UP               1       /* 上 (PE11)                    */
#define MB_KEY_RIGHT            2       /* 右 (PE12)                    */
#define MB_KEY_LEFT             3       /* 左 (PE13)                    */
#define MB_KEY_DOWN             4       /* 下 (PE14)                    */
#define MB_KEY_ESC              5       /* 返回 (PE15)                  */
#define MB_KEY_PWR_OFF          6       /* 关机 (PC2 运行态长按 2s)     */
#define MB_KEY_PWR_ON           7       /* 开机 (保留, 现开机由 STM32 拉 ESP32_EN 控制) */

#ifdef __cplusplus
}
#endif

#endif /* APP_MODBUS_REG_H */
