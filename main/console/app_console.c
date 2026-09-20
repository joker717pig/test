/**
  ******************************************************************************
  * @文件名称   app_console.c
  * @文件描述   串口命令 console 实现（USB-Serial-JTAG 双向交互）
  *            命令：help / info / mem / psram / stack / part
  ******************************************************************************
  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_psram.h"
#include "esp_heap_caps.h"
#include "esp_partition.h"
#include "app_console.h"
#include "app_debug.h"
#include "app_lcd.h"
#include "app_modbus.h"
#include "app_modbus_reg.h"
#include "app_spiffs.h"
#include "app_params.h"    /* 阶段 E：参数存储（NVS）调试 */
#include "voice.h"         /* 阶段 V：语音控制调试 */
#include "app_therapy.h"   /* 阶段 C：治疗控制模板（从处方表下发真实治疗命令） */
#include "therapy_rx.h"    /* 阶段 C：处方表 g_rx_all（stime 改写 stage_time 用） */
#include "emg_force.h"     /* 阶段 3.2：肌电力度引擎调试 */
#include "emg_dsp.h"       /* EMG_DSP_RAW16_LSB_UV 换算 */
#include "therapy_eval.h"  /* 盆底评估引擎调试（eval start/stop/status） */
#include "app_wifi.h"      /* 阶段 D：WiFi STA（OTA 联网验证） */
#include "app_ota.h"       /* 阶段 D：OTA 升级/版本清单验证 */

static const char *TAG = "app_console";

static void cmd_help(void)
{
    printf("commands:\r\n");
    printf("  help   : show this help\r\n");
    printf("  info   : chip / idf / flash / psram info\r\n");
    printf("  mem    : heap memory distribution\r\n");
    printf("  psram  : psram size / free\r\n");
    printf("  stack  : task stack high-water marks\r\n");
    printf("  part   : partition table\r\n");
    printf("  lcd    : run ST7365 bare test (colors/gradient)\r\n");
    printf("  bench  : full-screen fill speed benchmark\r\n");
    printf("  mb ... : modbus master commands (mb help)\r\n");
    printf("  spiffs : SPIFFS filesystem (info / ls / cat)\r\n");
    printf("  param  : NVS params (print / save / reset / set)\r\n");
    printf("  voice  : voice control (play/stop/vol/preview/busy)\r\n");
    printf("  therapy: treatment template (start/stop/pause/resume/status/scale)\r\n");
    printf("  top    : task CPU runtime stats\r\n");
    printf("  force  : EMG force engine (env/rms + acq ring)\r\n");
    printf("  eval   : pelvic floor evaluation (start/stop/status)\r\n");
    printf("  wifi   : OTA WiFi STA (init/connect/disconnect/state/ip)\r\n");
    printf("  ota    : OTA upgrade (manifest/esp32/image/stm32)\r\n");
}

static void cmd_info(void)
{
    esp_chip_info_t chip;
    uint32_t flash = 0;
    esp_chip_info(&chip);
    esp_flash_get_size(NULL, &flash);
    printf("-- system info --\r\n");
    printf("IDF version : %s\r\n", esp_get_idf_version());
    printf("Chip        : %s, %d cores\r\n", CONFIG_IDF_TARGET, chip.cores);
    printf("Flash       : %lu MB\r\n", (unsigned long)(flash / (1024 * 1024)));
    printf("PSRAM       : %lu MB\r\n", (unsigned long)(esp_psram_get_size() / (1024 * 1024)));
    printf("Free heap   : %lu KB (total), %lu KB (internal)\r\n",
           (unsigned long)(esp_get_free_heap_size() / 1024),
           (unsigned long)(esp_get_free_internal_heap_size() / 1024));
}

static void cmd_mem(void)
{
    printf("-- memory distribution --\r\n");
    printf("Free heap total    : %lu KB\r\n", (unsigned long)(esp_get_free_heap_size() / 1024));
    printf("Free heap internal : %lu KB\r\n", (unsigned long)(esp_get_free_internal_heap_size() / 1024));
    printf("Free PSRAM         : %lu KB\r\n", (unsigned long)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    printf("Largest free block : %lu KB\r\n", (unsigned long)(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT) / 1024));
    printf("-- heap regions (8-bit) --\r\n");
    heap_caps_print_heap_info(MALLOC_CAP_8BIT);
}

