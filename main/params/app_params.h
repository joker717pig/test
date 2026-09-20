/**
  ******************************************************************************
  * @文件名称   app_params.h
  * @文件描述   参数存储模块（NVS）接口（阶段 E：参数存储）
  *            设置类（音量/亮度/语言）+ 治疗类（3 功能 × 治疗参数）→ NVS 持久化。
  *            混合方案：本模块先做 NVS 部分；历史记录/疗程数据留 SPIFFS（阶段 D/E）。
  *            设计：内存副本 + 离开页面/点确认时保存（减少 NVS 写放大/磨损）。
  ******************************************************************************
  */

#ifndef APP_PARAMS_H
#define APP_PARAMS_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_PARAMS_NSPACE        "app"     /* NVS 命名空间 */
#define APP_PARAMS_TREAT_NUM     3         /* 治疗功能数（腹直肌分离/子宫复旧/催乳） */

/* 治疗参数持久化结构（POD：仅数值字段；Param_Data_t 含 LVGL 对象与字符串指针，不能直接存） */
typedef struct {
    int32_t intens1;    /* 阶段1强度 0~10 */
    int32_t intens2;    /* 阶段2强度 0~10 */
    int32_t freq;       /* 频率 Hz（处方默认 1/2/3） */
    int32_t pulse;      /* 脉宽（默认 500） */
    int8_t  stage;      /* 当前阶段 1/2 */
} treat_param_t;

/**
  * @brief  初始化 NVS flash + 打开 namespace + 加载全部参数到内存副本
  *         首次运行（无键）用默认值。可重复调用（幂等）。
  * @retval ESP_OK 成功
  */
esp_err_t app_params_init(void);

/* ---- 设置类参数（内存副本，set 后需 save 才落盘） ---- */
int32_t app_params_get_volume(void);
void    app_params_set_volume(int32_t v);
int32_t app_params_get_bright(void);
void    app_params_set_bright(int32_t v);
int32_t app_params_get_lang(void);
void    app_params_set_lang(int32_t l);

/* ---- 治疗类参数（内存副本，set 后需 save 才落盘） ---- */
esp_err_t app_params_get_treat(int idx, treat_param_t *out);
esp_err_t app_params_set_treat(int idx, const treat_param_t *in);

/* ---- 保存 / 调试 ---- */
esp_err_t app_params_save_settings(void);   /* 音量/亮度/语言 */
esp_err_t app_params_save_treat(int idx);   /* 单个功能治疗参数 */
esp_err_t app_params_save_all(void);        /* 全部 */
void      app_params_print(void);           /* console 打印当前内存副本 */
esp_err_t app_params_reset(void);           /* 擦除全部键，恢复默认值 */

#ifdef __cplusplus
}
#endif

#endif /* APP_PARAMS_H */
