/**
  ******************************************************************************
  * @文件名称   app_params.c
  * @文件描述   参数存储模块（NVS）实现（阶段 E：参数存储）
  *            - 混合方案：设置/治疗参数 → NVS（本次）；历史记录 → SPIFFS（后续阶段）
  *            - namespace "app"，键：vol / bri / lang（i32）+ treat0/1/2（blob）
  *            - 内存副本 + 离开页面/点确认时保存（不在变更时立即写，减少 NVS 磨损）
  ******************************************************************************
  */

#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "app_params.h"

static const char *TAG = "app_params";

/* NVS 键名（≤15 字符） */
#define KEY_VOLUME      "vol"
#define KEY_BRIGHT      "bri"
#define KEY_LANG        "lang"
#define KEY_TREAT0      "treat0"
#define KEY_TREAT1      "treat1"
#define KEY_TREAT2      "treat2"

/* 默认值 */
#define DEF_VOLUME      1
#define DEF_BRIGHT      1
#define DEF_LANG        1          /* LANG_ZH */

static nvs_handle_t s_nvs = 0;
static bool         s_ready = false;

/* ---- 内存副本 ---- */
static int32_t s_vol   = DEF_VOLUME;
static int32_t s_bri   = DEF_BRIGHT;
static int32_t s_lang  = DEF_LANG;
static treat_param_t s_treat[APP_PARAMS_TREAT_NUM] = {
    { .intens1 = 0, .intens2 = 0, .freq = 1, .pulse = 500, .stage = 1 },  /* 腹直肌分离 */
    { .intens1 = 0, .intens2 = 0, .freq = 2, .pulse = 500, .stage = 1 },  /* 子宫复旧 */
    { .intens1 = 0, .intens2 = 0, .freq = 3, .pulse = 500, .stage = 1 },  /* 催乳 */
};

static const char *treat_key(int idx)
{
    switch (idx) {
    case 0: return KEY_TREAT0;
    case 1: return KEY_TREAT1;
    case 2: return KEY_TREAT2;
    default: return NULL;
    }
}

esp_err_t app_params_init(void)
{
    /* 初始化 NVS flash（标准 IDF 模式：分区满/版本变化 → 擦除重试） */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s (0x%X)", esp_err_to_name(ret), ret);
        return ret;
    }

    ret = nvs_open(APP_PARAMS_NSPACE, NVS_READWRITE, &s_nvs);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open(%s) failed: %s (0x%X)", APP_PARAMS_NSPACE, esp_err_to_name(ret), ret);
        return ret;
    }
    s_ready = true;

    /* 设置类：缺失 → 默认值 */
    int32_t v = 0;
    s_vol  = (nvs_get_i32(s_nvs, KEY_VOLUME, &v) == ESP_OK) ? v : DEF_VOLUME;
    s_bri  = (nvs_get_i32(s_nvs, KEY_BRIGHT, &v) == ESP_OK) ? v : DEF_BRIGHT;
    s_lang = (nvs_get_i32(s_nvs, KEY_LANG,   &v) == ESP_OK) ? v : DEF_LANG;

    /* 治疗类：缺失 → 默认值 */
    for (int i = 0; i < APP_PARAMS_TREAT_NUM; i++) {
        treat_param_t t;
        size_t len = sizeof(t);
        if (nvs_get_blob(s_nvs, treat_key(i), &t, &len) == ESP_OK && len == sizeof(t)) {
            s_treat[i] = t;
        }
    }

    ESP_LOGI(TAG, "init OK (vol=%d bri=%d lang=%d)", (int)s_vol, (int)s_bri, (int)s_lang);
    return ESP_OK;
}

/* ---- 设置类 ---- */
int32_t app_params_get_volume(void) { return s_vol; }
void    app_params_set_volume(int32_t v) { s_vol = v; }
int32_t app_params_get_bright(void) { return s_bri; }
void    app_params_set_bright(int32_t v) { s_bri = v; }
int32_t app_params_get_lang(void)   { return s_lang; }
void    app_params_set_lang(int32_t l)   { s_lang = l; }