static void cmd_psram(void)
{
    printf("-- psram --\r\n");
    printf("total   : %lu MB\r\n", (unsigned long)(esp_psram_get_size() / (1024 * 1024)));
    printf("free    : %lu KB\r\n", (unsigned long)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    printf("largest : %lu KB\r\n", (unsigned long)(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024));
}

static void cmd_stack(void)
{
    app_debug_print_stack_watermarks();
}

static void cmd_lcd(void)
{
    app_lcd_bare_test();
}

static void cmd_bench(void)
{
    app_lcd_bench();
}

static void cmd_part(void)
{
    printf("-- partition table --\r\n");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY,
                                                     ESP_PARTITION_SUBTYPE_ANY, NULL);
    if (it == NULL) { printf("  (none)\r\n"); return; }
    for (; it != NULL; it = esp_partition_next(it)) {
        const esp_partition_t *p = esp_partition_get(it);
        printf("  %-12s type=%02d sub=%02d off=0x%06lx size=0x%06lx (%lu KB)\r\n",
               p->label, (int)p->type, (int)p->subtype,
               (unsigned long)p->address, (unsigned long)p->size,
               (unsigned long)(p->size / 1024));
    }
    esp_partition_iterator_release(it);
}

/* ---- Modbus 测试命令（阶段 A） ---- */
static int mb_tokens(const char *s, char tok[][16], int max)
{
    int n = 0;
    const char *p = s;
    while (n < max)
    {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;
        int i = 0;
        while (*p && *p != ' ' && *p != '\t' && i < 15) tok[n][i++] = *p++;
        tok[n][i] = '\0';
        n++;
    }
    return n;
}

static void cmd_mb_help(void)
{
    printf("modbus master commands:\r\n");
    printf("  mb info                    : show uart/config/state\r\n");
    printf("  mb fw                      : read firmware version (0x0302)\r\n");
    printf("  mb status                  : read status regs 0x0300~0x0304\r\n");
    printf("  mb read <addr> [n]         : FC03 read n holding regs (hex addr)\r\n");
    printf("  mb write <addr> <value>    : FC06 write single reg\r\n");
    printf("  mb write10 <addr> v1 [v2.] : FC10 write multiple regs\r\n");
    printf("  mb push on|off [ch]        : push listener on/off (ch 1/2/3)\r\n");
    printf("  mb acq                     : acq ring buffer status\r\n");
    printf("  mb log on|off              : hex frame log\r\n");
}

