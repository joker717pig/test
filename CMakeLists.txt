# EDA_EMG_ESP32 顶层 CMakeLists.txt
# 用法：激活 ESP-IDF 环境后，在本目录运行 `idf.py build`
cmake_minimum_required(VERSION 3.16)

# Force target chip to ESP32-S3 (a stale local sdkconfig could otherwise pull target to esp32)
set(IDF_TARGET "esp32s3")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(eda_emg_esp32)

# ---- 阶段 D：SPIFFS 资源镜像（spiffs_data/ → storage_a / storage_b 双镜像, 随固件烧录）----
# 双镜像：出厂同时写满两个图槽, 任一切换后均有有效数据; OTA 整刷非激活槽后翻转 NVS 标记。
# 产物 storage_a.bin / storage_b.bin 同时作为云端 OTA 镜像发布。
spiffs_create_partition_image(storage_a ${CMAKE_SOURCE_DIR}/spiffs_data FLASH_IN_PROJECT OBJ_NAME_LEN 64)
spiffs_create_partition_image(storage_b ${CMAKE_SOURCE_DIR}/spiffs_data FLASH_IN_PROJECT OBJ_NAME_LEN 64)
