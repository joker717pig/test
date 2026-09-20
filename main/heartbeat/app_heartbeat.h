/**
  ******************************************************************************
  * @文件名称   app_heartbeat.h
  * @文件描述   心跳模块：1s 周期读 STM32 状态（RTC + 电池），一次 FC03 读
  *             0x0305 起 10 个寄存器（RTC 5 + CTRL 1 + 电池 4）全部拿回。
  *             - RTC(0x0305~09) → clock_settime 同步 C 时钟（顶栏时间）
  *             - 电池(0x030B~0E) → SOC/电压/状态/充电电流（顶栏电池 UI）
  *             合并自原 app_rtc + app_batt（方案 A：一次事务全部拿回）
  ******************************************************************************
  */

#ifndef APP_HEARTBEAT_H
#define APP_HEARTBEAT_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 时间结构（本地墙钟，非 UTC） */
typedef struct {
    int year;       /* 2000~2099 */
    int month;      /* 1~12     */
    int day;        /* 1~31     */
    int hour;       /* 0~23     */
    int minute;     /* 0~59     */
} app_rtc_t;

/* 电池数据快照（来自 STM32 CBAT 寄存器） */
typedef struct {
    uint8_t  soc;        /* 电量 0~100 (%)            */
    uint16_t volt_mv;    /* 电池电压 (mV)             */
    uint8_t  status;     /* BAT_STATUS_* (见 app_modbus_reg.h) */
    uint16_t charge_ma;  /* 充电电流 (mA)             */
} app_batt_t;

/**
  * @brief  启动心跳轮询（esp_timer 1s + 事件框架；读 RTC+电池并同步 C 时钟）
  * @note   需在 app_modbus_init() 之后调用
  * @retval ESP_OK 成功
  */
esp_err_t app_heartbeat_init(void);

/* ---- RTC 时间 ---- */
/** @brief 获取最近一次成功读到的时间（false=尚未读到） */
bool app_heartbeat_get_time(app_rtc_t *out);
/** @brief 获取对时草稿（首次调用初始化为当前 RTC 时间） */
void app_heartbeat_get_pending(app_rtc_t *out);
/** @brief 更新对时草稿（设置页滚轮变化时调用） */
void app_heartbeat_set_pending(const app_rtc_t *in);
/** @brief 把草稿写入 STM32 RTC（FC10 5 字段 + FC06 CTRL）+ 同步 C 时钟 */
int app_heartbeat_commit(void);

/* ---- 电池信息 ---- */
/** @brief 获取最近一次成功读到的电池数据（false=尚未读到） */
bool app_heartbeat_get_batt(app_batt_t *out);

/**
 * @brief  停止心跳轮询（esp_timer stop；软关机时调用）
 * @note   开机靠 esp_restart 重启后 app_heartbeat_init() 自动恢复
 * @retval ESP_OK 成功
 */
esp_err_t app_heartbeat_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* APP_HEARTBEAT_H */
