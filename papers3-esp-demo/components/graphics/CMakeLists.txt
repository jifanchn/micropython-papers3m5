file(GLOB SOURCES *.c *.cpp)
# file(GLOB SOURCES *.c *.cpp "Fonts" "Fonts/Custom" "Fonts/GFXFF" "utility")
# file(GLOB HEADERS "Fonts" "Fonts/Custom" "Fonts/GFXFF" "utility")
set(requires "driver" "arduino-esp32" "freetype" "TFT_eSPI")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . ${HEADERS}
                    REQUIRES ${requires}
)
