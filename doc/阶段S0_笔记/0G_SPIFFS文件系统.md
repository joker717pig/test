# 0G — SPIFFS 文件系统 + 资源烧录

> 阶段 S0 | 完成 2026-08-11 | 验证通过

## 目标

让 ESP32 的 SPIFFS 分区（`storage`, ~12MB）可挂载可读写，把同事 UI 代码的全部 `.bin` 资源文件（字体/图片/图标）烧录进 Flash。

**不涉及** LVGL FS 驱动对接（那是阶段 B1）。

---

## 一、资源文件来源

| 源 | 目标（SPIFFS 内） | 数量 | 大小 |
|---|---|---|---|
| `D:\EDA_LZQ\盆底肌\mylvgldemo1\Resource\font\*.bin` | `/spiffs/font/` | 7 | ~31KB |
| `D:\EDA_LZQ\盆底肌\mylvgldemo1\Resource\image\*.bin` | `/spiffs/image/` | 10 | ~219KB |
| `D:\EDA_LZQ\盆底肌\mylvgldemo1\Resource\icon\*.bin` | `/spiffs/icon/` | 72 | ~1.44MB |
| **总计** | | **89** | **~1.65MB** |

复制命令：
```powershell
mkdir -p spiffs_data/font; mkdir spiffs_data/image; mkdir spiffs_data/icon
Copy-Item "D:\EDA_LZQ\盆底肌\mylvgldemo1\Resource\font\*.bin" spiffs_data/font/
Copy-Item "D:\EDA_LZQ\盆底肌\mylvgldemo1\Resource\image\*.bin" spiffs_data/image/
Copy-Item "D:\EDA_LZQ\盆底肌\mylvgldemo1\Resource\icon\*.bin" spiffs_data/icon/
```

---

## 二、配置变更

### 2.1 `partitions.csv`（已有，无需改）

```
storage, data, spiffs, 0x410000, 0xBF0000,
```

### 2.2 `sdkconfig.defaults`（追加）

```ini
CONFIG_SPIFFS_MAX_PARTITIONS=1
CONFIG_SPIFFS_USE_MTIME=y
```

### 2.3 根 `CMakeLists.txt`（追加）

```cmake
spiffs_create_partition_image(storage ${CMAKE_SOURCE_DIR}/spiffs_data FLASH_IN_PROJECT)
```

`FLASH_IN_PROJECT` 让 SPIFFS 镜像随 `idf.py flash` 一起烧录，**无需单独烧录步骤**。

### 2.4 `main/CMakeLists.txt`（REQUIRES 追加 `"spiffs"`）

```cmake
REQUIRES "... esp_driver_uart spiffs"
```

---

## 三、代码变更

### 3.1 新建 `main/app_spiffs.h`

```c
esp_err_t app_spiffs_init(void);        // 挂载 SPIFFS → /spiffs
void app_spiffs_print_info(void);       // 打印 total/used/free
void app_spiffs_list(const char *path); // 递归列文件
void app_spiffs_cat(const char *path, size_t offset, size_t len); // hex dump
```

### 3.2 新建 `main/app_spiffs.c`

核心逻辑：
```c
esp_vfs_spiffs_conf_t conf = {
    .base_path       = "/spiffs",
    .partition_label = "storage",
    .max_files       = 5,
    .format_if_mount_failed = false,
};
esp_vfs_spiffs_register(&conf);
```

`list_dir()` 用标准 POSIX `opendir/readdir/stat`（ESP-IDF VFS 层透明转发到 SPIFFS）。

### 3.3 `main/console/app_console.c`

新增 `spiffs` 命令组：
```
spiffs info                      → 分区信息
spiffs ls [path]                 → 递归列文件
spiffs cat <path> [off] [len]    → hex dump
```

dispatch：`else if (strncmp(line, "spiffs", 6) == 0) cmd_spiffs(...)`

### 3.4 `main/app_main.c`

在事件框架初始化之前调用：
```c
ESP_ERROR_CHECK(app_spiffs_init());
```

---

## 四、验证记录（2026-08-11 COM5）

```
idf.py build                    → 零错误
idf.py -p COM5 flash monitor    → 烧录成功
```

monitor 输出：
```
[spiffs] partition 'storage' mounted at /spiffs
[spiffs] total: 11229 KB, used: 1734 KB, free: 9494 KB
```

console 验证：
```
> spiffs info
[spiffs] partition 'storage' mounted at /spiffs
[spiffs] total: 11229 KB, used: 1734 KB, free: 9494 KB

> spiffs ls /spiffs/font
lv_font_14.bin                      412 B
lv_font_30.bin                     1680 B
lv_font_Btn.bin                    6300 B
lv_font_chinese.bin                5240 B
lv_font_Text.bin                   9036 B
lv_font_Title.bin                  7100 B
lv_font_Unit.bin                   1984 B

> spiffs ls /spiffs/image
battery2.bin                       7512 B
heart_image2.bin                   4812 B
Home_image.bin                     1212 B
image_text8.bin                  180012 B
other.bin                          1212 B
record_image.bin                   1887 B
set_image.bin                      1887 B
suo2.bin                           7512 B
suo3.bin                           7512 B
warn_image.bin                     4812 B

> spiffs ls /spiffs/icon
(72 files, all listed)

> spiffs cat /spiffs/font/lv_font_Btn.bin 0 64
  0000: 30 00 00 00 68 65 61 64 01 00 00 00 04 00 0F 00  0...head........
  0010: 0D 00 FD FF 10 00 FC FF 00 00 FD FF 0D 00 00 00  ................
  0020: 10 00 00 00 01 01 04 04 09 00 00 00 01 00 00 00  ................
  0030: 6C 01 00 00 63 6D 61 70 05 00 00 00 5C 00 00 00  l...cmap....\...
```

