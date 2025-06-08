file(GLOB SOURCES *.c *.cpp "Extensions" "Fonts" "Fonts/Custom" "Fonts/GFXFF" "Processors" "TFT_Drivers" "User_Setups")
file(GLOB HEADERS "Extensions" "Fonts" "Fonts/Custom" "Fonts/GFXFF" "Processors" "TFT_Drivers" "User_Setups")
set(requires "driver" "arduino-esp32" "freetype")

idf_component_register(SRCS ${SOURCES} "Fonts/ffsupport.cpp" "Fonts/font_render.cpp"
                    INCLUDE_DIRS . ${HEADERS}
                    REQUIRES ${requires}
)