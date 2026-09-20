# EDA_EMG_ESP32 main 组件
# 依赖说明：写 REQUIRES 会替换默认 requirements（common 由构建系统 link_libraries()
#           无条件注入，但 spi_flash/esp_psram/esp_event/esp_partition/esp_lcd
#           等非 common 组件必须在此显式声明，否则头文件找不到）。
# 显式声明：esp_lcd(屏驱动) + 本组件直接 include 的非 common 组件。
# 目录结构（阶段 E2 架构分层）：
#   ui/         = 公共操作层（basic 控件工厂 / Frame 顶栏 / lang 语言机制 / font 字库）
#   ui_page_cn/ = 中文纯页面（MenuPage / SetPage / PosReh_Page / PelFlo_AssPage）
#   ui_page_en/ = 英文纯页面（MenuPage / SetPage / ...）
#   ui_port/    = 平台移植层（LVGL 接入 / keypad / 'C'盘FS路径翻译）—— 我们的代码
#   ui_page_cn/ = 同事交付的页面代码（原装 Windows 路径，靠 ui_port/app_fs 翻译）
# SRC_DIRS **逐个**列出含 .c 的目录（不递归子目录）；INCLUDE_DIRS 覆盖各子目录。
idf_component_register(SRC_DIRS "." "events" "debug" "lcd" "console" "MODBUS" "dsp" "params" "heartbeat" "voice" "therapy" "ota"
                                  "ui" "ui/font" "ui_port"
                                  "ui_page_cn/MenuPage" "ui_page_cn/PelFlo_AssPage" "ui_page_cn/SetPage" "ui_page_cn/PosReh_Page"
                                  "ui_page_cn/PelFlo_ThePage"
                                  "ui_page_en" "ui_page_en/MenuPage" "ui_page_en/SetPage" "ui_page_en/PosReh_Page" "ui_page_en/PelFlo_AssPage" "ui_page_en/PelFlo_ThePage"
                       INCLUDE_DIRS "." "common" "events" "debug" "lcd" "console" "dsp" "params" "heartbeat" "voice" "therapy" "ota"
                                    "ui" "ui/font" "MODBUS"
                                    "ui_port" "ui" "ui_page_cn"
                                    "ui_page_cn/MenuPage" "ui_page_cn/PelFlo_AssPage" "ui_page_cn/SetPage" "ui_page_cn/PosReh_Page"
                                    "ui_page_cn/PelFlo_ThePage"
                                    "ui_page_en" "ui_page_en/MenuPage" "ui_page_en/SetPage" "ui_page_en/PosReh_Page" "ui_page_en/PelFlo_AssPage" "ui_page_en/PelFlo_ThePage"
                       REQUIRES "esp_lcd" "esp_driver_spi" "esp_driver_gpio"
                                "spi_flash" "esp_psram" "esp_event" "esp_partition"
                                "esp_timer" "esp_driver_uart" "spiffs" "nvs_flash"
                                "esp_wifi" "esp_netif" "esp_http_client"
                                "json" "mbedtls" "app_update")