static void cmd_mb(const char *args)
{
    char tok[8][16];
    int  nt = mb_tokens(args, tok, 8);
    uint16_t addr, vals[32];
    int  e;

    if (nt == 0) { cmd_mb_help(); return; }

    if      (strcmp(tok[0], "help")    == 0) cmd_mb_help();
    else if (strcmp(tok[0], "info")    == 0) app_modbus_print_info();
    else if (strcmp(tok[0], "fw")      == 0)
    {
        e = app_modbus_read_registers(MB_REG_FW_VERSION, 1, vals);
        if (e == MB_OK)
            printf("fw version: 0x%04X (v%d.%d)\r\n", vals[0], vals[0] >> 8, vals[0] & 0xFF);
        else
            printf("read fw failed: %s\r\n", app_modbus_err_str(e));
    }
    else if (strcmp(tok[0], "status") == 0)
    {
        e = app_modbus_read_registers(MB_REG_DEV_STATUS, 5, vals);
        if (e == MB_OK)
            printf("DEV=0x%04X ELEC=0x%04X FW=0x%04X ERR=0x%04X KEY=0x%04X\r\n",
                   vals[0], vals[1], vals[2], vals[3], vals[4]);
        else
            printf("read status failed: %s\r\n", app_modbus_err_str(e));
    }
    else if (strcmp(tok[0], "read")   == 0)
    {
        uint16_t n = 1;
        if (nt < 2) { printf("usage: mb read <addr> [n]\r\n"); return; }
        addr = (uint16_t)strtoul(tok[1], NULL, 0);
        if (nt >= 3) n = (uint16_t)strtoul(tok[2], NULL, 0);
        if (n == 0 || n > 32) { printf("count must be 1..32\r\n"); return; }
        e = app_modbus_read_registers(addr, n, vals);
        if (e == MB_OK)
        {
            printf("0x%04X x%d:", addr, n);
            for (uint16_t i = 0; i < n; i++)
                printf(" %04X(%d)", vals[i], (int16_t)vals[i]);
            printf("\r\n");
        }
        else
            printf("read 0x%04X x%d failed: %s\r\n", addr, n, app_modbus_err_str(e));
    }
    else if (strcmp(tok[0], "write")  == 0)
    {
        uint16_t val;
        if (nt < 3) { printf("usage: mb write <addr> <value>\r\n"); return; }
        addr = (uint16_t)strtoul(tok[1], NULL, 0);
        val  = (uint16_t)strtoul(tok[2], NULL, 0);
        e = app_modbus_write_register(addr, val);
        if (e == MB_OK) printf("write 0x%04X = 0x%04X OK\r\n", addr, val);
        else            printf("write failed: %s\r\n", app_modbus_err_str(e));
    }
    else if (strcmp(tok[0], "write10") == 0)
    {
        uint16_t wv[20];
        int n = 0;
        if (nt < 3) { printf("usage: mb write10 <addr> v1 [v2..]\r\n"); return; }
        addr = (uint16_t)strtoul(tok[1], NULL, 0);
        for (int i = 2; i < nt && n < 20; i++) wv[n++] = (uint16_t)strtoul(tok[i], NULL, 0);
        e = app_modbus_write_registers(addr, wv, n);
        if (e == MB_OK) printf("write10 0x%04X x%d OK\r\n", addr, n);
        else            printf("write10 failed: %s\r\n", app_modbus_err_str(e));
    }
    else if (strcmp(tok[0], "push")   == 0)
    {
        if (nt < 2) { printf("usage: mb push on|off [ch]\r\n"); return; }
        if (strcmp(tok[1], "on") == 0)
        {
            uint8_t ch = (nt >= 3) ? (uint8_t)strtoul(tok[2], NULL, 0) : 3;
            if (app_modbus_start_push(ch) != ESP_OK)
                printf("invalid channel (1/2/3)\r\n");
        }
        else
            app_modbus_stop_push();
    }
    else if (strcmp(tok[0], "acq")    == 0)
    {
        printf("acq ring: %u/%d samples, push=%s ch=%u\r\n",
               app_modbus_acq_available(), MB_ACQ_RING_CAP,
               app_modbus_push_active() ? "on" : "off",
               app_modbus_push_channel());
    }
    else if (strcmp(tok[0], "log")    == 0)
    {
        if (nt < 2) { printf("usage: mb log on|off\r\n"); return; }
        app_modbus_set_frame_log(strcmp(tok[1], "on") == 0);
    }
    else
        printf("unknown mb subcommand '%s' (mb help)\r\n", tok[0]);
}

/* ---- SPIFFS 验证命令（阶段 S0） ---- */
static void cmd_spiffs(const char *args)
{
    char tok[4][128];
    int nt = 0;
    const char *p = args;
    while (nt < 4) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;
        int i = 0;
        while (*p && *p != ' ' && *p != '\t' && i < 127) tok[nt][i++] = *p++;
        tok[nt][i] = '\0';
        nt++;
    }

    if (nt == 0 || strcmp(tok[0], "info") == 0) {
        app_spiffs_print_info();
    } else if (strcmp(tok[0], "ls") == 0) {
        app_spiffs_list(nt >= 2 ? tok[1] : "");
    } else if (strcmp(tok[0], "cat") == 0) {
        if (nt < 2) { printf("usage: spiffs cat <path> [offset] [len]\r\n"); return; }
        size_t off = (nt >= 3) ? (size_t)strtoul(tok[2], NULL, 0) : 0;
        size_t len = (nt >= 4) ? (size_t)strtoul(tok[3], NULL, 0) : 0;
        app_spiffs_cat(tok[1], off, len);
    } else {
        printf("spiffs subcommands: info | ls [path] | cat <path> [offset] [len]\r\n");
    }
}

