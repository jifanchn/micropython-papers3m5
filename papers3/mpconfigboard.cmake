set(IDF_TARGET esp32s3)

set(SDKCONFIG_DEFAULTS
    boards/sdkconfig.base
    boards/sdkconfig.usb
    boards/sdkconfig.ble
    boards/sdkconfig.240mhz
    boards/sdkconfig.spiram_sx
    boards/PAPERS3/sdkconfig.board
)

# Register Papers3 as user module
set(USER_C_MODULES
    ${MICROPY_BOARD_DIR}/../../../../../papers3/micropython.cmake
) 