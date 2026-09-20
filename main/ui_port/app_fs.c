/**
  ******************************************************************************
  * @文件名称   app_fs.c
  * @文件描述   LVGL 'C' 盘 FS 驱动实现（ui_port 平台移植层）
  *
  * 背景：
  *   同事 UI 代码硬编码 Windows 绝对路径 "C:/Users/zqt/Desktop/PDJ_LVGL/png/..."
  *   （字体 lv_binfont_create / 图片 lv_image_set_src / 图标）。
  *   为让同事代码「原装」可用、更新时零适配，这里注册一个 LVGL 'C' 盘驱动：
  *     - LVGL 剥掉 "C:" → open_cb 收到 real_path = "/Users/zqt/.../png/..."
  *     - translate_path() 把它翻译成 SPIFFS 实际路径
  *     - open 时整文件读入 RAM → read/seek 走内存（渲染零文件 I/O）
  *
 * 路径翻译规则（同事 /png/<dir>/<file> → SPIFFS）：1:1 镜像
 *   /png/<dir>/<f> → /spiffs/png/<dir>/<f>  （镜像目录结构，天然隔离跨目录同名文件）
 *   lv_font_honorans_medium_14.bin 超长改名 lv_font_14.bin（SPIFFS 32B 限制，0G 坑1）
  ******************************************************************************
  */

#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "lvgl.h"
#include "app_fs.h"

static const char *TAG = "app_fs";

/* ================================================================
 *  路径翻译：/Users/zqt/.../png/<dir>/<file> → /spiffs/<dir2>/<file>
 * ================================================================ */
static void translate_path(const char *real, char *out, size_t outsz)
{
    const char *p = strstr(real, "/png/");
    if (p == NULL) {
        /* 非同事路径：按 /spiffs + 原路径 处理（兼容直接 S:/ 或 /spiffs） */
        if (strncmp(real, "/spiffs", 7) == 0) {
            snprintf(out, outsz, "%s", real);
        } else {
            snprintf(out, outsz, "/spiffs%s", real);
        }
        return;
    }

    /* p 指向 "/png/font/xxx.bin" 或 "/png/menu/xxx.bin" 等：
     * 1:1 镜像到 /spiffs/png/<dir>/<file>（镜像目录结构，同事加目录零适配） */
    const char *name = strrchr(p, '/') + 1;
    /* SPIFFS 文件名 32 字符限制（0G 坑1）：超长改名 */
    if (strcmp(name, "lv_font_honorans_medium_14.bin") == 0) {
        size_t dir_len = (size_t)(strrchr(p, '/') - p);
        char dir[96];
        memcpy(dir, p, dir_len);
        dir[dir_len] = '\0';
        snprintf(out, outsz, "/spiffs%s/lv_font_14.bin", dir);
        return;
    }
    snprintf(out, outsz, "/spiffs%s", p);
}

/* ================================================================
 *  'C' 盘内存文件驱动
 * ================================================================ */
typedef struct {
    uint8_t *buf;        /* 指向文件内容（可能来自缓存表） */
    uint32_t size;
    uint32_t pos;
} cfile_t;

/* ---- 文件内容缓存：open 过的文件常驻 RAM，重复 open 零 SPIFFS 读取 ----
 * 背景：LVGL 渲染 image 时会反复 open（重绘/动画帧），若每次都从 SPIFFS
 * 重读 44KB 大图 → 渲染任务被拖垮，按键响应延迟 2s+（见 0H 笔记 七）。
 * 这里把文件内容缓存下来，open 只查表 + malloc 小结构。
 * 容量：菜单/字体共 ~20 个文件、~300KB，PSRAM 8MB 足够。 */
#define CFS_CACHE_MAX  32
typedef struct {
    char     full[96];
    uint8_t *data;
    uint32_t size;
    bool     used;
} cfs_cache_t;
static cfs_cache_t s_cache[CFS_CACHE_MAX];

static cfs_cache_t *cfs_cache_find(const char *full)
{
    for (int i = 0; i < CFS_CACHE_MAX; i++) {
        if (s_cache[i].used && strcmp(s_cache[i].full, full) == 0)
            return &s_cache[i];
    }
    return NULL;
}

static cfs_cache_t *cfs_cache_slot(void)
{
    for (int i = 0; i < CFS_CACHE_MAX; i++) {
        if (!s_cache[i].used) {
            s_cache[i].used = true;
            return &s_cache[i];
        }
    }
    /* 满：覆盖第一个（菜单场景够用，不做 LRU） */
    if (s_cache[0].data) free(s_cache[0].data);
    s_cache[0].used = true;
    return &s_cache[0];
}