/* 阶段 E：NVS 参数命令（print / save / reset / set） */
static void cmd_param(const char *args)
{
    const char *p = args;
    while (*p == ' ' || *p == '\t') p++;

    if (strncmp(p, "print", 5) == 0) {
        app_params_print();
    } else if (strncmp(p, "save", 4) == 0) {
        app_params_save_all();
    } else if (strncmp(p, "reset", 5) == 0) {
        app_params_reset();
    } else if (strncmp(p, "set", 3) == 0) {
        /* [我们] 调试用：param set <key> <values...> 只改内存副本，需再 param save 落盘
         *   vol|bri|lang <v>
         *   treat <idx> <intens1> <intens2> <freq> <pulse> <stage>   (idx 0~2) */
        const char *q = p + 3;
        while (*q == ' ' || *q == '\t') q++;
        char key[16] = {0};
        int i = 0;
        while (*q && *q != ' ' && *q != '\t' && i < 15) key[i++] = *q++;
        key[i] = '\0';
        if (key[0] == '\0') { printf("usage: param set <key> <values...>\r\n"); return; }

        int vals[6], nv = 0;
        while (nv < 6) {
            char *end = NULL;
            while (*q == ' ' || *q == '\t') q++;
            if (*q == '\0') break;
            vals[nv] = (int)strtol(q, &end, 0);
            if (end == q) break;   /* 无数字 */
            q = end;
            nv++;
        }
        if      (strcmp(key, "vol")  == 0 && nv >= 1) app_params_set_volume(vals[0]);
        else if (strcmp(key, "bri")  == 0 && nv >= 1) app_params_set_bright(vals[0]);
        else if (strcmp(key, "lang") == 0 && nv >= 1) app_params_set_lang(vals[0]);
        else if (strcmp(key, "treat") == 0 && nv >= 6 && vals[0] >= 0 && vals[0] < APP_PARAMS_TREAT_NUM) {
            treat_param_t t;
            if (app_params_get_treat(vals[0], &t) == ESP_OK) {
                t.intens1 = vals[1]; t.intens2 = vals[2];
                t.freq = vals[3];    t.pulse  = vals[4];
                t.stage = (int8_t)vals[5];
                app_params_set_treat(vals[0], &t);
            }
        } else {
            printf("bad: vol|bri|lang <v> | treat <idx> <i1> <i2> <freq> <pulse> <stage>\r\n");
            return;
        }
        app_params_print();   /* 打印改后内存副本 */
    } else {
        printf("param subcommands: print | save | reset | set\r\n");
    }
}

/* ---- 语音控制命令（阶段 V） ---- */
static void cmd_voice(const char *args)
{
    char tok[4][16];
    int  nt = mb_tokens(args, tok, 4);
    int  e;
    bool b;

    if (nt == 0) { printf("voice: play <seg> | stop | vol <0-5> | preview <0-5> | busy | help\r\n"); return; }

    if      (strcmp(tok[0], "help")    == 0)
        printf("voice: play <seg(1-0x55)> | stop | vol <0-5> | preview <0-5> | busy\r\n");
    else if (strcmp(tok[0], "play")    == 0)
    {
        if (nt < 2) { printf("usage: voice play <seg>\r\n"); return; }
        uint8_t seg = (uint8_t)strtoul(tok[1], NULL, 0);
        e = voice_play(seg);
        printf("voice play 0x%02X -> %s\r\n", seg, app_modbus_err_str(e));
    }
    else if (strcmp(tok[0], "stop")    == 0)
    {
        e = voice_stop();
        printf("voice stop -> %s\r\n", app_modbus_err_str(e));
    }
    else if (strcmp(tok[0], "vol")     == 0)
    {
        if (nt < 2) { printf("usage: voice vol <0-5>\r\n"); return; }
        uint8_t v = (uint8_t)strtoul(tok[1], NULL, 0);
        e = voice_set_vol(v);
        printf("voice vol %d (cmd 0x%02X) -> %s\r\n", (int)v, voice_vol_to_cmd(v), app_modbus_err_str(e));
    }
    else if (strcmp(tok[0], "preview") == 0)
    {
        if (nt < 2) { printf("usage: voice preview <0-5>\r\n"); return; }
        uint8_t v = (uint8_t)strtoul(tok[1], NULL, 0);
        e = voice_preview(v);
        printf("voice preview %d (debounced) -> %s\r\n", (int)v, app_modbus_err_str(e));
    }
    else if (strcmp(tok[0], "busy")    == 0)
    {
        e = voice_is_busy(&b);
        if (e == MB_OK) printf("voice busy=%d\r\n", b ? 1 : 0);
        else            printf("voice busy read failed: %s\r\n", app_modbus_err_str(e));
    }
    else
        printf("unknown voice subcommand '%s' (voice help)\r\n", tok[0]);
}

