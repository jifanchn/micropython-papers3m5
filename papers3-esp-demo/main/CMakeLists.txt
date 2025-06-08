file(GLOB SOURCES *.c *.cpp)

file(GLOB EXTERN_FILES resources/*)
set(requires "TFT_eSPI" "graphics" "freetype" "arduino-esp32" "epdiy" "ArduinoJson" "Arduino_BMI270_BMM150" "Reefwing-AHRS" "LovyanGFX" "PNGdec")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS .  ${HEADERS}
                    REQUIRES ${requires}
                    EMBED_FILES ${EXTERN_FILES}
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-result -Wunused-function -Wno-narrowing -Wno-array-bounds -Wno-reorder -Wno-strict-aliasing -Wno-missing-field-initializers)
set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -Wno-error)