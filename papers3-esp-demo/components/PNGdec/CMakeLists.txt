file(GLOB SOURCES src/*.c src/*.cpp)
# file(GLOB SOURCES *.c *.cpp "Fonts" "Fonts/Custom" "Fonts/GFXFF" "utility")
# file(GLOB HEADERS "Fonts" "Fonts/Custom" "Fonts/GFXFF" "utility")
set(requires arduino-esp32)

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS ./src ${HEADERS}
                    REQUIRES ${requires}
)