/* ---- 治疗控制命令（阶段C 模板：模板示例=处方3疗程1；新疗程改 start 里的 scheme） ---- */
static void cmd_therapy(const char *args)
{
    const char *p = args;
    while (*p == ' ' || *p == '\t') p++;

    if (strncmp(p, "help", 4) == 0 || *p == '\0') {
        printf("therapy: help | start <i1> <i2> | stop | pause | resume | status | scale <n>\r\n");
        printf("           | stime <rx> <scheme> <stage> <ms> | stime <rx> <scheme> <stage>\r\n");
        printf("           | ttime <ms> | ttime\r\n");
        printf("  start <i1> <i2> : 启动模板示例 处方3疗程1（步骤0/1 强度档 0~10；console 无 UI ctx）\r\n");
        printf("  scale <n>       : 1 真实秒 = n 会话秒（联调验证用；0/1=真实速度）\r\n");
        printf("  stime <rx> <scheme> <stage> <ms> : 改写处方阶段时长(ms)；rx=1~4 scheme/stage 从0起\r\n");
        printf("  stime <rx> <scheme> <stage>      : 查询该阶段当前时长(ms)\r\n");
        printf("  ttime <ms>      : 改写会话总治疗时长(ms)；如 ttime 600000 = 10分钟\r\n");
        printf("  ttime           : 查询当前会话总治疗时长(ms)\r\n");
        return;
    }
    if      (strncmp(p, "start", 5) == 0) {
        const char *q = p + 5;
        while (*q == ' ' || *q == '\t') q++;
        char *end = NULL;
        int i1 = (int)strtol(q, &end, 0);
        if (end == q) { printf("usage: therapy start <i1> <i2>\r\n"); return; }
        q = end;
        int i2 = (int)strtol(q, &end, 0);
        if (end == q) i2 = 0;
        // int r = therapy_start(&g_rx_all[RX_3]->schemes[0], i1, i2, NULL);
        // printf("therapy start (处方3疗程1, i1=%d i2=%d) -> %d\r\n", i1, i2, r);
    }
    else if (strncmp(p, "stop", 4) == 0)   { therapy_stop();   printf("therapy stop\r\n"); }
    else if (strncmp(p, "pause", 5) == 0)  { therapy_pause();  printf("therapy pause\r\n"); }
    else if (strncmp(p, "resume", 6) == 0) { therapy_resume(); printf("therapy resume\r\n"); }
    else if (strncmp(p, "status", 6) == 0) { therapy_print_status(); }
    else if (strncmp(p, "scale", 5) == 0) {
        int s = (int)strtol(p + 5, NULL, 0);
        therapy_set_scale((uint32_t)(s > 0 ? s : 1));
    }
    else if (strncmp(p, "stime", 5) == 0) {
        /* stime <rx> <scheme> <stage> [ms]  改写/查询处方阶段时长 */
        const char *q = p + 5;
        char *end = NULL;
        while (*q == ' ' || *q == '\t') q++;
        int rx = (int)strtol(q, &end, 0);  q = end;
        while (*q == ' ' || *q == '\t') q++;
        int sc = (int)strtol(q, &end, 0);  q = end;
        while (*q == ' ' || *q == '\t') q++;
        int st = (int)strtol(q, &end, 0);  q = end;
        if (rx < 1 || rx > RX_MAX || sc < 0 || st < 0) {
            printf("usage: therapy stime <rx 1~4> <scheme> <stage> [ms]\r\n");
            return;
        }
        const rx_prescription_t *prx = g_rx_all[rx - 1];
        if (!prx || sc >= prx->scheme_cnt) {
            printf("scheme %d out of range (rx%d has %d schemes)\r\n", sc, rx, (int)(prx ? prx->scheme_cnt : 0));
            return;
        }
        const rx_scheme_t *scheme = &prx->schemes[sc];
        if (st >= scheme->stage_cnt) {
            printf("stage %d out of range (scheme has %d stages)\r\n", st, (int)scheme->stage_cnt);
            return;
        }
        while (*q == ' ' || *q == '\t') q++;
        if (*q != '\0') {
            /* 有第4个参数 = 改写 */
            uint32_t ms = (uint32_t)strtoul(q, &end, 0);
            int r = therapy_set_stage_time(scheme, (uint8_t)st, ms);
            printf("rx%d scheme%d stage%d stage_time -> %u ms (%d min %d s) [%s]\r\n",
                   rx, sc, st, (unsigned)ms, (int)(ms / 60000), (int)((ms % 60000) / 1000),
                   r == MB_OK ? "OK" : "ERR");
        } else {
            /* 无第4个参数 = 查询 */
            uint32_t ms = therapy_get_stage_time(scheme, (uint8_t)st);
            printf("rx%d scheme%d stage%d stage_time = %u ms (%d min %d s)\r\n",
                   rx, sc, st, (unsigned)ms, (int)(ms / 60000), (int)((ms % 60000) / 1000));
        }
    }
    else if (strncmp(p, "ttime", 5) == 0) {
        /* ttime [ms]  改写/查询会话总治疗时长 */
        const char *q = p + 5;
        char *end = NULL;
        while (*q == ' ' || *q == '\t') q++;
        if (*q != '\0') {
            uint32_t ms = (uint32_t)strtoul(q, &end, 0);
            therapy_set_total_time(ms);
            printf("total_time -> %u ms (%d min %d s)\r\n",
                   (unsigned)ms, (int)(ms / 60000), (int)((ms % 60000) / 1000));
        } else {
            uint32_t ms = therapy_get_total_time();
            printf("total_time = %u ms (%d min %d s)\r\n",
                   (unsigned)ms, (int)(ms / 60000), (int)((ms % 60000) / 1000));
        }
    }
    else {
        printf("unknown therapy subcommand (therapy help)\r\n");
    }
}

