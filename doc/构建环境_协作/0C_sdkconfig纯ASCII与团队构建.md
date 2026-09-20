# 0C sdkconfig 纯 ASCII 与团队构建（同事编译崩溃排查）

> 日期：2026-08-17　工程：EDA_EMG_ESP32　关联 commit：`6afaaa4` / `3467c06`

## 现象
- 同事从 git clone 后 `idf.py build` 在配置阶段崩溃：
  ```
  UnicodeDecodeError: 'gbk' codec can't decode byte 0xae in position 103: illegal multibyte sequence
  CMake Error at .../tools/cmake/kconfig.cmake:209 (message): Failed to run kconfgen
  ```

## 根因
- `sdkconfig.defaults` 里有**中文注释**（如 `# 硬件：ESP32-S3...`），文件以 **UTF-8** 保存。
- ESP-IDF 用 Python `kconfgen` 读取该文件，Python 在中文 Windows 上默认按 **GBK(cp936)** 解码，
  UTF-8 中文的尾字节（本例 `0xae` = "默认配置"里"认"字 UTF-8 的尾字节，position 103 完全吻合）在 GBK 下非法 → 崩溃。
- 本机不崩是因为构建环境为 UTF-8 locale；同事机器为 GBK locale。**与代码无关，纯编码问题。**

## 修复（已提交 6afaaa4，本机 v5.5.3 构建通过）
1. `sdkconfig.defaults`：全部注释改写为**纯 ASCII 英文** → 任何 Windows locale（GBK/UTF-8）都能编译。
   - 验证方法：`[IO.File]::ReadAllBytes(...)` 统计 >127 字节数 = 0。
2. `CMakeLists.txt`：显式 `set(IDF_TARGET "esp32s3")`。
   - 同事日志出现 `target esp32`（应为 esp32s3），疑似目录里有残留旧 `sdkconfig` 把 target 带偏；
     显式锁定后无论有无 sdkconfig 都强制 esp32s3。
3. `.gitignore`：移除 `spiffs_data/` 忽略，84 个资源文件（4.1MB，`png/` 下字体/图标 .bin）**入库**。
   - 之前 spiffs_data 不入库 → 同事 clone 后无此目录 → `spiffs_create_partition_image` 构建 storage 镜像会失败。

## 补充（2026-08-17 同日二次崩溃：Kconfig.projbuild，commit 3467c06 已推送）
- 现象：同事拉取 6afaaa4 后再 build，`UnicodeDecodeError 'gbk' 0xae position 13590`（位置在合并后的**完整配置大文件**中段）。
- 根因：**`main/Kconfig.projbuild` 含 17 处中文**（注释 / menu 标题 / `help` 文本）。kconfgen 把 Kconfig 的中文合并进生成的完整配置，GBK 读它 → 崩（position 13590 正是配置中段）。
- 修复：`main/Kconfig.projbuild` 全部中文转英文；`partitions.csv` 顶部 3 行中文注释也一并转英文（同类风险预防）。两文件验证非 ASCII 字节 = 0。
- **教训（重要）**：工程里**所有被 ESP-IDF 的 Python 工具（kconfgen 等）读取的配置文件**（`sdkconfig.defaults`、`*.projbuild`/Kconfig、`partitions.csv` 等）**都必须纯 ASCII**；只有 `.c/.h/.md` 这类由编译器/编辑器按 UTF-8 读的文件才能写中文注释。

## 规范（团队协作，防止再踩）
- **凡被 Python 以默认编码打开的文件**（`sdkconfig.defaults`、`.defaults.*`、Kconfig 脚本等）**必须保持纯 ASCII**，
  注释一律英文。`.c/.h/.md` 中文注释不受影响（编译器/编辑器按 UTF-8 读）。
- **构建必需的资源目录（spiffs_data）必须入库**，不能靠 .gitignore 排除。
- **团队统一 ESP-IDF 版本 v5.5.3**：工程 sdkconfig 按 5.5 写的，
  5.4 下 `CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM` 等改名选项不识别（仅告警、功能不生效）。
- `dependencies.lock` 保持入库（决策：2026-08-17，不加入 .gitignore）；跨版本 build 会提示更新 lock，属正常。

## 给同事的复现/验证步骤
1. 装 **ESP-IDF v5.5.3**（与工程一致）；旧工程目录如有残留 `sdkconfig` 请删除后再 build
   （git 不提交 sdkconfig，git pull 也不会删本机已有的——`target esp32` / `SPIRAM_MODE_OCT unknown` 都是它引起）。
2. `git pull` 拉取 `3467c06`（含 sdkconfig.defaults + Kconfig.projbuild + partitions.csv 全部 ASCII 修复 + spiffs_data 资源）。
3. 在工程根目录 `idf.py build`，应直接通过。
