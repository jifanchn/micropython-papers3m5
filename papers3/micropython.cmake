# Papers3 MicroPython模块 - 完整EPDiy集成
# 支持M5Stack Papers3硬件的MicroPython模块

# 添加Papers3模块目录
set(PAPERS3_DIR ${CMAKE_CURRENT_LIST_DIR})

# ===== EPDiy集成配置 =====
set(EPDIY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../epdiy)

# EPDiy 源文件配置 (基于papers3-esp-demo/components/epdiy/CMakeLists.txt)
set(EPDIY_SOURCES
    # 核心文件
    ${EPDIY_ROOT}/src/epdiy.c
    ${EPDIY_ROOT}/src/render.c
    ${EPDIY_ROOT}/src/font.c
    ${EPDIY_ROOT}/src/displays.c
    ${EPDIY_ROOT}/src/board_specific.c
    ${EPDIY_ROOT}/src/builtin_waveforms.c
    ${EPDIY_ROOT}/src/highlevel.c
    
    # LCD输出支持 - 现在添加回来
    ${EPDIY_ROOT}/src/output_lcd/render_lcd.c
    ${EPDIY_ROOT}/src/output_lcd/lcd_driver.c
    
    # I2S输出支持
    ${EPDIY_ROOT}/src/output_i2s/render_i2s.c
    ${EPDIY_ROOT}/src/output_i2s/rmt_pulse.c
    ${EPDIY_ROOT}/src/output_i2s/i2s_data_bus.c
    
    # 通用输出支持
    ${EPDIY_ROOT}/src/output_common/lut.c
    ${EPDIY_ROOT}/src/output_common/lut.S
    ${EPDIY_ROOT}/src/output_common/line_queue.c
    ${EPDIY_ROOT}/src/output_common/render_context.c
    ${EPDIY_ROOT}/src/output_common/render_method.c
    
    # 向量扩展优化汇编文件
    ${EPDIY_ROOT}/src/diff.S
    
    # 板级支持
    ${EPDIY_ROOT}/src/board/epd_board.c
    ${EPDIY_ROOT}/src/board/epd_board_common.c
    ${EPDIY_ROOT}/src/board/tps65185.c
    ${EPDIY_ROOT}/src/board/pca9555.c
)

# Papers3模块源文件 (包含Touch模块，移除Button模块)
set(PAPERS3_SOURCES
    ${PAPERS3_DIR}/modpapers3.c
    ${PAPERS3_DIR}/papers3_buzzer.c
    ${PAPERS3_DIR}/papers3_battery.c
    ${PAPERS3_DIR}/papers3_gyro.c
    ${PAPERS3_DIR}/papers3_rtc.c
    ${PAPERS3_DIR}/papers3_epdiy.c
    ${PAPERS3_DIR}/papers3_led.c
    ${PAPERS3_DIR}/papers3_touch.c
)

# 包含目录
set(PAPERS3_INCLUDES
    ${PAPERS3_DIR}
    ${EPDIY_ROOT}/src
    ${EPDIY_ROOT}/src/output_common
    ${EPDIY_ROOT}/src/output_lcd
    ${EPDIY_ROOT}/src/output_i2s
    ${EPDIY_ROOT}/src/board
)

# 添加Papers3用户模块
add_library(usermod_papers3 INTERFACE)

target_sources(usermod_papers3 INTERFACE
    ${PAPERS3_SOURCES}
    ${EPDIY_SOURCES}
)

target_include_directories(usermod_papers3 INTERFACE
    ${PAPERS3_INCLUDES}
    # 直接添加ESP-IDF esp_lcd组件包含路径
    $ENV{IDF_PATH}/components/esp_lcd/include
    $ENV{IDF_PATH}/components/esp_lcd/rgb/include
)

# 链接MicroPython用户模块
target_link_libraries(usermod INTERFACE usermod_papers3)

# ===== ESP-IDF组件依赖 =====
# 确保所有必要的ESP-IDF组件被正确链接
set(COMPONENT_REQUIRES
    driver
    esp_common
    esp_hw_support
    esp_timer
    freertos
    heap
    log
    soc
    hal
    esp_rom
    esp_system
    esp_lcd
    esp_pm
    esp_adc
    esp_driver_gpio
    esp_driver_i2c
    esp_driver_spi
    esp_driver_uart
    esp_driver_ledc
    spi_flash
    efuse
    bootloader_support
    app_update
    nvs_flash
    partition_table
    esptool_py
)

# 添加ESP-IDF组件依赖 (esp_lcd)
# 这是确保esp_lcd组件被正确链接的关键
if(NOT CMAKE_BUILD_EARLY_EXPANSION)
    # 添加esp_lcd组件到IDF_COMPONENTS
    list(APPEND IDF_COMPONENTS esp_lcd)
endif()

# 为EPDiy配置编译定义
target_compile_definitions(usermod_papers3 INTERFACE
    -DCONFIG_EPD_BOARD_REVISION_V7=1
    -DCONFIG_EPD_DISPLAY_TYPE_ED047TC2=1
    -DCONFIG_EPD_BUS_IMPL_I2S=1
    # 重要：禁用LCD渲染方法避免FreeRTOS冲突
    # -DRENDER_METHOD_LCD=1
)

# 编译选项优化
target_compile_options(usermod_papers3 INTERFACE
    -Wno-unused-function
    -Wno-unused-variable
    -Wno-missing-field-initializers
    -O2
)

# Papers3硬件配置常量
target_compile_definitions(usermod_papers3 INTERFACE
    -DPAPERS3_EPD_WIDTH=960
    -DPAPERS3_EPD_HEIGHT=540
    -DPAPERS3_BUZZER_PIN=21
    -DPAPERS3_BATTERY_PIN=3
    -DPAPERS3_I2C_SDA=12
    -DPAPERS3_I2C_SCL=11
    -DPAPERS3_BMI270_ADDR=0x68
    -DPAPERS3_BM8563_ADDR=0x51
)

# ===== 冻结模块配置 =====
# 添加Papers3 Python模块到冻结模块
set(MICROPY_FROZEN_MANIFEST ${PAPERS3_DIR}/manifest.py)

# ===== 调试信息 =====
message(STATUS "Papers3 module configured:")
message(STATUS "  EPDiy source: ${EPDIY_ROOT}")
message(STATUS "  Papers3 sources: ${PAPERS3_SOURCES}")
message(STATUS "  Include paths: ${PAPERS3_INCLUDES}")
message(STATUS "  Component requirements: ${COMPONENT_REQUIRES}")
message(STATUS "  Frozen manifest: ${MICROPY_FROZEN_MANIFEST}")
message(STATUS "  ✅ Ready for build") 