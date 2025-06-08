
idf_component_register (
    SRC_DIRS
        src
        src/utilities/BMI270-Sensor-API
        src/utilities/BMM150-Sensor-API
    INCLUDE_DIRS
        src
        src/utilities/BMI270-Sensor-API
        src/utilities/BMM150-Sensor-API
    REQUIRES
        arduino-esp32
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-error -Wno-unused-variable -Wno-unused-result -Wunused-function -Wno-narrowing -Wno-array-bounds -Wno-reorder -fno-if-conversion)
set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -Wno-error)