static void cmd_top(void)
{
#if CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
    char buf[1024];
    vTaskGetRunTimeStats(buf);
    printf("Task          Abs Time      %% CPU\r\n");
    printf("%s", buf);
#else
    printf("top: CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS not enabled\r\n");
#endif
}

static void cmd_force(void)
{
    float env = emg_force_get_env();
    float rms = emg_force_get_rms();
    printf("force env=%.1f raw16 (%.0f uV)  rms=%.1f raw16 (%.0f uV)\r\n",
           (double)env, (double)(env * EMG_DSP_RAW16_LSB_UV),
           (double)rms, (double)(rms * EMG_DSP_RAW16_LSB_UV));
    printf("acq ring=%u/%d samples, push=%s\r\n",
           app_modbus_acq_available(), MB_ACQ_RING_CAP,
           app_modbus_push_active() ? "on" : "off");
}

/* [我们] 盆底评估调试：eval start / eval stop / eval（status） */
static void cmd_eval(const char *arg)
{
    if (strncmp(arg, "start", 5) == 0) {
        emg_force_reset();
        emg_force_start_acq(500, 1);
        esp_err_t e = eval_start();
        printf("eval start: %s\r\n", e == ESP_OK ? "OK" : "FAIL");
    } else if (strncmp(arg, "stop", 4) == 0) {
        eval_stop();
        emg_force_stop_acq();
        printf("eval stop: OK\r\n");
    } else {
        const eval_result_t *r = eval_get_result();
        printf("eval state=%d phase=%s remain=%ds/%ds\r\n",
               (int)eval_get_state(), eval_phase_name(),
               eval_remain_s(), eval_phase_total_s());
        printf("  baseline=%.1fuV\r\n", (double)r->baseline);
        printf("  rest1  avg=%.1f var=%.1f%%\r\n", (double)r->rest1_avg, (double)r->rest1_var);
        printf("  fast   max=%.1f(%.1fx) rise=%.2fs rec=%.2fs\r\n",
               (double)r->fast_max, (double)r->fast_max_ratio,
               (double)r->fast_rise, (double)r->fast_recover);
        printf("  sust   avg=%.1f(%.1fx) var=%.1f%%\r\n",
               (double)r->sust_avg, (double)r->sust_ratio, (double)r->sust_var);
        printf("  rest2  avg=%.1f var=%.1f%%\r\n", (double)r->rest2_avg, (double)r->rest2_var);
        printf("  total=%d  (force=%d endu=%d rise=%d rec=%d rest=%d sust=%d)\r\n",
               r->total, r->score_force, r->score_endu, r->score_rise,
               r->score_recover, r->score_rest, r->score_sust);
    }
}