文件头 `head` + `cmap` 表 → **确认是合法的 LVGL binary font 格式**。

---

## 五、遇到的坑

### 坑 1：SPIFFS `--obj-name-len=32` 文件名长度限制

`lv_font_honorans_medium_14.bin` = 33 字符 → 超过 SPIFFS 默认 32 字符限制。

```
RuntimeError: object name '/font/lv_font_honorans_medium_14.bin' too long
```

**解决**：改名为 `lv_font_14.bin`。注意：后续 `basic.c` 适配时，路径中的文件名也要对应修改。

> CMake 的 `spiffs_create_partition_image()` 不支持直接传 `OBJ_NAME_LEN` 参数（ESP-IDF 5.5.3 实测不生效），故采用重命名方案。

### 坑 2：`REQUIRES` 缺少 `spiffs` 组件

编译报错 `esp_spiffs.h: No such file or directory`。

**解决**：`main/CMakeLists.txt` 的 `REQUIRES` 追加 `"spiffs"`。
> ESP-IDF 的 `REQUIRES` 会**替换**默认 requirements，非 common 组件必须显式声明。

### 坑 3：`struct stat` 用 `st_mode` 非 `st_type`

ESP32 newlib 的 `struct stat` 没有 `st_type` 字段（这是 BSD 扩展）。

`S_ISDIR(st.st_type)` → `S_ISDIR(st.st_mode)`

### 坑 4：`snprintf` 格式截断警告（`-Werror=format-truncation`）

```c
char full[256];
snprintf(full, sizeof(full), "%s/%s", path, entry->d_name);
// path + "/" + d_name 可能超过 256 → 编译报错
```

**解决**：buf 扩大至 512 + 检查返回值。

### 坑 5：`app_console.c` 编辑破坏

用 `replace_string_in_file` 多次编辑同一文件时，前一次编辑的 `oldString` 末尾 `}` 被消耗（闭合 `cmd_mb`），导致后续函数 `cmd_spiffs` 和 `console_task` 被嵌套进 `cmd_mb` 内部。

**解决**：手动补齐 `cmd_mb` 丢失的闭合 `}`，完整重写 `console_task` 函数体。

---

## 六、烧录命令模板

```powershell
# 设环境变量（本机受限 PowerShell 用 .NET API）
[Environment]::SetEnvironmentVariable("IDF_PATH","C:\esp\v5.5.3\esp-idf")
[Environment]::SetEnvironmentVariable("IDF_TOOLS_PATH","C:\Espressif\tools")
[Environment]::SetEnvironmentVariable("IDF_PYTHON_ENV_PATH","C:\Espressif\tools\python\v5.5.3\venv")
$env:PATH="C:\Espressif\tools\cmake\3.30.2\bin;C:\Espressif\tools\ninja\1.12.1;C:\Espressif\tools\xtensa-esp-elf\esp-14.2.0_20251107\xtensa-esp-elf\bin;C:\Espressif\tools\python\v5.5.3\venv\Scripts;$env:PATH"

# 构建
cd d:\GitLab\EDA_EMG_ESP32
C:\Espressif\tools\python\v5.5.3\venv\Scripts\python.exe C:\esp\v5.5.3\esp-idf\tools\idf.py build

# 烧录+monitor（COM5；storage.bin 随固件自动烧录）
C:\Espressif\tools\python\v5.5.3\venv\Scripts\python.exe C:\esp\v5.5.3\esp-idf\tools\idf.py -p COM5 flash monitor
```

烧录命令行包含 4 个分区：
```
0x0        bootloader/bootloader.bin
0x10000    eda_emg_esp32.bin
0x8000     partition-table.bin
0x410000   storage.bin           ← SPIFFS 镜像
```

---

## 七、下一步（阶段 B1）

- 启用 `CONFIG_LV_USE_FS_STDIO=y`，注册 LVGL FS 驱动（letter='S'）
- `basic.h/c` 适配：
  - `chinese_font_init()` 中 `lv_binfont_create("C:/Users/zqt/...")` → `lv_binfont_create("S:/font/lv_font_xxx.bin")`
  - `Creat_Image()` 中图片路径改 SPIFFS 路径
  - 局部变量/头文件路径适配 ESP-IDF 规范
- Frame 顶栏 + Menu 主菜单移植
- 按键→菜单导航（Modbus `INPUT_EVENT` → LVGL keypad indev）