/* ---- 治疗类 ---- */
esp_err_t app_params_get_treat(int idx, treat_param_t *out)
{
    if (idx < 0 || idx >= APP_PARAMS_TREAT_NUM || out == NULL) return ESP_ERR_INVALID_ARG;
    *out = s_treat[idx];
    return ESP_OK;
}

esp_err_t app_params_set_treat(int idx, const treat_param_t *in)
{
    if (idx < 0 || idx >= APP_PARAMS_TREAT_NUM || in == NULL) return ESP_ERR_INVALID_ARG;
    s_treat[idx] = *in;
    return ESP_OK;
}

/* ---- 保存 ---- */
esp_err_t app_params_save_settings(void)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    nvs_set_i32(s_nvs, KEY_VOLUME, s_vol);
    nvs_set_i32(s_nvs, KEY_BRIGHT, s_bri);
    nvs_set_i32(s_nvs, KEY_LANG,   s_lang);
    esp_err_t ret = nvs_commit(s_nvs);
    ESP_LOGI(TAG, "save settings: vol=%d bri=%d lang=%d (%s)",
             (int)s_vol, (int)s_bri, (int)s_lang,
             ret == ESP_OK ? "OK" : esp_err_to_name(ret));
    return ret;
}

esp_err_t app_params_save_treat(int idx)
{
    if (idx < 0 || idx >= APP_PARAMS_TREAT_NUM) return ESP_ERR_INVALID_ARG;
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    esp_err_t ret = nvs_set_blob(s_nvs, treat_key(idx), &s_treat[idx], sizeof(treat_param_t));
    if (ret != ESP_OK) return ret;
    ret = nvs_commit(s_nvs);
    ESP_LOGI(TAG, "save treat%d: freq=%d pulse=%d i1=%d i2=%d stage=%d (%s)", idx,
             (int)s_treat[idx].freq, (int)s_treat[idx].pulse,
             (int)s_treat[idx].intens1, (int)s_treat[idx].intens2,
             (int)s_treat[idx].stage, ret == ESP_OK ? "OK" : esp_err_to_name(ret));
    return ret;
}

esp_err_t app_params_save_all(void)
{
    esp_err_t ret = app_params_save_settings();
    if (ret != ESP_OK) return ret;
    for (int i = 0; i < APP_PARAMS_TREAT_NUM; i++) {
        ret = app_params_save_treat(i);
        if (ret != ESP_OK) return ret;
    }
    return ESP_OK;
}

/* ---- 调试 ---- */
void app_params_print(void)
{
    printf("[params] NVS namespace '%s'\r\n", APP_PARAMS_NSPACE);
    printf("  vol  = %d\r\n", (int)s_vol);
    printf("  bri  = %d\r\n", (int)s_bri);
    printf("  lang = %d (0=EN 1=ZH)\r\n", (int)s_lang);
    for (int i = 0; i < APP_PARAMS_TREAT_NUM; i++) {
        printf("  treat%d: freq=%d pulse=%d intens1=%d intens2=%d stage=%d\r\n", i,
               (int)s_treat[i].freq, (int)s_treat[i].pulse,
               (int)s_treat[i].intens1, (int)s_treat[i].intens2,
               (int)s_treat[i].stage);
    }
}

esp_err_t app_params_reset(void)
{
    if (!s_ready) return ESP_ERR_INVALID_STATE;
    esp_err_t ret = nvs_erase_all(s_nvs);
    if (ret != ESP_OK) return ret;
    ret = nvs_commit(s_nvs);
    if (ret != ESP_OK) return ret;

    /* 恢复默认值（与 UI 静态默认一致：freq=1/2/3, pulse=500, intens=0, stage=1） */
    s_vol = DEF_VOLUME; s_bri = DEF_BRIGHT; s_lang = DEF_LANG;
    for (int i = 0; i < APP_PARAMS_TREAT_NUM; i++) {
        s_treat[i].intens1 = 0;
        s_treat[i].intens2 = 0;
        s_treat[i].freq    = i + 1;
        s_treat[i].pulse   = 500;
        s_treat[i].stage   = 1;
    }
    ESP_LOGW(TAG, "params reset to defaults");
    return app_params_save_all();
}