/* ---- 阶段 D：OTA WiFi STA 验证命令 ---- */
static void cmd_wifi(const char *arg)
{
    const char *p = arg;
    while (*p == ' ' || *p == '\t') p++;

    if (strncmp(p, "help", 4) == 0 || *p == '\0') {
        printf("wifi: init | connect | disconnect | state | ip\r\n");
        return;
    }
    if      (strncmp(p, "init", 4) == 0) {
        esp_err_t e = app_wifi_init();
        printf("wifi init -> %s\r\n", e == ESP_OK ? "OK" : esp_err_to_name(e));
    }
    else if (strncmp(p, "connect", 7) == 0) {
        esp_err_t e = app_wifi_connect();
        printf("wifi connect -> %s (state=%d)\r\n", e == ESP_OK ? "OK" : esp_err_to_name(e), (int)app_wifi_get_state());
    }
    else if (strncmp(p, "disconnect", 10) == 0) {
        esp_err_t e = app_wifi_disconnect();
        printf("wifi disconnect -> %s\r\n", e == ESP_OK ? "OK" : esp_err_to_name(e));
    }
    else if (strncmp(p, "state", 5) == 0) {
        static const char *names[] = { "IDLE", "CONNECTING", "CONNECTED", "FAILED" };
        int s = (int)app_wifi_get_state();
        printf("wifi state=%d (%s)\r\n", s, names[s]);
    }
    else if (strncmp(p, "ip", 2) == 0) {
        char ip[16];
        app_wifi_get_ip(ip, sizeof(ip));
        printf("wifi ip=%s\r\n", ip);
    }
    else {
        printf("unknown wifi subcommand (wifi help)\r\n");
    }
}

/* ---- 阶段 D：OTA 验证命令 ---- */
static void cmd_ota(const char *arg)
{
    const char *p = arg;
    while (*p == ' ' || *p == '\t') p++;

    if (strncmp(p, "help", 4) == 0 || *p == '\0') {
        printf("ota: manifest | esp32 | image | stm32 | vercmp\r\n");
        printf("  manifest : fetch version.json and print 3 targets (async)\r\n");
        printf("  esp32    : upgrade ESP32 firmware (uses manifest esp32 url, async)\r\n");
        printf("  image    : upgrade image storage (uses manifest image url, async)\r\n");
        printf("  stm32    : download STM32 fw to /spiffs/fw (async)\r\n");
        printf("  vercmp   : self-test two-part version compare (sync)\r\n");
        printf("  (all run in dedicated ota task, 16KB stack)\r\n");
        return;
    }

    /* vercmp 自测：同步纯函数，直接在 console 任务跑 */
    if (strncmp(p, "vercmp", 6) == 0) {
        struct { const char *a, *b; int expect; } cases[] = {
            { "1.0", "1.0", 0 }, { "1.0", "2.0", -1 }, { "2.0", "1.0", 1 },
            { "1.2", "1.10", -1 }, { "1.0.5", "1.0", 0 }, { "2.3", "2.3", 0 },
        };
        char buf[8];
        app_ota_ver_bcd_to_str(APP_FW_VERSION, buf, sizeof(buf));
        printf("local APP_FW_VERSION 0x%04X -> %s\r\n", (unsigned)APP_FW_VERSION, buf);
        int fail = 0;
        for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
            int r = app_ota_ver_compare(cases[i].a, cases[i].b);
            int got = (r > 0) - (r < 0);
            int ok = (got == cases[i].expect);
            if (!ok) fail++;
            printf("  cmp(%s,%s)=%d expect %d %s\r\n", cases[i].a, cases[i].b, got,
                   cases[i].expect, ok ? "OK" : "FAIL");
        }
        printf("vercmp self-test %s (%d fail)\r\n", fail ? "FAILED" : "PASS", fail);
        return;
    }

    ota_cmd_t cmd;

    if (strncmp(p, "manifest", 8) == 0) {
        cmd = OTA_CMD_FETCH_MANIFEST;
    }
    else if (strncmp(p, "esp32", 5) == 0) {
        cmd = OTA_CMD_UPGRADE_ESP32;
    }
    else if (strncmp(p, "image", 5) == 0) {
        cmd = OTA_CMD_UPGRADE_IMAGE;
    }
    else if (strncmp(p, "stm32", 5) == 0) {
        cmd = OTA_CMD_UPGRADE_STM32;
    }
    else {
        printf("unknown ota subcommand (ota help)\r\n");
        return;
    }

    esp_err_t e = app_ota_request(cmd);
    if (e == ESP_OK) {
        printf("ota: command dispatched to ota task (watch log)\r\n");
    } else {
        printf("ota: busy or not started: %s\r\n", esp_err_to_name(e));
    }
}

