# MicroPython User Module for Papers3
# Simplified implementation with battery and buzzer only

# Papers3 module sources (no EPDiy for now)
set(PAPERS3_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/modpapers3.c
    ${CMAKE_CURRENT_LIST_DIR}/papers3_buzzer.c
    ${CMAKE_CURRENT_LIST_DIR}/papers3_battery.c
)

# Create user module
add_library(usermod_papers3 INTERFACE)

target_sources(usermod_papers3 INTERFACE
    ${PAPERS3_SOURCES}
)

target_include_directories(usermod_papers3 INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_compile_definitions(usermod_papers3 INTERFACE
    MODULE_PAPERS3_ENABLED=1
)

# Link with required ESP-IDF components for basic functionality
target_link_libraries(usermod_papers3 INTERFACE
    idf::freertos
    idf::esp_timer
    idf::driver
    idf::esp_driver_gpio
    idf::esp_driver_ledc
)

target_link_libraries(usermod INTERFACE usermod_papers3) 