static void *cfs_open_cb(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    (void)drv; (void)mode;
    char full[160];
    translate_path(path, full, sizeof(full));

    /* 1) 查缓存：命中则复用文件内容，零 SPIFFS 读取 */
    cfs_cache_t *ce = cfs_cache_find(full);
    if (ce == NULL) {
        FILE *f = fopen(full, "rb");
        if (f == NULL) {
            ESP_LOGW(TAG, "open fail: '%s' -> '%s'", path, full);
            return NULL;
        }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (sz <= 0) { fclose(f); return NULL; }

        ce = cfs_cache_slot();
        ce->size = (uint32_t)sz;
        strlcpy(ce->full, full, sizeof(ce->full));
        ce->data = malloc((size_t)sz);
        if (ce->data == NULL) { fclose(f); return NULL; }
        if (fread(ce->data, 1, (size_t)sz, f) != (size_t)sz) {
            free(ce->data); ce->data = NULL; ce->used = false;
            fclose(f); return NULL;
        }
        fclose(f);
        ESP_LOGI(TAG, "C:load '%s' -> %s (%u B, SPIFFS)", path, full, ce->size);
    }

    /* 2) 每次 open 分配独立句柄，指向缓存内容 */
    cfile_t *cf = malloc(sizeof(cfile_t));
    if (cf == NULL) return NULL;
    cf->buf  = ce->data;
    cf->size = ce->size;
    cf->pos  = 0;
    ESP_LOGD(TAG, "C:open '%s' -> %s (cache)", path, full);
    return cf;
}

static lv_fs_res_t cfs_close_cb(lv_fs_drv_t *drv, void *file_p)
{
    (void)drv;
    free(file_p);           /* 只释放句柄，缓存内容常驻 */
    return LV_FS_RES_OK;
}

static lv_fs_res_t cfs_read_cb(lv_fs_drv_t *drv, void *file_p, void *buf,
                               uint32_t btr, uint32_t *br)
{
    (void)drv;
    cfile_t *cf = (cfile_t *)file_p;
    uint32_t avail = cf->size - cf->pos;
    uint32_t n = (btr < avail) ? btr : avail;
    if (n) memcpy(buf, cf->buf + cf->pos, n);
    cf->pos += n;
    *br = n;
    return LV_FS_RES_OK;
}

static lv_fs_res_t cfs_seek_cb(lv_fs_drv_t *drv, void *file_p, uint32_t pos,
                               lv_fs_whence_t whence)
{
    (void)drv;
    cfile_t *cf = (cfile_t *)file_p;
    switch (whence) {
    case LV_FS_SEEK_SET: cf->pos = pos; break;
    case LV_FS_SEEK_CUR: cf->pos += pos; break;
    case LV_FS_SEEK_END: cf->pos = cf->size + pos; break;
    default: return LV_FS_RES_INV_PARAM;
    }
    if (cf->pos > cf->size) cf->pos = cf->size;
    return LV_FS_RES_OK;
}

static lv_fs_res_t cfs_tell_cb(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    (void)drv;
    *pos_p = ((cfile_t *)file_p)->pos;
    return LV_FS_RES_OK;
}

/* LVGL 9 无 size_cb：lv_fs_size 内部用 seek(END)+tell 求大小，内存文件天然支持 */

/* ================================================================
 *  注册
 * ================================================================ */
esp_err_t app_fs_init(void)
{
    static lv_fs_drv_t drv;     /* 需保持存活，lv_fs_drv_register 只存指针 */
    lv_fs_drv_init(&drv);

    drv.letter     = 'C';       /* 同事路径 "C:/..." 走本驱动 */
    drv.cache_size = 0;         /* 内存文件直读，不用 LVGL 文件缓存 */
    drv.open_cb    = cfs_open_cb;
    drv.close_cb   = cfs_close_cb;
    drv.read_cb    = cfs_read_cb;
    drv.write_cb   = NULL;      /* 只读 */
    drv.seek_cb    = cfs_seek_cb;
    drv.tell_cb    = cfs_tell_cb;

    lv_fs_drv_register(&drv);

    ESP_LOGI(TAG, "LVGL FS 'C' driver registered (colleague path -> SPIFFS, RAM-cached)");
    return ESP_OK;
}