static void console_task(void *arg)
{
    char line[128];
    size_t len;
    char c;
    (void)arg;

    printf("\r\n=== EDA_EMG_ESP32 console ===\r\n");
    printf("type 'help' for commands\r\n");

    while (1) {
        printf("> ");
        fflush(stdout);
        len = 0;

        /* 走 VFS stdin（USB-Serial-JTAG 已注册）；read 返回 0=无数据、>0=1 字符 */
        while (1) {
            if (read(0, &c, 1) <= 0) {
                vTaskDelay(pdMS_TO_TICKS(20));   /* 无数据：让出 CPU，避免忙等/刷屏 */
                continue;
            }
            if (c == '\n' || c == '\r') {
                if (len > 0) break;   /* 行结束 */
                continue;             /* 空行：忽略 */
            }
            if (len < sizeof(line) - 1) {
                line[len++] = c;
            }
        }
        if (len == 0) continue;       /* 无有效命令，重新提示 */

        line[len] = '\0';
        if      (strcmp(line, "help")  == 0) cmd_help();
        else if (strcmp(line, "info")  == 0) cmd_info();
        else if (strcmp(line, "mem")   == 0) cmd_mem();
        else if (strcmp(line, "psram") == 0) cmd_psram();
        else if (strcmp(line, "stack") == 0) cmd_stack();
        else if (strcmp(line, "part")  == 0) cmd_part();
        else if (strcmp(line, "lcd")   == 0) cmd_lcd();
        else if (strcmp(line, "bench") == 0) cmd_bench();
        else if (strncmp(line, "mb", 2) == 0) cmd_mb(line[2] == ' ' ? line + 3 : "");
        else if (strncmp(line, "spiffs", 6) == 0) cmd_spiffs(line[6] == ' ' ? line + 7 : "");
        else if (strncmp(line, "param", 5) == 0) cmd_param(line[5] == ' ' ? line + 6 : "");
        else if (strncmp(line, "voice", 5) == 0) cmd_voice(line[5] == ' ' ? line + 6 : "");
        else if (strncmp(line, "therapy", 7) == 0) cmd_therapy(line[7] == ' ' ? line + 8 : "");
        else if (strcmp(line, "top")   == 0) cmd_top();
        else if (strcmp(line, "force") == 0) cmd_force();
        else if (strncmp(line, "eval", 4) == 0) cmd_eval(line[4] == ' ' ? line + 5 : "");
        else if (strncmp(line, "wifi", 4) == 0) cmd_wifi(line[4] == ' ' ? line + 5 : "");
        else if (strncmp(line, "ota", 3) == 0) cmd_ota(line[3] == ' ' ? line + 4 : "");
        else printf("unknown: '%s' (type 'help')\r\n", line);
    }
}

esp_err_t app_console_start(void)
{
    BaseType_t r = xTaskCreate(console_task, "console", 4096, NULL, 5, NULL);
    if (r != pdPASS) {
        ESP_LOGE(TAG, "create console task failed");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "console started (type 'help' in monitor)");
    return ESP_OK;
}
