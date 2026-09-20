/**
  ******************************************************************************
  * @文件名称   app_spiffs.c
  * @文件描述   SPIFFS 文件系统挂载与工具 API 实现（阶段 S0）
  ******************************************************************************
  */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "app_spiffs.h"

static const char *TAG = "app_spiffs";

/* 当前激活的图片槽（0=storage_a / 1=storage_b），启动时从 NVS 读取 */
static uint8_t s_active_slot = APP_STORAGE_SLOT_A;

static const char *slot_label(uint8_t slot)
{
    return (slot == APP_STORAGE_SLOT_B) ? APP_STORAGE_LABEL_B : APP_STORAGE_LABEL_A;
}

/* 确保 NVS 已初始化（幂等；可能早于 app_params_init 被调用） */
static esp_err_t nvs_ensure_init(void)
{
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        r = nvs_flash_init();
    }
    return r;
}

esp_err_t app_spiffs_init(void)
{
    esp_err_t ret = nvs_ensure_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s (0x%X)", esp_err_to_name(ret), ret);
        return ret;
    }

    /* 读 active_storage 标记（默认 0 → storage_a） */
    nvs_handle_t h;
    if (nvs_open(APP_STORAGE_NSPACE, NVS_READONLY, &h) == ESP_OK) {
        int32_t v = 0;
        if (nvs_get_i32(h, APP_STORAGE_KEY_SLOT, &v) == ESP_OK &&
            (v == APP_STORAGE_SLOT_A || v == APP_STORAGE_SLOT_B)) {
            s_active_slot = (uint8_t)v;
        }
        nvs_close(h);
    }

    esp_vfs_spiffs_conf_t conf = {
        .base_path              = "/spiffs",
        .partition_label        = slot_label(s_active_slot),
        .max_files              = 5,
        .format_if_mount_failed = false,
    };

    ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount SPIFFS (partition not found or corrupted)");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition '%s'", slot_label(s_active_slot));
        } else {
            ESP_LOGE(TAG, "Failed to init SPIFFS: %s (0x%X)", esp_err_to_name(ret), ret);
        }
        return ret;
    }

    app_spiffs_print_info();
    return ESP_OK;
}

uint8_t app_spiffs_get_active_slot(void)
{
    return s_active_slot;
}

esp_err_t app_spiffs_set_active_slot(uint8_t slot)
{
    if (slot != APP_STORAGE_SLOT_A && slot != APP_STORAGE_SLOT_B) {
        return ESP_ERR_INVALID_ARG;
    }
    esp_err_t ret = nvs_ensure_init();
    if (ret != ESP_OK) return ret;

    nvs_handle_t h;
    ret = nvs_open(APP_STORAGE_NSPACE, NVS_READWRITE, &h);
    if (ret != ESP_OK) return ret;

    ret = nvs_set_i32(h, APP_STORAGE_KEY_SLOT, (int32_t)slot);
    if (ret == ESP_OK) ret = nvs_commit(h);
    nvs_close(h);

    if (ret == ESP_OK) {
        s_active_slot = slot;
        ESP_LOGI(TAG, "active storage slot flip to %s", slot_label(slot));
    }
    return ret;
}

void app_spiffs_print_info(void)
{
    size_t total = 0, used = 0;
    esp_err_t ret = esp_spiffs_info(slot_label(s_active_slot), &total, &used);
    if (ret != ESP_OK) {
        printf("[spiffs] info failed: %s\r\n", esp_err_to_name(ret));
        return;
    }
    printf("[spiffs] partition '%s' mounted at /spiffs\r\n", slot_label(s_active_slot));
    printf("[spiffs] total: %u KB, used: %u KB, free: %u KB\r\n",
           (unsigned)(total / 1024), (unsigned)(used / 1024), (unsigned)((total - used) / 1024));
}

static void list_dir(const char *path, int depth)
{
    DIR *d = opendir(path);
    if (d == NULL) {
        printf("%*s[open failed: %s]\r\n", depth * 2, "", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(d)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full[512];
        int written = snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(full)) continue;

        struct stat st;
        if (stat(full, &st) != 0) continue;

        if (S_ISDIR(st.st_mode)) {
            printf("%*s[%s]/\r\n", depth * 2, "", entry->d_name);
            list_dir(full, depth + 1);
        } else {
            printf("%*s%-32s %6lu B\r\n", depth * 2, "", entry->d_name,
                   (unsigned long)st.st_size);
        }
    }
    closedir(d);
}

void app_spiffs_list(const char *path)
{
    const char *p = (path && path[0] != '\0') ? path : "/spiffs";
    list_dir(p, 0);
}

void app_spiffs_cat(const char *path, size_t offset, size_t len)
{
    if (path == NULL || path[0] == '\0') {
        printf("[spiffs] cat: path required\r\n");
        return;
    }
    if (len == 0) len = 256;

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        printf("[spiffs] cat: open '%s' failed\r\n", path);
        return;
    }

    /* 获取文件大小 */
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, (long)offset, SEEK_SET);

    unsigned char buf[16];
    size_t total = 0;
    size_t limit = (len < (size_t)(fsize - (long)offset)) ? len : (size_t)(fsize - (long)offset);

    printf("[spiffs] %s  (%ld B total, offset=%u, len=%u)\r\n",
           path, fsize, (unsigned)offset, (unsigned)limit);

    while (total < limit) {
        size_t n = fread(buf, 1, 16, f);
        if (n == 0) break;

        printf("  %04X: ", (unsigned)(offset + total));
        for (size_t i = 0; i < 16; i++) {
            if (i < n) printf("%02X ", buf[i]);
            else       printf("   ");
        }
        printf(" ");
        for (size_t i = 0; i < n; i++)
            printf("%c", (buf[i] >= 0x20 && buf[i] < 0x7F) ? buf[i] : '.');
        printf("\r\n");
        total += n;
    }
    fclose(f);
}
